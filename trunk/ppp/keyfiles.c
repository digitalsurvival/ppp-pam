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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <ctype.h>

#include "ppp.h"

#include "sha2.h"
#include "cmdline.h" 

#include "keyfiles.h"

static char *private_key_file_name = "/private_key";
static char *private_count_file_name = "/private_cnt";
static char *private_generated_file_name = "/private_gen";
static char *private_key_dir = "/.pppauth";
static char userhome[128] = "";


static char *_home_dir() {
	static struct passwd *pwdata = NULL;
	
	if (strlen(userhome) == 0) {
		/* get home dir for logged-in user */
		if (pwdata == NULL) {
			uid_t uid = geteuid();
			pwdata = getpwuid(uid);
		}
	
		if (pwdata) {
			return pwdata->pw_dir;
		}
	} else {
		return userhome;
	}
	
	
	return NULL;
}                                     

static char *_key_file_dir() {
	static char fname[128] = "";
	
	if (strlen(fname) == 0) {
		strncpy(fname, _home_dir(), 128 - strlen(private_key_file_name) - strlen(private_key_dir) - 1);
		strncat(fname, private_key_dir, strlen(private_key_dir));
	}
	return fname;
}

static char *_key_file_name() {
	static char fname[128] = "";
	
	if (strlen(fname) == 0) {
		strncpy(fname, _key_file_dir(), 128 - strlen(private_key_file_name) - 1);
		strncat(fname, private_key_file_name, strlen(private_key_file_name));
	}
	return fname;
}

static char *_cnt_file_name() {
	static char fname[128] = "";
	
	if (strlen(fname) == 0) {
		strncpy(fname, _key_file_dir(), 128 - strlen(private_count_file_name) - 1);
		strncat(fname, private_count_file_name, strlen(private_count_file_name));
	}
	return fname;
}

static char *_gen_file_name() {
	static char fname[128] = "";
	
	if (strlen(fname) == 0) {
		strncpy(fname, _key_file_dir(), 128 - strlen(private_generated_file_name) - 1);
		strncat(fname, private_generated_file_name, strlen(private_generated_file_name));
	}
	return fname;
}

