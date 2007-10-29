noinst_LIBRARIES = libmpi.a libppp.a

libmpi_a_SOURCES = dummy.c ./$(srcdir)/mpi/mpi.c ./$(srcdir)/mpi/mpprime.c
libmpi_a_CFLAGS = -ansi -pedantic -Wall -O3 -funsigned-char

dummy.c: logtab.h mpi-types.h
	touch dummy.c

logtab.h: ./$(srcdir)/mpi/make-logtab
	$(PERL) ./$(srcdir)/mpi/make-logtab > $@

mpi-types.h: ./$(srcdir)/mpi/types.pl
	$(PERL) ./$(srcdir)/mpi/types.pl 2 $(CC) "$(libmpi_a_CFLAGS)" > $@

libppp_a_SOURCES = ./$(srcdir)/ppp/keyfiles.h ./$(srcdir)/ppp/keyfiles.c \
                   ./$(srcdir)/ppp/ppp.h ./$(srcdir)/ppp/ppp.c \
                   ./$(srcdir)/rijndael/rijndael.h ./$(srcdir)/rijndael/rijndael.c \
                   ./$(srcdir)/sha2/sha2.h ./$(srcdir)/sha2/sha2.c
libppp_a_CFLAGS = -Wall -I./$(srcdir)/rijndael -I./$(srcdir)/sha2 -I./$(srcdir)/mpi

bin_PROGRAMS = pppauth
pppauth_SOURCES = \
	./$(srcdir)/ppp/pppauth.c ./$(srcdir)/ppp/cmdline.h ./$(srcdir)/ppp/cmdline.c \
	./$(srcdir)/ppp/print.h ./$(srcdir)/ppp/print.c ./$(srcdir)/ppp/http.h ./$(srcdir)/ppp/http.c
pppauth_LDADD = libmpi.a libppp.a $(UUID_LIBS)
pppauth_CFLAGS = -Wall -I./$(srcdir)/rijndael -I./$(srcdir)/sha2 -I./$(srcdir)/mpi


noinst_PROGRAMS = pam_ppp.so
pam_ppp_so_SOURCES = ./$(srcdir)/ppp/pam_ppp.c
pam_ppp_so_LDADD = libmpi.a libppp.a
pam_ppp_so_LDFLAGS = -bundle -Ddarwin -lc -lpam $(UUID_LIBS)
pam_ppp_so_CFLAGS = -Wall -Ddarwin -no-cpp-precomp -DPAM_DYNAMIC \
                    -I./$(srcdir)/rijndael -I./$(srcdir)/sha2 -I./$(srcdir)/mpi


## CLEANFILES  = $(srcdir)/mpi/*.o $(srcdir)/mpi/core $(srcdir)/mpi/*~ 
## CLEANFILES += $(srcdir)/mpi/tests/*.o $(srcdir)/mpi/tests/core $(srcdir)/mpi/tests/*~
## CLEANFILES += $(srcdir)/mpi/utils/*.o $(srcdir)/mpi/utils/core $(srcdir)/mpu/tests/*~
## 
## DISTCLEANFILES  = $(srcdir)/mpi/test-info.c $(srcdir)/mpi/libmpi.a
## DISTCLEANFILES += $(srcdir)/mpi/tests/a.out $(srcdir)/mpi/tests/magtest \
##                   $(srcdir)/mpi/tests/mptest-rsa $(srcdir)/mpi/tests/mpi-test \
##                   $(srcdir)/mpi/tests/test-info.c $(srcdir)/mpi/tests/test-errors.txt
## DISTCLEANFILES += $(srcdir)/mpi/utils/basecvt $(srcdir)/mpi/utils/bin2mag \
##                   $(srcdir)/mpi/utils/exptmod $(srcdir)/mpi/utils/fact \
##                   $(srcdir)/mpi/utils/gcd $(srcdir)/mpi/utils/invmod \
##                   $(srcdir)/mpi/utils/isprime $(srcdir)/mpi/utils/makeprime \
##                   $(srcdir)/mpi/utils/mpfactor $(srcdir)/mpi/utils/mpicalc \
##                   $(srcdir)/mpi/utils/mulsqr $(srcdir)/mpi/utils/mult \
##                   $(srcdir)/mpi/utils/multime $(srcdir)/mpi/utils/metime \
##                   $(srcdir)/mpi/utils/pi $(srcdir)/mpi/utils/primegen \
##                   $(srcdir)/mpi/utils/sieve

## test: build/pppauth
## 	make -C mpi test
## 	@cat ppp/testvectors.txt | $(TESTFILTER) > build/testvectors.txt
## 	@$(TESTCMD) 1 | $(TESTFILTER) > build/testoutput.txt
## 	@$(TESTCMD) 2 | $(TESTFILTER) >> build/testoutput.txt
## 	@$(TESTCMD) 3 | $(TESTFILTER) >> build/testoutput.txt
## 	@$(TESTCMD) 1234567890 | $(TESTFILTER) >> build/testoutput.txt
## 	@$(TESTCMD) 1234567891 | $(TESTFILTER) >> build/testoutput.txt
## 	@$(TESTCMD) 1234567892 | $(TESTFILTER) >> build/testoutput.txt
## 	@$(TESTCMD) 65536 | $(TESTFILTER) >> build/testoutput.txt
## 	@$(TESTCMD) 65537 | $(TESTFILTER) >> build/testoutput.txt
## 	@$(TESTCMD) 65538 | $(TESTFILTER) >> build/testoutput.txt
## 	@$(TESTCMD) 4294967295 | $(TESTFILTER) >> build/testoutput.txt
## 	@$(TESTCMD) 4294967296 | $(TESTFILTER) >> build/testoutput.txt
## 	@$(TESTCMD) 4294967297 | $(TESTFILTER) >> build/testoutput.txt
## 	@echo Running test vectors for pppauth...
## 	cmp build/testvectors.txt build/testoutput.txt
## 	@echo Passed all test vectors.

TESTCMD = ./pppauth --passphrase testvectors --text --name testvectors --card
TESTFILTER = grep -v testvectors | grep -v ww.GRC.com
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
	@echo Running test vectors for pppauth...
	cmp testvectors.txt testoutput.txt
	@echo Passed all test vectors.

install-exec-local:
	cp pam_ppp.so /usr/lib/pam/pam_ppp.so

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
