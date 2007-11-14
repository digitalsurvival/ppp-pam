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
#include <unistd.h>

#include "ppp.h"

#include "cmdline.h"
#include "keyfiles.h"

int fKey = 0;
int fTime = 0;
int fSkip = 0;
int fHtml = 0;
int fText = 0;
int fNext = 0;
int fAlphabet = 0;
int fName = 0;
int fCard = 0;
int fDontSkipFailures = 0;
int fShowPasscode = 0;
int fPassphrase = 0;
int fPasscode = 0;
int fVerbose = 0;
int fUseVersion = 0;
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
		"  -a, --alphabet <string>\n"
		"                     Optionally used with --key to specify a character set\n"
		"                     used for passcodes.\n"
		// "  --time             Specify the use of time-varying passcodes.\n"
		"  -s, --skip         Skip to --passcode or to --card specified.\n"
		"  -h, --html         Generate html passcards for printing.\n"
		"  -t, --text         Generate text passcards for printing.\n"
		"  -m, --name <name>  Specify hostname to use for printing passcards.\n"
		"  --next <num>       Generate next <num> consecutive passcards from current\n"
		"                     active passcode for printing.\n"
		"  -c, --card <num>   Specify number of passcard to --skip to or print.\n"
		"  -p, --passcode <RRC[NNNN]>\n"
		"                     Specify a single passcode identifier to --skip to or print.\n"
		"                     Where: NNNN is the decimal integer passcard number, C is\n"
		"                     the column (A through G), and RR is the row (1 through 10).\n"
		"                     Square brackets around NNNN and comma separators are optional.\n"
		"  --passphrase <phrase>\n"
		"                     Use the specified <phrase> to create a temporary key for\n"
		"                     testing purposes only.  This temporary key is not saved\n"
		"                     and will only be used until the program exits.\n"
		"  --dontSkip         Used with --key to specify that on authentication, system\n"
		"                     will not advance to the next passcode on a failed attempt.\n"
		"  --showPasscode     Used with --key to specify that on authentication, system\n"
		"                     will display passcode as it is typed.\n"
		"  -v, --verbose      Display more information about what is happening.\n"
		/* -u, --useVersion <N>              UNDOCUMENT feature used only for testing */
		, progname()
	);
}

int isDecimal(char *str, int len) {
	int i;
	for (i=0; i<len; i++) {
		if ( ! isdigit(str[i]) ) {
			return 0;
		}
	}
	return 1;
}

