/* Copyright (c) 2007, Thomas Fors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
            
#include "ppp.h"
#include "print.h";

#define KEY_BITS (int)256

static const char * alphabet = "23456789!@#%+=:?abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPRSTUVWXYZ";

static unsigned long rk[RKLENGTH(KEY_BITS)];
static int nRounds = 0;

static mp_int d_seqKey;
static mp_int d_currPasscodeNum;
static mp_int d_lastCardGenerated;
static mp_int d_maxPasscodes;
static char d_passcode[5] = "";
static char *d_prompt = NULL;
static int d_prompt_len = 0;

static void _bytes_to_mp(unsigned char *bytes, mp_int *mp, int len) {
	mp_read_unsigned_bin(mp, bytes, len);
}

static void _mp_to_bytes(mp_int *mp, unsigned char *bytes, int len) {
	int i;
	for (i=0; i<len; i++) {
		bytes[i] = '\x00';
	}
	i = len - mp_unsigned_bin_size(mp);
	mp_to_unsigned_bin(mp, bytes+i);
}

static void _reverse_bytes(unsigned char *buf, int len) {
	/* We use _reverse_bytes() in here because the MPI library
	 * stores it's numbers in opposite endianness to the PPP
	 * sequence key specification.
	 */
	int i;
	unsigned char temp;
	for (i=0; i<len/2; i++) {
		temp = buf[i];
		buf[i] = buf[len-i-1];
		buf[len-i-1] = temp;
	}
}

static void _zero_bytes(unsigned char *buf, int len) {
	int i;
	for (i=0; i<len; i++) {
		buf[i] = '\x00';
	}
}

static void _zero_rijndael_state() {
	int i;
	for (i=0; i<RKLENGTH(KEY_BITS); i++) {
		rk[i] = 0;
	}
	nRounds = 0;
}

static void _locate_passcode(mp_int *passcodeNum, mp_int *cipherNum, mp_int *offset) {
	mp_div_2d(passcodeNum, 4, cipherNum, offset);
	mp_mul_d(cipherNum, 3, cipherNum);
}

static char *_extract_passcode_from_block(mp_int *cipherBlock, int n) {
	static unsigned char cipherdata[16*3];
	_mp_to_bytes(cipherBlock, cipherdata, 16*3);

	int i = n * 3;
	
    d_passcode[0] = alphabet[(int)(cipherdata[i]&0x3f)];
    d_passcode[1] = alphabet[(int)(((cipherdata[i]&0xc0)>>6) + ((cipherdata[i+1]&0x0f)<<2))];
    d_passcode[2] = alphabet[(int)(((cipherdata[i+1]&0xf0)>>4) + ((cipherdata[i+2]&0x03)<<4))];
    d_passcode[3] = alphabet[(int)((cipherdata[i+2]&0xfc)>>2)];
	d_passcode[4] = '\x00';
	
	_zero_bytes(cipherdata, 16*3);
	
	return d_passcode;
}

static void _setup_encrypt(mp_int *key) {
	unsigned char k[32];
	_mp_to_bytes(key, k, 32);
	_reverse_bytes(k, 32);
	
	nRounds = rijndaelSetupEncrypt(rk, k, KEY_BITS);
	
	_zero_bytes(k, 32);
}

static void _encrypt(mp_int *plain, mp_int *cipher) {
	unsigned char p[16];
	unsigned char c[16];
	
	_mp_to_bytes(plain, p, 16);
	_reverse_bytes(p, 16);
	rijndaelEncrypt(rk, nRounds, p, c);
	_bytes_to_mp(c, cipher, 16);
	
	_zero_bytes(p, 16);
	_zero_bytes(c, 16);
}


