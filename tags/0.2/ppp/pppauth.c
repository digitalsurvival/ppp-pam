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

#include <stdio.h>

#include "cmdline.h"
#include "keyfiles.h"

#include "mpi.h"
#include "sha2.h"
#include "ppp.h"
#include "print.h"
#include "http.h"

int main( int argc, char * argv[] )
{
	pppInit();
	clInit(argv[0]);
	printInit();
	
	processCommandLine(argc, argv);

	if (fVerbose)
		printf("Verbose output enabled.\n");
	
	/* get user's sequence key */
	readKeyFile();
	
	if (fKey) {
		/* generate and save new key */
		if (fVerbose)
			printf("Generating new random sequence key.\n");
		generateRandomSequenceKey();
		writeKeyFile();
	}
	
	if (fPassphrase) {
		/* generate a temporary key based on passphrase */
		if (fVerbose)
			printf("Generating a temporary sequence key based on passphrase.\n");
		generateSequenceKeyFromPassphrase(getPassphrase());
		 
		/* TODO: Delete this.
		 * This call to writeKeyFile() is used to save a sequence key 
		 * based on a known plaintext passphrase as the user's key.  
		 * It is useful for testing purposes but is dangerous to leave 
		 * enabled since it can reduce the security of the secret key
		 * substantially.
		 */
		// writeKeyFile();
	}                     
	
	if (fVerbose) {
		printf("%s sequence key: ", (fPassphrase ? "Temporary" : "User"));
		printKey(seqKey());
		printf("\n");
		mp_int mp;
		mp_init(&mp);
		mp_add_d(lastCardGenerated(), 1, &mp);
		printf("Last passcard printed: %s\n", mpToDecimalString(&mp, ','));
		mp_clear(&mp);
	}
	    
	mp_int n;
	mp_init(&n);
	mp_zero(&n);
	
	if (fSkip) {
		/* Skip forward in passcode space */
		mp_int new;
		mp_init(&new);
		calculatePasscodeNumberFromCardColRow(&cardNum, 0, 0, &new);
		if (fPasscode) {
			calculatePasscodeNumberFromCardColRow(&cardNum, colNum, rowNum, &new);
		}
		
		if (fVerbose) {
			mp_int mp;
			mp_init(&mp);
			mp_add_d(currPasscodeNum(), 1, &mp);
			printf("Current passcode number: %s\n", mpToDecimalString(&mp, ','));
			mp_add_d(&new, 1, &mp);
			printf("Skipping to passcode number: %s\n", mpToDecimalString(&mp, ','));
			mp_clear(&mp);
		}
		
		if (mp_cmp(&new, currPasscodeNum()) <= 0) {
			errorExit("you can only `--skip' forward.");
		}
		
		setCurrPasscodeNum(&new);
		calculateCardContainingPasscode(currPasscodeNum(), &new);
		if (mp_cmp(lastCardGenerated(), &new) < 0) {
			mp_sub_d(&new, 1, &new);
			setLastCardGenerated(&new);
			mp_add_d(&new, 1, &new);
		}
		
		if (fVerbose) {
			mp_add_d(&new, 1, &new);
			printf("Card containing passcode: %s\n", mpToDecimalString(&new, ','));
		}
		mp_clear(&new);
		
		writeState();
	} 
	
	/* Calculate specified passcode number */
	if ( ! fSkip && (fCard || fPasscode) ) {
		calculatePasscodeNumberFromCardColRow(&cardNum, 0, 0, &n);
		if (fPasscode) {
			calculatePasscodeNumberFromCardColRow(&cardNum, colNum, rowNum, &n);
		}
		if (fVerbose) {
			mp_int mp;
			mp_init(&mp);
			mp_add_d(&cardNum, 1, &mp);
			printf("Passcard number %s\n", mpToDecimalString(&mp, ','));
			printf("Column %c\n", colNum + 'A');
			printf("Row %d\n", rowNum+1);
			mp_add_d(&n, 1, &mp);
			printf("Passcode number %s\n", mpToDecimalString(&mp, ','));
			mp_clear(&mp);
		}
	}
	
	/* Print cards or individual passcode */
	if (fText) {
		if (fNext) {
			int i;
			for (i=0; i<numCards; i++) {
				mp_int mp;
				mp_init(&mp);
				mp_add_d(lastCardGenerated(), 1, &mp);
				printCard(&mp);
				if ( ! fPassphrase ) {
					/* Keep track of last card printed with --next if 
					 * user's key was used.
					 */
					incrLastCardGenerated();
					writeState();
				}
				mp_add_d(&cardNum, 1, &cardNum);
				mp_clear(&mp);
			}
		} else {
			if (fPasscode) {
				printf("%s\n", getPasscode(&n));
			} else {
				printCard(&cardNum);
			}
		}
	}
	
	if (fHtml) {
		httpServe();
	}
	
	             
	/* cleanup , zero memory, etc */
	mp_clear(&n);
	clCleanup();
	pppCleanup();
	printCleanup();
	
	return 0;
}
