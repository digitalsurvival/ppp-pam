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
#include <string.h>

#include "ppp.h"

#include "cmdline.h"
#include "keyfiles.h"

#include "mpi.h"
#include "sha2.h"
#include "print.h"
#include "latex.h"
#include "http.h"

int main( int argc, char * argv[] )
{
	int hasFile;
	pppInit();
	clInit(argv[0]);
	printInit();

	/* get user's sequence key */
	hasFile = readKeyFile(0);

	processCommandLine(argc, argv);


	if ( keyfileExists() && !hasFile && !(fKey || fPassphrase) ) {
		errorExitWithUsage("key file exists, but is invalid. Recreate it with -k option");
	}


	if (fVerbose)
		printf("Verbose output enabled.\n");

	if (fVerbose) {
		printf("PPP Version in use: %d\n", pppVersion());
	}
	
	if (fKey) {
		/* generate and save new key */
		if (fVerbose)
			printf("Generating new random key.\n");
		generateRandomSequenceKey();
		if (fTime) {
			pppSetFlags(PPP_TIME_BASED);
		}
		writeKeyFile();
	}
	
	if (fPassphrase) {
		/* generate a temporary key based on passphrase */
		if (fVerbose)
			printf("Generating a temporary key based on passphrase.\n");
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
		printf("%s key: ", (fPassphrase ? "Temporary" : "User"));
		printKey(seqKey());
		printf("\n");
		if (!fPassphrase) {
			mp_int mp;
			mp_init(&mp);
			mp_add_d(lastCardGenerated(), 1, &mp);
			printf("Last passcard printed: %s\n", mpToDecimalString(&mp, ','));
			mp_clear(&mp);
		}
	}

	mp_int n;
	mp_init(&n);
	mp_zero(&n);
	
	if (fSkip) {
		/* Skip forward in passcode space */
		mp_int newNum;
		mp_init(&newNum);
		calculatePasscodeNumberFromCardColRow(&cardNum, 0, 0, &newNum);
		if (fPasscode && !fPasscodeCurr) {
			calculatePasscodeNumberFromCardColRow(&cardNum, colNum, rowNum, &newNum);
		}
		
		if (fVerbose) {
			mp_int mp;
			mp_init(&mp);
			mp_add_d(currPasscodeNum(), 1, &mp);
			printf("Current passcode number: %s\n", mpToDecimalString(&mp, ','));
			mp_add_d(&newNum, 1, &mp);
			printf("Skipping to passcode number: %s\n", mpToDecimalString(&mp, ','));
			mp_clear(&mp);
		}
		
		if (mp_cmp(&newNum, currPasscodeNum()) <= 0) {
			errorExit("you can only `--skip' forward.");
		}
		
		setCurrPasscodeNum(&newNum);
		calculateCardContainingPasscode(currPasscodeNum(), &newNum);
		if (mp_cmp(lastCardGenerated(), &newNum) < 0) {
			mp_sub_d(&newNum, 1, &newNum);
			setLastCardGenerated(&newNum);
			mp_add_d(&newNum, 1, &newNum);
		}
		
		if (fVerbose) {
			mp_add_d(&newNum, 1, &newNum);
			printf("Card containing passcode: %s\n", mpToDecimalString(&newNum, ','));
		}
		mp_clear(&newNum);
		
		writeState();
	} 
	
	/* Calculate specified passcode number */
	if ( ! fSkip && (fCard || fPasscode) ) {
		calculatePasscodeNumberFromCardColRow(&cardNum, 0, 0, &n);
		if (fPasscode && !fPasscodeCurr) {
			calculatePasscodeNumberFromCardColRow(&cardNum, colNum, rowNum, &n);
		}
		// if (fVerbose) {
		// 	mp_int mp;
		// 	mp_init(&mp);
		// 	mp_add_d(&cardNum, 1, &mp);
		// 	printf("Passcard number %s\n", mpToDecimalString(&mp, ','));
		// 	printf("Column %c\n", colNum + 'A');
		// 	printf("Row %d\n", rowNum+1);
		// 	mp_add_d(&n, 1, &mp);
		// 	printf("Passcode number %s\n", mpToDecimalString(&mp, ','));
		// 	mp_clear(&mp);
		// }
	} 
	
	if ( ! fPasscode && fVerbose) {
		printf("Current passcode: %s\n", currCode());
		mp_int mp;
		mp_init(&mp);
		getNumPrintedCodesRemaining(&mp);
		printf("Printed passcodes remaining: %s\n", mpToDecimalString(&mp, ','));
		mp_clear(&mp);
	}
	
	/* Print cards or individual passcode */
	if (fText || fLatex) {
		if (fNext) {
			int i;
			for (i=0; i<numCards; i++) {
				mp_int mp;
				mp_init(&mp);
				if ( ! fPassphrase ) {
					mp_add_d(lastCardGenerated(), 1, &mp);
					if (fLatex)
						latexCard(&mp);
					else
						printCard(&mp);
					/* Keep track of last card printed with --next if 
					 * user's key was used.
					 */
					incrLastCardGenerated();
					writeState();
				} else {
					if (fLatex)
						latexCard(&cardNum);
					else
						printCard(&cardNum);
				}
				mp_add_d(&cardNum, 1, &cardNum);
				mp_clear(&mp);
			}
		} else {
			if (fPasscode) {
				if (fPasscodeCurr) {
					printf("%s: %s\n", currCode(), getPasscode(currPasscodeNum()));
				} else {
					printf("%s\n", getPasscode(&n));
				}
			} else {
				if (fLatex)
					latexCard(&cardNum);
				else
					printCard(&cardNum);
			}
		}
	}
	
	if (fHtml) {
		httpServe();
	}
	
	if (fTime && !fKey) {
		printf("%s\n", getPasscode(NULL));
	}	
	
	/* display any warnings */
	char buffer[2048];
	if (!fKey && !fPassphrase && !fLatex && !fPasscodeCurr) {
		while (pppWarning(buffer, 2048)) {
			if (strlen(buffer)) {
				fprintf(stderr, "%s\n", buffer);
			}
		}
	}
	
	/* cleanup , zero memory, etc */
	mp_clear(&n);
	clCleanup();
	pppCleanup();
	printCleanup();
	
	return 0;
}