int validNumCards(char *str, int length) {
	if ( ! isDecimal(str, length) ) {
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
	
	if ( ! isDecimal(data, strlen(data)) ) {
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
	if ( ! isDecimal(str, length) ) {
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
	if (validRowNum(str, c) == 0) {
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
	
	if (validCardNum(passcode+c+d, strlen(passcode+c+d)) == 0) {
		return 0;
	}
	return 1;
}

void processCommandLine( int argc, char * argv[] )
{
	int c, i, j;
	
	static struct option long_options[] = { 
		{"key",			no_argument,		0, 'k'},
		{"time",		no_argument,		&fTime, 1},
		{"skip",   		no_argument,		0, 's'},
		{"html",		no_argument,		0, 'h'},
		{"text",		no_argument,		0, 't'},
		{"next",		no_argument,		&fNext, 1},
		{"alphabet",	required_argument,	0, 'a'},
		{"name",		required_argument,	0, 'm'},
		{"card",		required_argument,	0, 'c'},
		{"passphrase",	required_argument,	&fPassphrase, 1},
		{"passcode",	required_argument,	0, 'p'},
		{"dontskip",	no_argument,		&fDontSkipFailures, 1},
		{"dontSkip",	no_argument,		&fDontSkipFailures, 1},
		{"showpasscode",no_argument,		&fShowPasscode, 1},
		{"showPasscode",no_argument,		&fShowPasscode, 1},
		{"verbose",		no_argument, 		0, 'v'},
		{"useVersion",	required_argument,	0, 'u'},
		{0, 0, 0, 0}
	};

    while (1) {
		/* getopt_long stores the option index here. */
		int option_index = 0;
		c = getopt_long_only(argc, argv, "kshta:m:c:p:vu:", long_options, &option_index);
  
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
			case 'a':
				setPasscodeAlphabet(optarg);
				fAlphabet = 1;
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
				i = j = 0;
				while( optarg[i] != '\x00' ) {
					switch( optarg[i] ) {
						case ',' : 
						case '[' :
						case ']' :
						case ' ' :
							i++;
							break;
						default:
							passcode[j++] = optarg[i++];
							break;
					}
					passcode[j] = '\x00';
				}
				break;
			case 'v':
				fVerbose = 1;
				break;
			case 'u':
				fUseVersion = 1;
				useVersion(atoi(optarg));
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
   
	/* process ppp flags */
    
	if (pppCheckFlags(PPP_TIME_BASED) && !(fKey || fPassphrase) ) {
		fTime = 1;
	}
	
	if (fTime) {
		pppSetFlags(PPP_TIME_BASED);
	} else {
		pppClearFlags(PPP_TIME_BASED);
	}
	
	if (fDontSkipFailures) {
		pppSetFlags(PPP_DONT_SKIP_ON_FAILURES);
	} else {
		pppClearFlags(PPP_DONT_SKIP_ON_FAILURES);
	}

	if (fShowPasscode) {
		pppSetFlags(PPP_SHOW_PASSCODE);
	} else {
		pppClearFlags(PPP_SHOW_PASSCODE);
	}
	
	/* Disable time-based authentication for now
	 * since it's still experimental and has not yet
	 * been fully implemented.  Comment out the following
	 * two lines to enable it.
	 */
	pppClearFlags(PPP_TIME_BASED);
	fTime = 0;
	
    /* validate the command line options */

    if ( ! (fKey | fSkip | fHtml | fText | fTime) ) {
		errorExitWithUsage("nothing to do!");
	}

	if (fPasscode && fCard) {
		errorExitWithUsage("cannot specify `--passcode' and `--card' together");
	} 

	if (fHtml && fText) {
		errorExitWithUsage("cannot specify `--html' and `--text' together");
	}
	
	if (fSkip && !(fPasscode || fCard)) {
		errorExitWithUsage("must specify a passcode to `--skip' to with -p or -c.");
	} 
	
	if ( fText && !(fNext || fCard || fPasscode) ) {
		errorExitWithUsage("must specify which card(s) to generate with `--next' or `--card' or `--passcode'");
	}

	if ( fHtml && !(fNext || fCard) ) {
		errorExitWithUsage("must specify which card(s) to generate with `--next' or `--card'");
	}
	
	if ( ! (keyfileExists() || fKey || fPassphrase) ) {
		errorExitWithUsage("must create a sequence key with `--key' or use a `--passphrase'");
	}

	if (fPasscode) {
		if (validPasscode(passcode, strlen(passcode)) == 0) {
			errorExitWithUsage("invalid passcode specified");
		}
	}
	
	if (fPassphrase && fNext && !fCard) {
		errorExitWithUsage("Cannot use `--next' with `--passphrase' unless `--card' is also used");
	}
	
	if (fNext && fCard && !fPassphrase) {
		errorExitWithUsage("Cannot use `--next' with `--card' except with `--passphrase'");
	}
	
	if (fHtml && fPasscode) {
		errorExit("cannot specify `--passcode' with `--html'");
	}
	          
	/* Time-Based Authentication **Experimental** */
	if (fTime && (fHtml || fText)) {
		errorExit("Cannot print passcards.  Key is for time-based authentication.");
	}
	if (fTime && fSkip) {
		errorExit("Cannot skip.  Key is for time-based authentication.");
	}
	
	if (fUseVersion && !fPassphrase) {
		errorExit("--useVersion can only be used with --passphrase");
	}
	
	
}

char *getPassphrase() {
	return passphrase;
}

void clCleanup() {
	mp_clear(&cardNum);
}
