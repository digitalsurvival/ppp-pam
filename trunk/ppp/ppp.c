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

#define KEY_BITS (int)256

/* IMPORTANT NOTE
 *
 * If you update the PPP algorithm in any way, it's important
 * to remain backwards compatibe with older algorithms where
 * people may be relying on them to log into machines.
 *
 * Set _ppp_ver to the latest version of the algorithm this
 * code can handle, but be sure to never break the handling
 * of older versions in here.
 *
 * When an older keyfile is loaded, _key_ver will contain the
 * version number of the PPP algorithm used to create it.  All
 * processing in here should use the version specified in
 * _key_ver.  In the event that _ppp_ver != _key_ver, it's
 * appropriate that the application advise the user to generate
 * a new key and print passcards using the updated algorithm.
 */

/* latest PPP algorithm version that's supported by this code */
static int _ppp_ver = 2;

/* PPP version that the keyfile was created in */
static int _key_ver = 0;

/* Flags that permit customizing the implementation */
static unsigned int _ppp_flags = 0;

static const char * default_alphabet = "23456789!@#%+=:?abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPRSTUVWXYZ";
static char * alphabet = NULL;
static int alphabetlen = 0;

static unsigned long rk[RKLENGTH(KEY_BITS)];
static int nRounds = 0;

static mp_int d_seqKey;
static mp_int d_currPasscodeNum;
static mp_int d_reservedPasscodeNum;
static char d_reserved;

static mp_int d_lastCardGenerated;
static mp_int d_maxPasscodes;
static char d_passcode[5] = "";
static char *d_prompt = NULL;
static int d_prompt_len = 0;
static char *d_code = NULL;
static int d_code_len = 0;
static char *d_buf = NULL;
static int d_buflen = 0;

typedef union {
	unsigned char bytes[4];
	unsigned int val;
} utype;

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

static void _mp_to_uint(mp_int *mp, unsigned int *i) {
	utype tell, v;

	tell.val = 0x4d494d49;
	v.val = 0;

	mp_to_unsigned_bin(mp, v.bytes);

	/* We can't just use htonl() here because it swaps bytes
	 * on intel (little-endian) platforms and it's motorolla
	 * (big-endian) platforms where we want to swap bytes.
	 */
	if (tell.bytes[0] == 'M') {
		_reverse_bytes(v.bytes, 4);
	}

	*i = v.val;

	v.val = 0;
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

	unsigned char *kp = seqkey;
	if (keyVersion() == 2) {
		kp += 16;
	}

	/* get encryption key from sequence key (32 MSBs) */
	mp_int key;
	mp_init(&key);
	mp_read_unsigned_bin(&key, kp, 32);

	/* prepare for encryption */
	_setup_encrypt(&key);
	mp_clear(&key);

	/* get offset from sequence key (16 LSBs) */
	mp_int offset;
	mp_init(&offset);
	switch (keyVersion()) {
	case 1:
		mp_read_unsigned_bin(&offset, seqkey+32, 16);
		break;
	case 2:
		/* version 2 does away with the offset */
		mp_zero(&offset);
		break;
	default:
		/* unsupported */
		break;
	}
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
	d_reserved = 0;
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
	_zero_bytes((unsigned char *)d_code, d_code_len);
	free(d_code);
	_zero_bytes((unsigned char *)d_buf, d_buflen);
	free(d_buf);
	_zero_bytes((unsigned char *)alphabet, alphabetlen);
	free(alphabet);
}

char *mpToDecimalString(mp_int *mp, char groupChar) {
	int len = mp_radix_size(mp, 10);
	int nCommas = 0;

	_zero_bytes((unsigned char *)d_buf, d_buflen);
	free(d_buf);
	d_buflen = len + len/3 + 2;
	d_buf = (char *)malloc(d_buflen);
	mp_toradix(mp, (unsigned char *)d_buf, 10);

	len = strlen(d_buf);

	if (groupChar) {
		int nGroups = len / 3;
		int firstGroupLen = len % 3;
		nCommas = nGroups;
		if (firstGroupLen == 0) {
			nCommas--;
		}
	}

	if (nCommas > 0) {
		int i, pos;
		int offset = nCommas;
		d_buf[len+offset] = '\x00';
		for (i=0; i<len; i++) {
			pos = len - 1 - i;
			if (i>0 && i%3==0) {
				d_buf[pos+offset] = ',';
				offset--;
			}
			d_buf[pos+offset] = d_buf[pos];
		}
	}

	return d_buf;
}

