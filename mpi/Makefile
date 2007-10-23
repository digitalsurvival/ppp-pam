##
## Makefile for MPI library
##
## by Michael J. Fromberger <http://www.dartmouth.edu/~sting/>
## Originally written 1998 by Michael J. Fromberger
## This source is in the public domain.
##
## $Id: Makefile,v 1.1 2004/02/08 04:29:29 sting Exp $
##
## To build the MPI library, edit the configuration options in the
## file Makefile.base, and then type 'make lib' to make the library.
## To run unit tests, run the 'all-tests' script.  To run timing
## tests, run 'timetest'.  There are also some compile-time options in
## 'mpi-config.h', which you should set as appropriate.
##

include Makefile.base

###----------------------------------------------------------------------
### You should not need to edit anything below this line, unless you
### are modifying and/or re-distributing the MPI library.
###----------------------------------------------------------------------

# This is the version string used for the documentation, and used when
# building the distribution tarball.  
VERS=1.8.6

# These are the source files included in the distribution tar file
SRCS=mpi.c mplogic.c mpprime.c mprsa.c primes.c

# These are the header files included in the distribution tar file
HDRS=logtab.h mpi-config.h mpi-types.h mpi.h mplogic.h mpprime.h mprsa.h

# These are scripts and other files that should be included in the
# distribution file
OTHER=README CHANGES Makefile Makefile.base make-logtab stats types.pl

# How many bytes should each digit have?  (Advisory)
DIGIT_SIZE=2

# Create an archive of the MPI library and the primality testing code
lib: mpi.o mpprime.o
	ar crf libmpi.a mpi.o mpprime.o
	$(RANLIB) libmpi.a

mpi.o: logtab.h mpi-config.h mpi-types.h mpi.h mpi.c
	$(CC) $(CFLAGS) -c mpi.c

mpi-types.h:
	$(PERL) types.pl $(DIGIT_SIZE) > mpi-types.h

logtab.h:
	$(PERL) make-logtab > logtab.h

mpprime.o: mpi.h mpprime.h primes.c mpprime.c
	$(CC) $(CFLAGS) -c mpprime.c

tools: lib
	@cd utils && make tools

test: lib
	@cd tests && make mpi-test && ./all-tests

clean:
	rm -f *.o core *~

allclean: clean
	@cd utils && make clean
	@cd tests && make clean

distclean: clean
	@cd utils && make distclean
	@cd tests && make distclean
	rm -f test-info.c libmpi.a

# Make a distribution tar file and corresponding PGP signature
dist: $(SRCS) $(HDRS) $(OTHER)
	tar -cf mpi-temp.tar \
		$(HDRS) $(SRCS) $(OTHER) contrib doc tests utils
	mkdir mpi-$(VERS)
	cd mpi-$(VERS) && tar -xf ../mpi-temp.tar
	rm -f mpi-temp.tar
	tar -cvf mpi-$(VERS).tar mpi-$(VERS)
	rm -rf mpi-$(VERS)
	gzip -9v mpi-$(VERS).tar

# -- end -- 
