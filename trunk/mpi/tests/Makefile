##
## Makefile to build MPI library test programs
## 
## by Michael J. Fromberger <sting@linguist.dartmouth.edu>
## Copyright (C) 2000 Michael J. Fromberger, All Rights Reserved
##
## $Id: Makefile,v 1.1 2004/02/08 04:28:59 sting Exp $
##

# Pull in the user configurations from the base file
include ../Makefile.base

# Add the parent directory to the include and library paths, so that
# the compiler can find the header files properly.
TCFLAGS=$(CFLAGS) -I.. -L..

mpi-test: mpi-test.c test-info.c ../libmpi.a
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

magtest: magtest.c ../libmpi.a
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

test-info.c: test-arrays.txt make-test-arrays
	perl make-test-arrays test-arrays.txt > test-info.c

mptest-rsa: mptest-rsa.c ../libmpi.a ../mprsa.o
	$(CC) $(TCFLAGS) -o $@ ../mprsa.o $< -lmpi $(LIBS)

clean:
	rm -f *.o core *~

distclean: clean
	rm -f a.out magtest mptest-rsa mpi-test test-info.c test-errors.txt

# -- end --
