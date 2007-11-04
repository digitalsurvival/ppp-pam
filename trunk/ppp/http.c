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
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>

#include "ppp.h"

#include "http.h"

#define SERVER "webserver/1.0"
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

void htmlStart(FILE *f) {
	fprintf(f, "<html>\n");
	fprintf(f, "<head><title>&nbsp;Perfect Paper Passwords&nbsp;</title>\n");	
	fprintf(f, "<style type=\"text/css\">\n");
	
	fprintf(f, "body { color:#009; background:white; font-size:10pt; font-family: verdana, tahoma, arial, helvetica, sans-serif, \"MS Sans Serif\"; }\n");
	fprintf(f, "#nopagepad { margin:0; padding:0; border:0; } /* for pages with flush graphics*/\n");
	

	fprintf(f, ".passcard {\n");
	fprintf(f, "color:#000;\n");
	fprintf(f, "background:#fff;\n");
	fprintf(f, "border:1px solid #000;\n");
	fprintf(f, "font-size:14px;\n");
	fprintf(f, "font-family:\"Courier New\", monospace;\n");
	fprintf(f, "}\n");

	fprintf(f, ".passcard_header {\n");
	fprintf(f, "color:#000;\n");
	fprintf(f, "background:#eee;\n");
	fprintf(f, "border-bottom:solid #aaa;\n");
	fprintf(f, "border-bottom-width:1px;\n");
	fprintf(f, "padding:6px 10px;\n");
	fprintf(f, "}\n");

	fprintf(f, ".passcard_content {\n");
	fprintf(f, "padding:4px 10px 10px 10px;\n");
	fprintf(f, "font-weight:bold;\n");
	fprintf(f, "}\n");

	fprintf(f, ".passcard_column_labels {\n");
	fprintf(f, "text-align:center; padding-bottom:2px;\n");
	fprintf(f, "}\n");

	fprintf(f, ".passchar {\n");
	fprintf(f, "color:black;\n");
	fprintf(f, "text-align:center;\n");
	fprintf(f, "font-weight:bold;\n");
	fprintf(f, "font-size:22px;\n");
	fprintf(f, "font-family:\"Courier New\", monospace;\n");
	fprintf(f, "margin:0.75em 0;\n");
	fprintf(f, "}	\n");
	
	fprintf(f, ".passcard { font-size:14px; }\n");
	
	fprintf(f, "@media screen { .passcard_content {	font-weight:normal; } }\n");
	
	fprintf(f, "</style>\n");
	fprintf(f, "</head><body id=\"nopagepad\" bgcolor=\"white\"><center><table border=\"0\" cellpadding=\"0\" cellspacing=\"1\">\n");
}

void htmlEnd(FILE *f) {
	fprintf(f, "</table>\n");
	fprintf(f, "<br />\n");
	fprintf(f, "<font color=\"black\" size=1><b>Print this page (in portrait orientation if you would like all three cards<br />to fit) then either cut out and separate the individual passcards, or<br />keep the three attached and fold down into one-card size.</b><span class=\"font7px\"><br /><br /></span>Please refer to GRC's Perfect Paper Passwords pages<br />for information about the operation of this system.</font></center>\n");	
	fprintf(f, "</html>\n");
}

void htmlCard(FILE *f, mp_int *nCard) {
	char groupChar = ',';
	mp_int start;
	mp_init(&start);
	calculatePasscodeNumberFromCardColRow(nCard, 0, 0, &start);

	char buf[70*4];
	getPasscodeBlock(&start, 70, buf);
	mp_clear(&start);
	
	char hname[39];
	strncpy(hname, hostname(), 38);
	
	mp_int n;
	mp_init(&n);
	mp_add_d(nCard, 1, &n);
	char *cardnumber = mpToDecimalString(&n, groupChar);
	char *cn = cardnumber;
	mp_clear(&n);
	
	if (strlen(hname) + strlen(cardnumber) + 3 > 38) {
		if (strlen(hname) > 27) {
			hname[27] = '\x00';
		}
		int ellipses = strlen(cardnumber) - (38 - strlen(hname) - 3);
		if (ellipses > 0) {
			cn = cardnumber+ellipses;
			cn[0] = cn[1] = cn[2] = '.';
			/* When truncating the card number, make sure we don't
			 * begin with a comma after the ellipses
			 */
			if (cn[3] == groupChar) {
				cn[3] = '.';
				cn++;
			}
		}
	}
	
	fprintf(f, "<tr><td><div class=\"passcard\">\n");
	fprintf(f, "<div class=\"passcard_header\">");
	fprintf(f, "%s", hname);
	int j;
	for (j=0; j<38-strlen(hname)-strlen(cn)-2; j++)
		fprintf(f, "&nbsp;");
	fprintf(f, "[%s]</div>\n", cn);
	
	fprintf(f, "<div class=\"passcard_content\">\n");
	fprintf(f, "<div class=\"passcard_column_labels\">");
	fprintf(f, "&nbsp;&nbsp;&nbsp;&nbsp;A&nbsp;&nbsp;&nbsp;&nbsp;B&nbsp;&nbsp;&nbsp;&nbsp;C&nbsp;&nbsp;&nbsp;&nbsp;D&nbsp;&nbsp;&nbsp;&nbsp;E&nbsp;&nbsp;&nbsp;&nbsp;F&nbsp;&nbsp;&nbsp;&nbsp;G\n");
	fprintf(f, "</div>\n");
	         
	j = 0;
	int r, c;
	for (r=1; r<=10; r++) {
		if (r < 10) fprintf(f, "&nbsp;");
		fprintf(f, "%d:&nbsp;", r);
		for (c=0; c<7; c++) {
			if (c) fprintf(f, "&nbsp;");
			fprintf(f, "%c%c%c%c", buf[j*4+0], buf[j*4+1], buf[j*4+2], buf[j*4+3]);  
			j++;
		}
		fprintf(f, "<br />\n");
	}
 	fprintf(f, "</div>\n");
	fprintf(f, "</div>\n");
	fprintf(f, "</td></tr>\n");

	/* zero passcodes from memory */
	memset(buf, 0, 70*4);
}

