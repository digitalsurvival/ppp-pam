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

#define DEFAULT_USER "nobody"

#include <stdio.h>

#include "ppp.h"

/*
 * here, we make definitions for the externally accessible functions
 * in this file (these definitions are required for static modules
 * but strongly encouraged generally) they are used to instruct the
 * modules include file to define their prototypes.
 */

#define PAM_SM_AUTH
#define PAM_SM_ACCOUNT
#define PAM_SM_SESSION
#define PAM_SM_PASSWORD

#ifdef HAVE_SECURITY_PAM_MODULES_H
	#include <security/pam_modules.h>
	#include <security/_pam_macros.h>
#else	
	#include <pam/pam_modules.h>
	#include <pam/pam_mod_misc.h>
	#include <pam/_pam_macros.h>
#endif	

/* --- authentication management functions --- */

PAM_EXTERN
int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	int retval;
	const char *user=NULL;

	/*
	 * authentication requires we know who the user wants to be
	 */
	retval = pam_get_user(pamh, &user, NULL);
	if (retval != PAM_SUCCESS) {
		D(("get user returned error: %s", pam_strerror(pamh,retval)));
		return retval;
	}
	if (user == NULL || *user == '\0') {
		D(("username not known"));
		pam_set_item(pamh, PAM_USER, (const void *) DEFAULT_USER);
	}

	retval = PAM_AUTH_ERR;
	
	pppInit();
	
	setUser(user);
	if ( ! readKeyFile()) {
		retval = PAM_IGNORE;
		goto cleanup;
	}
	
	struct pam_conv *conversation;
	struct pam_message message;
	struct pam_message *pmessage = &message;
	struct pam_response *resp = NULL;
	if (pppCheckFlags(PPP_SHOW_PASSCODE)) {
		message.msg_style = PAM_PROMPT_ECHO_ON;
	} else {
		message.msg_style = PAM_PROMPT_ECHO_OFF;
	}
	message.msg = currPrompt();
	
	/* Use conversation function to prompt user for passcode */
	pam_get_item(pamh, PAM_CONV, (const void **)&conversation);
	conversation->conv(1, (const struct pam_message **)&pmessage,
			&resp, conversation->appdata_ptr);
	
	retval = PAM_AUTH_ERR;
	if (resp) {
		if (pppAuthenticate(resp[0].resp))
			retval = PAM_SUCCESS;
		_pam_drop_reply(resp, 1);
	}

cleanup:
	pppCleanup();

	return retval;
}

PAM_EXTERN
int pam_sm_setcred(pam_handle_t *pamh,int flags,int argc,const char **argv) {
	 return PAM_IGNORE;
}


/* --- session management functions --- */

PAM_EXTERN
int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	return PAM_IGNORE;
}

PAM_EXTERN
int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	int retval = PAM_IGNORE;
	const char *user;

	struct pam_conv *conversation;
	struct pam_message message;
	struct pam_message *pmessage = &message;
	struct pam_response *resp = NULL;

	if (flags & PAM_SILENT) {
		return retval;
	}

	if(pam_get_user(pamh, &user, NULL) != PAM_SUCCESS || user == NULL || *user == '\0') {
		return PAM_USER_UNKNOWN;
	}

	message.msg_style = PAM_TEXT_INFO;
	
	pppInit();
	setUser((char *)user);
	if ( ! readKeyFile()) {
		user = NULL;
		/* TODO return a more appropriate error here */
		return PAM_USER_UNKNOWN;
	}
	
	char buffer[2048];
	while (pppWarning(buffer, 2048)) {
		if (strlen(buffer)) {
			message.msg = buffer;
	
			/* Use conversation function to give user contents of umotd */
			pam_get_item(pamh, PAM_CONV, (const void **)&conversation);
			conversation->conv(1, (const struct pam_message **)&pmessage,
					&resp, conversation->appdata_ptr);
			if (resp)
				_pam_drop_reply(resp, 1);
		}
	}

	pppCleanup();

	return retval;
}

/* end of module definition */

#ifdef PAM_STATIC

/* static module data */

struct pam_module _pam_permit_modstruct = {
	"pam_ppp",
	pam_sm_authenticate,
	pam_sm_setcred,
	NULL,
	pam_sm_open_session,
	pam_sm_close_session,
	NULL
};

#endif