char *currCode() {
	mp_int mp;
	mp_int row;
	unsigned int c, r;

	mp_init(&mp);
	mp_init(&row);

	calculateCardContainingPasscode(currAuthPasscodeNum(), &mp);
	mp_add_d(&mp, 1, &mp);
	char *cardstr = mpToDecimalString(&mp, ',');
	mp_sub_d(&mp, 1, &mp);

	mp_mul_d(&mp, 70, &row);
	mp_sub(currAuthPasscodeNum(), &row, &row);
	mp_set_int(&mp, 7);
	mp_div(&row, &mp, &row, &mp);
	_mp_to_uint(&row, &r);
	_mp_to_uint(&mp, &c);
	mp_clear(&mp);
	mp_clear(&row);

	free(d_code);
	d_code = (char *)malloc(strlen("[]") + strlen(cardstr) + 6);
	sprintf(d_code, "%d%c [%s]",++r, c+'A', cardstr);

	mp_clear(&row);
	c = r = 0;

	return d_code;
}

char *currPrompt() {
	int length = strlen("Passcode : ") + strlen(currCode()) + 6 + 4;
	if (lockingFailed)
		length += strlen("(no lock) ");
	free(d_prompt);
	d_prompt = (char *)malloc(length);
	if (lockingFailed)
		sprintf(d_prompt, "(no lock) Passcode %s: ", currCode());
	else
		sprintf(d_prompt, "Passcode %s: ", currCode());
	return d_prompt;
}


int pppAuthenticate(const char *attempt) {
	int rv = 0;

	if (strcmp(getPasscode(currAuthPasscodeNum()), attempt) == 0) {
		rv = 1;
		if (!d_reserved) {
			/* Increment now, wasn't incremented before */
			incrCurrPasscodeNum();
			writeState();
		}
	} else {
		if ( ! pppCheckFlags(PPP_DONT_SKIP_ON_FAILURES)) {
			if (!d_reserved) {
				/* Increment now */
				incrCurrPasscodeNum();
				writeState();
			}
		} else {
			if (d_reserved) {
				/* Was reserved, but failed and should be decreased... */
				/* FIXME: This shouldn't really be used because it causes a security
				 * bug similar to not using reservation at all. *
				 * UPDATE: This will never happen, as pam module doesn't reserve
				 * passwords if dontSkip is enabled */
				decrCurrPasscodeNum();
				writeState();
			}
		}
	}
	d_reserved = 0;

	_zero_bytes((unsigned char *)d_passcode, 5);

	return rv;
}

