# Copyright (c) 2007, Thomas Fors
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived from
#       this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

CC = gcc
CFLAGS += -Wall -Irijndael -Isha2 -Impi

PPPAUTH_OBJS += ppp/pppauth.o
PPPAUTH_OBJS += ppp/cmdline.o
PPPAUTH_OBJS += ppp/keyfiles.o
PPPAUTH_OBJS += ppp/ppp.o
PPPAUTH_OBJS += build/pppauth.a
PPPAUTH_OBJS += ppp/print.o
PPPAUTH_OBJS += ppp/http.o
PPPAUTH_OBJS += mpi/libmpi.a

PAM_OBJS += ppp/pam_ppp.o
PAM_OBJS += ppp/keyfiles.o
PAM_OBJS += ppp/ppp.o
PAM_OBJS += build/pppauth.a
PAM_OBJS += mpi/libmpi.a

TESTCMD = build/pppauth --passphrase testvectors --text --name testvectors --card
TESTFILTER = grep -v testvectors | grep -v ww.GRC.com

all: build/pppauth build/pam_ppp.so

install: build/pppauth build/pam_ppp.so
	cp build/pppauth /usr/bin/pppauth
	cp build/pam_ppp.so /usr/lib/pam/pam_ppp.so

test: build/pppauth
	make -C mpi test
	@cat ppp/testvectors.txt | $(TESTFILTER) > build/testvectors.txt
	@$(TESTCMD) 1 | $(TESTFILTER) > build/testoutput.txt
	@$(TESTCMD) 2 | $(TESTFILTER) >> build/testoutput.txt
	@$(TESTCMD) 3 | $(TESTFILTER) >> build/testoutput.txt
	@$(TESTCMD) 1234567890 | $(TESTFILTER) >> build/testoutput.txt
	@$(TESTCMD) 1234567891 | $(TESTFILTER) >> build/testoutput.txt
	@$(TESTCMD) 1234567892 | $(TESTFILTER) >> build/testoutput.txt
	@$(TESTCMD) 65536 | $(TESTFILTER) >> build/testoutput.txt
	@$(TESTCMD) 65537 | $(TESTFILTER) >> build/testoutput.txt
	@$(TESTCMD) 65538 | $(TESTFILTER) >> build/testoutput.txt
	@$(TESTCMD) 4294967295 | $(TESTFILTER) >> build/testoutput.txt
	@$(TESTCMD) 4294967296 | $(TESTFILTER) >> build/testoutput.txt
	@$(TESTCMD) 4294967297 | $(TESTFILTER) >> build/testoutput.txt
	@echo Running test vectors for pppauth...
	cmp build/testvectors.txt build/testoutput.txt
	@echo Passed all test vectors.

build/pppauth: $(PPPAUTH_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

build/pam_ppp.so: $(PAM_OBJS)
	$(CC) -bundle -Ddarwin -no-cpp-precomp -lc -lpam $^ -o $@ 
	
ppp/pam_ppp.o: ppp/pam_ppp.c
	$(CC) $(CFLAGS) -Ddarwin -no-cpp-precomp -DPAM_DYNAMIC -c  $^ -o $@

build/pppauth.a: rijndael/rijndael.o sha2/sha2.o 
	ar -cr $@ $^

mpi/libmpi.a:
	make -C mpi

.PHONY: clean
clean:
	rm -f build/* rijndael/*.o sha2/*.o ppp/*.o
	make -C mpi clean

