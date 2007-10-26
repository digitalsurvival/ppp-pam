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
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "cmdline.h"
#include "keyfiles.h"

int fKey = 0;
int fSkip = 0;
int fHtml = 0;
int fText = 0;
int fNext = 0;
int fName = 0;
int fCard = 0;
int fPassphrase = 0;
int fPasscode = 0;
int fVerbose = 0;
int numCards = 0;
static char passphrase[1024] = "";
static char passcode[1024] = "";
static char hname[40] = "";
static char *pn = NULL;


mp_int cardNum;
int rowNum = 0;
int colNum = 0;

void clInit(char *argv0) {
	/* initialize the program name */
	pn = strrchr(argv0, '/');
	if (pn) {
		pn++;
	} else {
		pn= argv0;
	}
	
	mp_init(&cardNum);
}

char *progname() {
	return pn;
}

char *hostname() {
	if ( ! fName) {
		gethostname(hname, 38);
	}
	
	return hname;
}

void errorExitWithUsage(char *msg) {
	fprintf(stderr, "%s: %s\n", progname(), msg);
	usage();
	exit(-1);
}

void errorExit(char *msg) {
	fprintf(stderr, "%s: %s\n", progname(), msg);
	exit(-1);
}

void errorMessage(char *msg) {
	fprintf(stderr, "%s: %s\n", progname(), msg);
}

void usage() {
	fprintf(stderr, 
		"Usage: %s [options]\n"
		"Options:\n"
		"  -k, --key          Generate a new sequence key and save in ~/.pppauth.\n"
		"  -s, --skip         Skip to --passcode or to --card specified.\n"
		// "  -h, --html         Generate html passcards for printing.\n"
		"  -t, --text         Generate text passcards for printing.\n"
		"  -m, --name <name>  Specify hostname to use for printing passcards.\n"
		"  --next <num>       Generate next <num> consecutive passcards from current\n"
		"                     active passcode for printing.\n"
		"  -c, --card <num>   Specify number of passcard to --skip to or print.\n"
		"  -p, --passcode <NNNNCRR>\n"
		"                     Specify a single passcode identifier to --skip to or print.\n"
		"                     Where: NNNN is the decimal integer passcard number, C is\n"
		"                     the column (A through G), and RR is the row (1 through 10).\n"
		"  --passphrase <phrase>\n"
		"                     Use the specified <phrase> to create a temporary key for\n"
		"                     testing purposes only.  This temporary key is not saved\n"
		"                     and will only be used until the program exits.\n"
		"  -v, --verbose      Display more information about what is happening.\n"
		, progname()
	);
}

int isDecimal(char *str) {
	int i;
	for (i=0; i<strlen(str); i++) {
		if ( ! isdigit(str[i]) ) {
			return 0;
		}
	}
	return 1;
}

int validNumCards(char *str, int length) {
	if ( ! isDecimal(str) ) {
		return 0;
	}
	numCards = atoi(str);
	return 1;
}

int validCardNum(char *str, int length) {
	char data[1024];
	if (length > 1023) {
		return 0;
	}
	strncpy(data, str, length);
	data[length] = '\x00';
	
	if ( ! isDecimal(data) ) {
		return 0;
	}
	mp_read_radix(&cardNum, (unsigned char *)data, 10);
	
	if (mp_cmp_d(&cardNum, 1) < 0) {
		return 0;
	}
	mp_sub_d(&cardNum, 1, &cardNum); /* make zero-based */
	return 1;
}

int validColLetter(char *str, int length) {
	if (length > 1 || toupper(str[0]) > 'G' || toupper(str[0]) < 'A') {
		return 0;
	}
	colNum = toupper(str[0]) - 'A'; /* make zero-based */
	return 1;
}

int validRowNum(char *str, int length) {
	if ( ! isDecimal(str) ) {
		return 0;
	}
	rowNum = atoi(str);
	if (rowNum < 1 || rowNum > 10) {
		return 0;
	}
	rowNum--; /* make zero-based */
	return 1;
}

int validPasscode(char *str, int length) {
	int c = strspn(str, "0123456789");
	if (c < 1) {
		return 0;
	}
	if (c >= strlen(str)) {
		return 0;
	}
	if (validCardNum(str, c) == 0) {
		return 0;
	}
	int d = strspn(str+c, "abcdefgABCDEFG");
	if (d < 1) {
		return 0;
	}
	if (c+d > strlen(str)) {
		return 0;
	}
	if (validColLetter(str+c, d) == 0) {
		return 0;
	}
	
	if (validRowNum(passcode+c+d, strlen(passcode+c+d)) == 0) {
		return 0;
	}
	return 1;
}