int pppWarning(char *buf, int size) {
	static int warnNum = 0;
	mp_int mp;
	mp_init(&mp);

	buf[0] = '\x00';

	switch (warnNum) {
	case 0:
		getNumPrintedCodesRemaining(&mp);
		if (mp_cmp_d(&mp, 70) <= 0 && mp_cmp_d(&mp, 14) > 0) {
			snprintf(buf, size, "\n"
				"===========================================================\n"
				"  You are on your last printed passcard. Please print\n"
				"  more so you can continue to log into your account.\n"
				"===========================================================\n"
			);
		}
		break;
	case 1:
		getNumPrintedCodesRemaining(&mp);
		if (mp_cmp_d(&mp, 14) <= 0 && mp_cmp_d(&mp, 0) > 0) {
			snprintf(buf, size, "\n"
				"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
				"  You have %s printed passcode%s remaining. Please print\n"
				"  more passcodes IMMEDIATELY so you can continue to log\n"
				"  into your account.\n"
				"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n",
				mpToDecimalString(&mp, 0), (mp_cmp_d(&mp, 1) ? "s":"")
			);
		}
		break;
	case 2:
		getNumPrintedCodesRemaining(&mp);
		if (mp_cmp_d(&mp, 0) <= 0) {
			snprintf(buf, size, "\n"
				"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
				"            WARNING:  YOU ARE OUT OF PASSCODES             \n"
				"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
				"  YOU MUST IMMEDIATELY PRINT MORE PASSCARDS if you wish to \n"
				"  log in again.\n"
				"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
			);
		}
		break;
	case 3:
		if (pppVersion() > keyVersion()) {
			snprintf(buf, size, "\n"
				"===========================================================\n"
				"            NOTICE:  NEW PPP VERSION AVAILABLE             \n"
				"\n"
				"  Version %d of the PPP algorithm is now supported.\n"
				"\n"
				"  It is recommended that you upgrade to the new version by\n"
				"  generating a new random key and printing new passcodes.\n"
				"===========================================================\n",
				pppVersion()
			);
		}
		break;
	default:
		warnNum = -1;
		break;
	}

	mp_clear(&mp);
	return ++warnNum;
}

mp_int *seqKey() {
	return &d_seqKey;
}

void setSeqKey(mp_int *mp) {
	mp_copy(mp, &d_seqKey);
}

mp_int *currAuthPasscodeNum() {
	/* Return passcode which must be used for authentication */
	if (d_reserved) {
		return &d_reservedPasscodeNum;
	} else {
		return &d_currPasscodeNum;
	}
}

mp_int *currPasscodeNum() {
	/* Return passcode */
	return &d_currPasscodeNum;
}


void setCurrPasscodeNum(const mp_int *mp) {
	mp_copy(mp, &d_currPasscodeNum);
}

void zeroCurrPasscodeNum() {
	mp_zero(&d_currPasscodeNum);
}

void incrCurrPasscodeNum() {
	mp_add_d(&d_currPasscodeNum, 1, &d_currPasscodeNum);
}

void decrCurrPasscodeNum() {
	mp_sub_d(&d_currPasscodeNum, 1, &d_currPasscodeNum);
}