static void _compute_passcode_block(mp_int *cipherNum, mp_int *cipherBlock) {
	unsigned char seqkey[48];
	_mp_to_bytes(&d_seqKey, seqkey, 48);
	
	/* get encryption key from sequence key (32 MSBs) */
    mp_int key;
	mp_init(&key);
	mp_read_unsigned_bin(&key, seqkey, 32);

	/* prepare for encryption */
	_setup_encrypt(&key);
	mp_clear(&key);

	/* get offset from sequence key (16 LSBs) */
	mp_int offset;
	mp_init(&offset);
	mp_read_unsigned_bin(&offset, seqkey+32, 16);
	_zero_bytes(seqkey, 48);

	/* compute plaintext (offset + N) mod 2^128 */
	mp_int plaintext;
	mp_init(&plaintext);
	mp_add(&offset, cipherNum, &plaintext);
	mp_clear(&offset);
	mp_int modulus;
	mp_init(&modulus);
	mp_set(&modulus, 1);
	mp_mul_2d(&modulus, 128, &modulus);
	mp_mod(&plaintext, &modulus, &plaintext);

	/* get ciphertext */
	unsigned char cipherblock[16*3];
	mp_int cipher;
	mp_init(&cipher);
	_encrypt(&plaintext, &cipher);
	_mp_to_bytes(&cipher, cipherblock, 16);
	mp_add_d(&plaintext, 1, &plaintext);
	mp_mod(&plaintext, &modulus, &plaintext);
	_encrypt(&plaintext, &cipher);
	_mp_to_bytes(&cipher, cipherblock+16, 16);
	mp_add_d(&plaintext, 1, &plaintext);
	mp_mod(&plaintext, &modulus, &plaintext);
	mp_clear(&modulus);
	_encrypt(&plaintext, &cipher);
	mp_clear(&plaintext);
	_zero_rijndael_state();
	_mp_to_bytes(&cipher, cipherblock+32, 16);
	mp_clear(&cipher);

	/* return cipherBlock */
	_bytes_to_mp(cipherblock, cipherBlock, 16*3);
	_zero_bytes(cipherblock, 16*3);
}



void pppInit() {
	mp_init(&d_seqKey);
	mp_init(&d_currPasscodeNum);
	mp_init(&d_lastCardGenerated);
	
	/* Here, we compute the maximum number of passcodes handle by this
	 * code. This turns out to be less than the theoretical maximum
	 * because we don't count the ones from the last (incomplete) 
	 * passcard.
	 * max = int(2^128 * 16 / 3 / 70) * 70
	 */
	mp_init(&d_maxPasscodes);
	mp_set_int(&d_maxPasscodes, 1);
	mp_mul_2d(&d_maxPasscodes, 128, &d_maxPasscodes);
	mp_mul_d(&d_maxPasscodes, 16, &d_maxPasscodes);
	mp_div_d(&d_maxPasscodes, 3, &d_maxPasscodes, NULL);
	mp_div_d(&d_maxPasscodes, 70, &d_maxPasscodes, NULL);
	mp_mul_d(&d_maxPasscodes, 70, &d_maxPasscodes);
}

void pppCleanup() {
	/* MPI code included in this project is configured to zero memory
	 * upon executing mp_clear so this should remove the key and other
	 * sensitive data from memory once you call pppCleanup.
	 */
	mp_clear(&d_seqKey);
	mp_clear(&d_currPasscodeNum);
	mp_clear(&d_lastCardGenerated);
	mp_clear(&d_maxPasscodes);
	
	_zero_bytes((unsigned char *)d_passcode, 5);
	_zero_rijndael_state();
	
	_zero_bytes((unsigned char *)d_prompt, d_prompt_len);
	free(d_prompt);
}

char *currPrompt() {
	mp_int mp;
	mp_int row;

	/* This works on Intel-based Mac OS X, but
	 * it may require tweaking on other platforms
	 * in order to get the endianness correct.
	 */
	union {
		unsigned char bytes[4];
		unsigned int val;
	} c,r;
	c.val = r.val = 0;

	mp_init(&mp);
	mp_init(&row);
                                
	calculateCardContainingPasscode(currPasscodeNum(), &mp);
	mp_add_d(&mp, 1, &mp);
	char *cardstr = mpToDecimalString(&mp, ',');
	mp_sub_d(&mp, 1, &mp);
	
	mp_mul_d(&mp, 70, &row);
	mp_sub(currPasscodeNum(), &row, &row);
	mp_set_int(&mp, 7);
	mp_div(&row, &mp, &row, &mp);
	mp_to_unsigned_bin(&row, r.bytes);
	mp_to_unsigned_bin(&mp, c.bytes);
	mp_clear(&mp);
	mp_clear(&row);
	
	free(d_prompt);
	d_prompt = malloc(strlen("Passcode ():") + strlen(cardstr) + 6);
	sprintf(d_prompt, "Passcode (%s-%c-%d): ", cardstr, c.val+'A', ++r.val);
	
	mp_clear(&row);
	_zero_bytes(c.bytes, 4);
	_zero_bytes(r.bytes, 4);
	
	return d_prompt;
}

mp_int *seqKey() {
	return &d_seqKey;
}

void setSeqKey(mp_int *mp) {
	mp_copy(mp, &d_seqKey);
}

mp_int *currPasscodeNum() {
	return &d_currPasscodeNum;
}

void setCurrPasscodeNum(mp_int *mp) {
	mp_copy(mp, &d_currPasscodeNum);
}

void zeroCurrPasscodeNum() {
	mp_zero(&d_currPasscodeNum);
}

void incrCurrPasscodeNum() {
	mp_add_d(&d_currPasscodeNum, 1, &d_currPasscodeNum);
}