void processCommandLine( int argc, char * argv[] )
{
	int c;
	
	static struct option long_options[] = { 
		{"key",			no_argument,		0, 'k'},
		{"skip",   		no_argument,		0, 's'},
		// {"html",		no_argument,		0, 'h'},
		{"text",		no_argument,		0, 't'},
		{"next",		no_argument,		&fNext, 1},
		{"name",		required_argument,	0, 'm'},
		{"card",		required_argument,	0, 'c'},
		{"passphrase",	required_argument,	&fPassphrase, 1},
		{"passcode",	required_argument,	0, 'p'},
		{"verbose",		no_argument, 		0, 'v'},
		{0, 0, 0, 0}
	};

    while (1) {
		/* getopt_long stores the option index here. */
		int option_index = 0;
		c = getopt_long_only(argc, argv, "kstm:c:p:v", long_options, &option_index);
  
		/* Detect the end of the options. */
		if (c == -1)
			break;
  
		switch (c) {
			case 0:
				if (strcmp(long_options[option_index].name, "passphrase") == 0) {
					strncpy(passphrase, optarg, 1023);
					passphrase[1023] = '\x00';
				}
				if (strcmp(long_options[option_index].name, "next") == 0) {
					/* fNext is already set, so do nothing */
				}
				break;
  
			case 'k':
				fKey = 1;
				break;
			case 's':
				fSkip = 1;
				break;
			case 'h':
				fHtml = 1;
				break;
			case 't':
				fText = 1;
				break;
			case 'm':
				fName = 1;
				strncpy(hname, optarg, 39);
				break;
			case 'c':
				fCard = 1;
				if (validCardNum(optarg, strlen(optarg)) == 0) {
					errorExitWithUsage("invalid card number specified");
				}
				break;
			case 'p':
				fPasscode = 1;
				strncpy(passcode, optarg, 1023);
				passcode[1023] = '\x00';
				break;
			case 'v':
				fVerbose = 1;
				break;
				
			case '?':
				/* getopt_long already printed an error message. */
				usage();
				exit(-1);
				break;
  
			default:
				abort();
		}
	}	
	
	if ( fNext ) {
		if (argc == optind) {
			/* default of next 1 cards */
			numCards = 1;
		}
		if ((argc - optind) == 1 ) {
			if (validNumCards(argv[optind], strlen(argv[optind])) == 0) {
				errorExitWithUsage("invalid number of cards specified");
			}
			optind++;
		}
	}
	
	if (argc > optind) {
		errorExitWithUsage("unrecognized options on command line");
	}
   
	
	/* validate the command line options */
	
	if ( ! (fKey | fSkip | fHtml | fText) ) {
		errorExitWithUsage("nothing to do!");
	}

	if (fPasscode && fCard) {
		errorExitWithUsage("cannot specify `--passcode' and `--card' together");
	} 

	if (fHtml && fText) {
		errorExitWithUsage("cannot specify `--html' and `--text' together");
	}
	
	if (fSkip && !(fPasscode || fCard)) {
		errorExitWithUsage("must specify a passcode to `--skip' to with -p or -n,-r,-c.");
	} 
	
	if ( (fHtml || fText) && !(fNext || fCard || fPasscode) ) {
		errorExitWithUsage("must specify which card(s) to generate with `--next' or `--card' or `--passcode'");
	}
	
	if ( ! (keyfileExists() || fKey || fPassphrase) ) {
		errorExitWithUsage("must create a sequence key with `--key' or use a `--passphrase'");
	}

	if (fPasscode) {
		if (validPasscode(passcode, strlen(passcode)) == 0) {
			errorExitWithUsage("invalid passcode specified");
		}
	}
	
	if (fPassphrase && fNext) {
		errorExitWithUsage("Cannot use `--next' with `--passphrase'");
	}

	if (fNext && fCard) {
		errorExitWithUsage("Cannot use `--next' with `--card'");
	}
}

char *getPassphrase() {
	return passphrase;
}

void clCleanup() {
	mp_clear(&cardNum);
}

