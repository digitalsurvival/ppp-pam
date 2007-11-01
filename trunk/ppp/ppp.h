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

#ifndef _PPP_H_
#define _PPP_H_

#include "sha2.h"
#include "rijndael.h"
#include "mpi.h"

#include "print.h"
#include "keyfiles.h"
             
#define PPP_FLAGS_PRESENT			0x0001
#define PPP_DONT_SKIP_ON_FAILURES	0x0002
#define PPP_TIME_BASED				0x0004
    
void pppInit();
void pppCleanup();
char * mpToDecimalString(mp_int *mp, char groupChar);
char *currCode();
char *currPrompt();
int pppAuthenticate(char *attempt);
mp_int *seqKey();
void setSeqKey(mp_int *mp);
mp_int *currPasscodeNum();
void setCurrPasscodeNum(mp_int *mp);
void zeroCurrPasscodeNum();
void incrCurrPasscodeNum();
mp_int *lastCardGenerated();
void setLastCardGenerated(mp_int *mp);
void zeroLastCardGenerated();
void incrLastCardGenerated();
void calculatePasscodeNumberFromCardColRow(mp_int *card, int col, int row, mp_int *passcodeNum);
void generateSequenceKeyFromPassphrase(char *phrase);
void generateRandomSequenceKey();
char *getPasscode(mp_int *n);
void getPasscodeBlock(mp_int *startingPasscodeNum, int qty, char *output);
void calculateCardContainingPasscode(mp_int *passcodeNum, mp_int *cardNum);
void getNumPrintedCodesRemaining(mp_int *mp);
int pppVersion();
void setKeyVersion(int v);
int keyVersion();
void pppSetFlags(unsigned int mask);
void pppClearFlags(unsigned int mask);
unsigned int pppCheckFlags(unsigned int mask);

#endif