void reservePasscodeNum(void) {
	mp_copy(&d_currPasscodeNum, &d_reservedPasscodeNum);
	d_reserved = 1;

	/* Increment num, so parallel sessions won't reserve the same passCode */
	/* FIXME: Races should be fixed by some lock file */
	incrCurrPasscodeNum();
	writeState();
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


void generateSequenceKeyFromPassphrase(const char *phrase) {
	unsigned char bytes[48];

	/* use the current ppp version */
	setKeyVersion(pppVersion());

	switch (keyVersion()) {
	case 1:
		sha384((const unsigned char *)phrase, strlen(phrase), bytes);
		_reverse_bytes(bytes, 48);
		_bytes_to_mp(bytes, &d_seqKey, 48);
		_zero_bytes(bytes, 48);
		break;
	case 2:
		sha256((const unsigned char *)phrase, strlen(phrase), bytes);
		_reverse_bytes(bytes, 32);
		_bytes_to_mp(bytes, &d_seqKey, 32);
		_zero_bytes(bytes, 32);
		break;
	default:
		/* unsupported */
		break;
	}

	pppSetFlags(PPP_FLAGS_PRESENT);
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

	/* use the current ppp version */
	setKeyVersion(pppVersion());

	switch (keyVersion()) {
	case 1:
		sha384(entropy, 32, bytes);
		_zero_bytes(entropy, 32);
		_reverse_bytes(bytes, 48);
		_bytes_to_mp(bytes, &d_seqKey, 48);
		_zero_bytes(bytes, 48);
		break;
	case 2:
		sha256(entropy, 32, bytes);
		_zero_bytes(entropy, 32);
		_reverse_bytes(bytes, 32);
		_bytes_to_mp(bytes, &d_seqKey, 32);
		_zero_bytes(bytes, 32);
	break;
	default:
		/* unsupported */
		break;
	}

	pppSetFlags(PPP_FLAGS_PRESENT);
	zeroCurrPasscodeNum();
	zeroLastCardGenerated();
}

char *getPasscode(const mp_int *n) {
	unsigned int ofs = 0;
	mp_int cipherNum, offset;
	mp_init(&cipherNum);
	mp_init(&offset);

	mp_int N;
	mp_init(&N);

	if (pppCheckFlags(PPP_TIME_BASED)) {
		/* Experimental time-based passcodes */
		mp_set_int(&N, time(NULL));
		mp_div_2d(&N, 5, &N, NULL);  // divide by 32 or right shift 5 bits
	} else {
		mp_copy(n, &N);
	}

	_locate_passcode(&N, &cipherNum, &offset);
	mp_clear(&N);

	/* Get ciphertext block (cipher N, N+1, N+2)
	 */
	mp_int cipherBlock;
	mp_init(&cipherBlock);
	_compute_passcode_block(&cipherNum, &cipherBlock);
	mp_clear(&cipherNum);

	_mp_to_uint(&offset, &ofs);
	mp_clear(&offset);

	char *passcode = _extract_passcode_from_block(&cipherBlock, ofs);
	mp_clear(&cipherBlock);
	ofs = 0;

	return passcode;
}

void getPasscodeBlock(mp_int *startingPasscodeNum, int qty, char *output) {
	int i;

	mp_int cipherBlock;
	mp_init (&cipherBlock);

	mp_int cipherNum;
	mp_init(&cipherNum);

	unsigned int ofs = 0;
	mp_int offset;
	mp_init(&offset);

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

		_mp_to_uint(&offset, &ofs);
		strncpy(output+4*i, _extract_passcode_from_block(&cipherBlock, ofs), 4);

		mp_add_d(&passcodeNum, 1, &passcodeNum);
	}

	ofs = 0;
}

void getNumPrintedCodesRemaining(mp_int *mp) {
	mp_int last;
	mp_init(&last);

	mp_int *curr = currPasscodeNum();
	mp_add_d(lastCardGenerated(), 1 ,&last);
	mp_mul_d(&last, 70 ,&last);
	mp_sub(&last, curr, &last);

	mp_copy(&last, mp);
	mp_clear(&last);
}

int pppVersion() {
	return _ppp_ver;
}

void useVersion(int v) {
	_ppp_ver = v;
}

int cmp(const void *vp, const void *vq) {
	const char *p = vp;
	const char *q = vq;
	return (*p - *q);
}

void setPasscodeAlphabet(const char *a) {
	/* sorted user-specified alphabet */
	alphabetlen = strlen(a);
	free(alphabet);
	alphabet = (char *)malloc(alphabetlen+1);
	strncpy(alphabet, a, alphabetlen+1);
	qsort(alphabet, alphabetlen, sizeof(char), cmp);
}

void setKeyVersion(int v) {
	_key_ver = v;

	// set alphabet based on version
	switch (keyVersion()) {
	case 3:
		/* sort default alphabet */
		alphabetlen = strlen(default_alphabet);
		free(alphabet);
		alphabet = (char *)malloc(alphabetlen+1);
		strncpy(alphabet, default_alphabet, alphabetlen+1);
		qsort(alphabet, strlen(alphabet), sizeof(char), cmp);
		break;
	default:
		/* unsorted default alphabet */
		alphabetlen = strlen(default_alphabet);
		free(alphabet);
		alphabet = (char *)malloc(alphabetlen+1);
		strncpy(alphabet, default_alphabet, alphabetlen+1);
		break;
	}
}

int keyVersion() {
	return _key_ver;
}

void pppSetFlags(unsigned int mask) {
	_ppp_flags |= mask;
}

void pppClearFlags(unsigned int mask) {
	_ppp_flags &= ~mask;
}

unsigned int pppCheckFlags(unsigned int mask) {
	return _ppp_flags & mask;
}