void httpSendHeaders(FILE *f, int status, char *title, char *extra, char *mime, int length, time_t date) {
  time_t now;
  char timebuf[128];

  fprintf(f, "%s %d %s\r\n", PROTOCOL, status, title);
  fprintf(f, "Server: %s\r\n", SERVER);
  now = time(NULL);
  strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
  fprintf(f, "Date: %s\r\n", timebuf);
  if (extra) fprintf(f, "%s\r\n", extra);
  if (mime) fprintf(f, "Content-Type: %s\r\n", mime);
  if (length >= 0) fprintf(f, "Content-Length: %d\r\n", length);
  if (date != -1)
  {
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&date));
    fprintf(f, "Last-Modified: %s\r\n", timebuf);
  }
  fprintf(f, "Connection: close\r\n");
  fprintf(f, "\r\n");
}


void httpSendError(FILE *f, int status, char *title, char *extra, char *text) {
  httpSendHeaders(f, status, title, extra, "text/html", -1, -1);
  fprintf(f, "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\r\n", status, title);
  fprintf(f, "<BODY><H4>%d %s</H4>\r\n", status, title);
  fprintf(f, "%s\r\n", text);
  fprintf(f, "</BODY></HTML>\r\n");
}

int httpProcess(FILE *f) {
	int rv = 1;
	char buf[4096];

	char *method;
	char *path;
	char *protocol;

	if (!fgets(buf, sizeof(buf), f)) return -1;

	method = strtok(buf, " ");
	path = strtok(NULL, " ");
	protocol = strtok(NULL, "\r");
	if (!method || !path || !protocol) return -1;

	fseek(f, 0, SEEK_CUR); // Force change of stream direction

	if (strcasecmp(method, "GET") != 0)
		httpSendError(f, 501, "Not supported", NULL, "Method is not supported.");
	if (strncmp(path, "/", 2) == 0) {
		httpSendHeaders(f, 200, "OK", NULL, "text/html", /* length */ -1, /* statbuf->st_mtime */ -1);
		if (fNext) {
			int i;
			htmlStart(f);
			for (i=0; i<numCards; i++) {
				mp_int mp;
				mp_init(&mp);
				mp_add_d(lastCardGenerated(), 1, &mp);
				htmlCard(f, &mp);
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
			htmlEnd(f);
		} else {
			htmlStart(f);
			htmlCard(f, &cardNum);
			htmlEnd(f);
		}
		rv = 0;
	} else {
		httpSendError(f, 404, "Not Found", NULL, "File not found.");
	}

	return rv;
}

void httpServe() {
	int sock;
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);

	sock = socket(AF_INET, SOCK_STREAM, 0);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = 0;
	bind(sock, (struct sockaddr *) &sin, slen);

	listen(sock, 5);

	getsockname(sock, (struct sockaddr *)&sin, &slen);
	char url[128];
	sprintf(url, "http://localhost:%d", ntohs(sin.sin_port));
	
	if ( ! fork()) {
#ifdef OS_IS_MACOSX
		execlp("/usr/bin/open", "/usr/bin/open", url, NULL);
#else
		printf("\nPlease launch a browser on this machine and go to the URL:\n");
		printf("   %s\n", url);
		printf("To get your passcards for printing.\n\n");
		printf("Waiting for browser to access URL...\n");
#endif		
	}
		
	int s;
	FILE *f;

	s = accept(sock, NULL, NULL);
	
	if (s >= 0) {
		f = fdopen(s, "a+");
		httpProcess(f);
		fflush(f);
		fclose(f);
	}

	close(sock);
}