mp_int *lastCardGenerated() {
	return &d_lastCardGenerated;
}

void setLastCardGenerated(mp_int *mp) {
	mp_copy(mp, &d_lastCardGenerated);
}

void zeroLastCardGenerated() {
	mp_zero(&d_lastCardGenerated);
	mp_sub_d(&d_lastCardGenerated, 1, &d_lastCardGenerated);
}

void incrLastCardGenerated() {
	mp_add_d(&d_lastCardGenerated, 1, &d_lastCardGenerated);
}

void calculatePasscodeNumberFromCardColRow(mp_int *card, int col, int row, mp_int *passcodeNum) {
	mp_mul_d(card, 70, passcodeNum);
	mp_add_d(passcodeNum, row*7, passcodeNum);
	mp_add_d(passcodeNum, col, passcodeNum);
}

void calculateCardContainingPasscode(mp_int *passcodeNum, mp_int *cardNum) {
	mp_set_int(cardNum, 70);
	mp_div(passcodeNum, cardNum, cardNum, NULL);
}


void generateSequenceKeyFromPassphrase(char *phrase) {
	unsigned char bytes[48];
	sha384((const unsigned char *)phrase, strlen(phrase), bytes);
	_reverse_bytes(bytes, 48);
	_bytes_to_mp(bytes, &d_seqKey, 48);
	_zero_bytes(bytes, 48);
	
	zeroCurrPasscodeNum();
	zeroLastCardGenerated();
}

void generateRandomSequenceKey() {
	int i;
	uuid_t uuid;
	unsigned char entropy[32];
	unsigned char bytes[48];

	uuid_generate_time(uuid);
	for (i=0; i<16; i++) {
		entropy[i] = uuid[i];
	}
	
	uuid_generate_random(uuid);
	for (i=0; i<16; i++) {
		entropy[i+16] = uuid[i];
	}
	
	sha384(entropy, 32, bytes);
	_zero_bytes(entropy, 32);
	_reverse_bytes(bytes, 48);
	_bytes_to_mp(bytes, &d_seqKey, 48);
	_zero_bytes(bytes, 48);

	zeroCurrPasscodeNum();
	zeroLastCardGenerated();
}

char *getPasscode(mp_int *n) {
	mp_int cipherNum, offset;
	mp_init(&cipherNum);
	mp_init(&offset);
	
	_locate_passcode(n, &cipherNum, &offset);
	           
	/* Get ciphertext block (cipher N, N+1, N+2) 
	 */
	mp_int cipherBlock;
	mp_init(&cipherBlock);
	_compute_passcode_block(&cipherNum, &cipherBlock);
	mp_clear(&cipherNum);
	  
	/* This works on Intel-based Mac OS X, but
	 * it may require tweaking on other platforms
	 * in order to get the endianness correct.
	 */
	union {
		unsigned char bytes[4];
		unsigned int val;
	} ofs;
	ofs.val = 0;
	mp_to_unsigned_bin(&offset, ofs.bytes);
	mp_clear(&offset);

	char *passcode = _extract_passcode_from_block(&cipherBlock, ofs.val);
	mp_clear(&cipherBlock);
	ofs.val = 0;
	
	return passcode;
}

void getPasscodeBlock(mp_int *startingPasscodeNum, int qty, char *output) {
	int i;
	
	mp_int cipherBlock;
	mp_init (&cipherBlock);

	mp_int cipherNum;
	mp_init(&cipherNum);

	mp_int offset;
	mp_init(&offset);

	/* This works on Intel-based Mac OS X, but
	 * it may require tweaking on other platforms
	 * in order to get the endianness correct.
	 */
	union {
		unsigned char bytes[4];
		unsigned int val;
	} ofs;
	ofs.val = 0;

	/* force the initial computation by making lastCipherNum
	 * differ from cipherNum
	 */
	mp_int lastCipherNum;
	mp_init(&lastCipherNum);
	mp_sub_d(&cipherNum, 1, &lastCipherNum);

	mp_int passcodeNum;
	mp_init(&passcodeNum);
	mp_copy(startingPasscodeNum, &passcodeNum);
	for (i=0; i<qty; i++) {
		_locate_passcode(&passcodeNum, &cipherNum, &offset);
		
		if (mp_cmp(&cipherNum, &lastCipherNum) != 0) {
			_compute_passcode_block(&cipherNum, &cipherBlock);
			mp_copy(&cipherNum, &lastCipherNum);
		}
		
		mp_to_unsigned_bin(&offset, ofs.bytes);
		strncpy(output+4*i, _extract_passcode_from_block(&cipherBlock, ofs.val), 4);
		
		mp_add_d(&passcodeNum, 1, &passcodeNum);
	}
}