static void _enforce_permissions() {
	chown(_key_file_dir(), -1, 0);
	chmod(_key_file_dir(), S_IRWXU | S_IRWXG);

	chown(_key_file_name(), -1, 0);
	chmod(_key_file_name(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

	chown(_cnt_file_name(), -1, 0);
	chmod(_cnt_file_name(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

	chown(_gen_file_name(), -1, 0);
	chmod(_gen_file_name(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
}

static int _file_exists(char *fname) {
	struct stat s;
	if (stat(fname, &s) < 0) {
		/* does not exist */
		return 0;
	} else {
		if (s.st_mode & S_IFREG) {
			/* exists and is a regular file */
			_enforce_permissions();
			return 1;
		} else {
			/* exists but is not a regular file */
			// TODO: abort with an error
			return 0;
		}
	}
	return 0;
}

static int _dir_exists(char *fname) {
	struct stat s;
	if (stat(fname, &s) < 0) {
		/* does not exist */
		return 0;
	} else {
		if (s.st_mode & S_IFDIR) {
			/* exists and is a folder */
			_enforce_permissions();
			return 1;
		} else {
			/* exists but is not a folder */
			// TODO: abort with an error
			return 0;
		}
	}
	return 0;
}

static int _ppp_version(char *buf) {
	int ver = -1;
	
	if (buf[0] != ' ')
		return 1;
		
	buf[4] = '\x00';
	if (strncmp(buf, " PPP", 4) != 0) {
		/* not a PPP file, so exit */
		return -1;
	}

	buf[9] = '\x00';
	sscanf(buf+5, "%04d", &ver);
		
	return ver;
}

static int _data_format(char *buf) {
	int fmt = -1;
	
	if (buf[0] != ' ') {
		/* not a versioned file */
		return 0;
	}
	
	buf[14] = '\x00';
	sscanf(buf+10, "%04d", &fmt);
		
	return fmt;
}

static int _ppp_flags(char *buf) {
	int flags = 0;
	
	if (buf[0] != ' ') {
		/* not a versioned file */
		pppClearFlags(0xffff);
		return 0;
	}
		
	if (strlen(buf+15) < 5) {
		/* is not long enough to check for flags */
		pppClearFlags(0xffff);
		return 0;
	}
	
	if (buf[19] != ' ') {
		/* file does not contain flags */
		pppClearFlags(0xffff);
		return 0;
	}
		
	buf[19] = '\x00';
	sscanf(buf+15, "%04x", &flags);
	pppSetFlags(PPP_FLAGS_PRESENT);
	
	return flags;
}

static void _read_data(char *buf, mp_int *mp) {	
	switch (_data_format(buf)) {
		case 0:
			/* unversioned file, mpi radix 64 format */
			mp_read_radix(mp, (unsigned char *)buf, 64);
			break;
		case 1:
			/* versioned file, mpi radix 64 format */
			if (pppCheckFlags(PPP_FLAGS_PRESENT)) 
				mp_read_radix(mp, (unsigned char *)(buf+20), 64);
			else
				mp_read_radix(mp, (unsigned char *)(buf+15), 64);
			break;
	}
	
}
      
static void _write_data(mp_int *mp, FILE *fp) {
	char buf[128];
	/* write ppp identifer */
	fwrite(" PPP ", 1, 5, fp);

	/* write ppp version */
	fprintf(fp, "%04d", pppVersion());
	fwrite(" ", 1, 1, fp);
	
	/* Current data format is versioned file, 
	 * mpi radix 64. 
	 */
	int current_data_format = 1;

	/* write data format */
	fprintf(fp, "%04d", current_data_format);
	fwrite(" ", 1, 1, fp); 
	
	/* write flags */
	fprintf(fp, "%04x", pppCheckFlags(0xffff));
	fwrite(" ", 1, 1, fp);

	/* IMPORTANT NOTE:
	 *
	 * If you change current_data_format above, 
	 * make sure you:
	 * 
	 * 1. Update the code below that writes the data to
	 *    reflect the data format you specified in
	 *    current_data_format above.
	 * 2. Add compatible read code to the switch statement
	 * in _read_data() above.
	 */
	              
	/* mpi radix 64 is the data format du jour */
	mp_toradix(mp, (unsigned char *)buf, 64);
	fwrite(buf, 1, strlen(buf)+1, fp);
}

static int confirm(char *prompt) {
	char buf[1024], *p;
	
	do {
		/* Display the prompt (on stderr because stdout might be redirected). */
		fflush(stdout);
		fprintf(stderr, "%s", prompt);
		fflush(stderr);
		/* Read line */
		if (fgets(buf, sizeof(buf), stdin) == NULL) {
			/* Got EOF.  Just exit. */
			/* Print a newline (the prompt probably didn't have one). */
			fprintf(stderr, "\n");
			fprintf(stderr, "Aborted by user");
			return 0;
		}
		p = buf + strlen(buf) - 1;
		while (p > buf && isspace(*p))
			*p-- = '\0';
		p = buf;
		while (*p && isspace(*p))
			p++;
		if (strcasecmp(p, "no") == 0) {
			return 0;
		}
	} while (strcasecmp(p, "yes") != 0);

	return 1;
}

void setUser(char *user) {
#ifdef OS_IS_MACOSX	
	strncpy(userhome, "/Users/", 7);
#endif
#ifdef OS_IS_LINUX	
	strncpy(userhome, "/home/", 6);
#endif
	strncat(userhome, user, 120);
}

int keyfileExists() {
	return _file_exists(_key_file_name());
}

int readKeyFile() {
	FILE *fp;
	char buf[128];
	mp_int num;
	int ver[3];
	
	mp_init(&num);
	
	if ( ! _file_exists(_key_file_name()) )
		return 0;

	if ( ! _file_exists(_cnt_file_name()) )
		return 0;
	
	if ( ! _file_exists(_gen_file_name()) )
		return 0;
	
	fp = fopen(_key_file_name(), "r");
	if ( ! fp) 
		return 0;
	fread(buf, 1, 128, fp);
	fclose(fp);
	ver[0] = _ppp_version(buf);
	pppSetFlags(_ppp_flags(buf)); /* load flags */
	_read_data(buf, &num);
	setSeqKey(&num);

	fp = fopen(_cnt_file_name(), "r");
	if ( ! fp) 
		return 0;
	fread(buf, 1, 128, fp);
	fclose(fp);
	ver[1] = _ppp_version(buf);
	_read_data(buf, &num);
	setCurrPasscodeNum(&num);

	fp = fopen(_gen_file_name(), "r");
	if ( ! fp) 
		return 0;
	fread(buf, 1, 128, fp);
	ver[2] = _ppp_version(buf);
	fclose(fp);
	_read_data(buf, &num);
	setLastCardGenerated(&num);

	memset(buf, 0, 128);
	mp_clear(&num);
	
	if ( (ver[0] != ver[1]) || (ver[1] != ver[2]) ) {
		/* Inconsistency in PPP version among the three data files */
		return 0;
	}

	/* tell PPP code which version the key expects */
	setKeyVersion(ver[0]);

	return 1;
}
    
int writeState() {
	FILE *fp[2];
	char buf[128];

	if ( ! _dir_exists(_key_file_dir()) )
		return 0;
		
	if ( ! pppCheckFlags(PPP_FLAGS_PRESENT)) {
		/* Update the key file to include flags */
		pppSetFlags(PPP_FLAGS_PRESENT);
		fp[0] = fopen(_key_file_name(), "w");
		if (fp[0]) {
			_write_data(seqKey(), fp[0]);
			fclose(fp[0]);
		}
	}
		
	fp[0] = fopen(_cnt_file_name(), "w");
	fp[1] = fopen(_gen_file_name(), "w");
	if (fp[0] && fp[1]) {
		_write_data(currPasscodeNum(), fp[0]);
		fclose(fp[0]);

		_write_data(lastCardGenerated(), fp[1]);
		fclose(fp[1]);
                     
		memset(buf, 0, 128);
		
		return 1;
	}
	
	return 0;
}

int writeKeyFile() {
	FILE *fp[3];
	char buf[128];
	int proceed = 1;
	
	/* create ~/.pppauth if necessary */
	if ( ! _dir_exists(_key_file_dir()) ) {
		mkdir(_key_file_dir(), S_IRWXU);
	}
       
	/* warn about overwriting an existing key */
	if ( _file_exists(_key_file_name()) ) {
		proceed = 0;
		fprintf(stderr, "\n"
		    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
			"@    WARNING: YOU ARE ABOUT TO OVERWRITE YOUR KEY FILE!   @\n"
			"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
			"\n"
			"IF YOU PROCEED, YOUR EXISTING PASSCARDS WILL BECOME USELESS!\n"
			"\n"
			"If this is not what you intended to do, type `NO' below.\n"
			"\n"
			"By typing `yes' below, a new sequence key will be generated\n"
			"and you will no longer be able to log in using your existing\n"
			"passcards.  New passcards must be printed.\n"
			"\n"
		);
		
		proceed = confirm("Are you sure you want to proceed (yes/no)? ");
	}

	if (proceed) {
		_enforce_permissions();
	
		umask(S_IRWXG|S_IRWXO);
		fp[0] = fopen(_key_file_name(), "w");
		fp[1] = fopen(_cnt_file_name(), "w");
		fp[2] = fopen(_gen_file_name(), "w");
		if (fp[0] && fp[1] && fp[2]) {
			_write_data(seqKey(), fp[0]);
			fclose(fp[0]);

			_write_data(currPasscodeNum(), fp[1]);
			fclose(fp[1]);

			_write_data(lastCardGenerated(), fp[2]);
			fclose(fp[2]);
			
			memset(buf, 0, 128);
			
			fprintf(stderr, "\n"
				"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
				"  A new sequence key has been generated and saved.  It is\n"
				"  HIGHLY RECOMMENDED that you IMMEDIATELY print new pass-\n"
				"  cards in order to access your account in the future.\n"
				"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"
				"\n"
			);
			return 1;
		}
	} else {
		fprintf(stderr, "\n"
			"===========================================================\n"
			"  A new sequence key WAS NOT generated and your old key\n"
			"  remains intact.  As a result, you can continue to use\n"
			"  your existing passcards to log into your account.\n"
			"===========================================================\n"
			"\n"
		);
	}
	
	return 0;
}
