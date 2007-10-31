
MYCFLAGS = -I./$(srcdir)/rijndael -I./$(srcdir)/sha2 -I./$(srcdir)/mpi \
         -Wall -O3 -funsigned-char

MPISRC = dummy.c ./$(srcdir)/mpi/mpi.c ./$(srcdir)/mpi/mpprime.c

dummy.c: logtab.h mpi-types.h
	echo "#include <stdio.h>" > dummy.c

logtab.h: ./$(srcdir)/mpi/make-logtab
	$(PERL) ./$(srcdir)/mpi/make-logtab > $@

mpi-types.h: ./$(srcdir)/mpi/types.pl
	$(PERL) ./$(srcdir)/mpi/types.pl 2 $(CC) "$(MYCFLAGS)" > $@

PPPSRC = ./$(srcdir)/ppp/keyfiles.h ./$(srcdir)/ppp/keyfiles.c \
         ./$(srcdir)/ppp/ppp.h ./$(srcdir)/ppp/ppp.c \
         ./$(srcdir)/rijndael/rijndael.h ./$(srcdir)/rijndael/rijndael.c \
         ./$(srcdir)/sha2/sha2.h ./$(srcdir)/sha2/sha2.c

./$(srcdir)/ppp/pppauth.c: mpi-types.h

bin_PROGRAMS = pppauth
pppauth_SOURCES = \
	./$(srcdir)/ppp/pppauth.c ./$(srcdir)/ppp/cmdline.h ./$(srcdir)/ppp/cmdline.c \
	./$(srcdir)/ppp/print.h ./$(srcdir)/ppp/print.c ./$(srcdir)/ppp/http.h ./$(srcdir)/ppp/http.c \
	$(PPPSRC) $(MPISRC)
pppauth_LDADD = $(UUID_LIBS)
pppauth_CFLAGS = $(MYCFLAGS)


noinst_PROGRAMS = pam_ppp.so
pam_ppp_so_SOURCES = ./$(srcdir)/ppp/pam_ppp.c $(PPPSRC) $(MPISRC)
if OSX
pam_ppp_so_LDFLAGS = -bundle -Ddarwin -lc -lpam $(UUID_LIBS)
pam_ppp_so_CFLAGS = $(MYCFLAGS) -Ddarwin -no-cpp-precomp -DPAM_DYNAMIC
else	
pam_ppp_so_LDFLAGS = -shared -lc -lpam $(UUID_LIBS)
pam_ppp_so_CFLAGS = $(MYCFLAGS) -DPAM_DYNAMIC -fPIC
endif

TESTCMD = ./pppauth --passphrase testvectors --text --name testvectors --card
TESTFILTER = grep -v testvectors
test: pppauth
	@cat ./$(srcdir)/ppp/testvectors.txt | $(TESTFILTER) > testvectors.txt
	@$(TESTCMD) 1 | $(TESTFILTER) > testoutput.txt
	@$(TESTCMD) 2 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 3 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 1234567890 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 1234567891 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 1234567892 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 65536 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 65537 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 65538 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 4294967295 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 4294967296 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 4294967297 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 18446744073709551615 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 18446744073709551616 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 18446744073709551617 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 79228162514264337593543950335 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 79228162514264337593543950336 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 79228162514264337593543950337 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 5192296858534827628530496329220095 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 5192296858534827628530496329220096 | $(TESTFILTER) >> testoutput.txt
	@$(TESTCMD) 5192296858534827628530496329220097 | $(TESTFILTER) >> testoutput.txt
	@echo Running test vectors for pppauth...
	cmp testvectors.txt testoutput.txt
	@echo Passed all test vectors.

install-exec-local:
if OSX
	cp pam_ppp.so /usr/lib/pam/pam_ppp.so
else	
	cp pam_ppp.so /lib/security/pam_ppp.so
endif	

CLEANFILES = logtab.h mpi-types.h dummy.c testvectors.txt testoutput.txt

maintclean:
	- make clean
	rm -f ./$(srcdir)/Makefile.in ./$(srcdir)/aclocal.m4 ./$(srcdir)/compile \
		./$(srcdir)/configure ./$(srcdir)/depcomp ./$(srcdir)/install-sh \
		./$(srcdir)/missing ./$(srcdir)/mkinstalldirs
	rm -rf ./$(srcdir)/autom4te.cache .deps
	rm -f ./$(srcdir)/config.main ./$(srcdir)/config.sub ./$(srcdir)/config.guess \
		./$(srcdir)/ltmain.sh
	rm -f ./$(srcdir)/libtool ./$(srcdir)/config.log ./$(srcdir)/config.status
	rm -f ./$(srcdir)/build/*
	rm -f Makefile
	
maintprep: maintclean
	cd ./$(srcdir); \
	glibtoolize --copy; \
	aclocal; \
	autoconf; \
	automake --add-missing --copy
