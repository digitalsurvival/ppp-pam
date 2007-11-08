##
## Makefile to build MPI library utility programs
## 
## by Michael J. Fromberger <sting@linguist.dartmouth.edu>
## Copyright (C) 2000 Michael J. Fromberger, All Rights Reserved
##
## $Id: Makefile,v 1.1 2004/02/08 04:28:32 sting Exp $
##

# Pull in the user configurations from the base file
include ../Makefile.base

# Add the parent directory to the include and library paths, so that
# the compiler can find the header files properly.
TCFLAGS=$(CFLAGS) -I.. -L..

# This is the list of tools that will be built by 'make tools'
TOOLS=basecvt bin2mag exptmod fact gcd invmod isprime makeprime \
	mpfactor mpicalc mulsqr mult multime metime pi primegen sieve

tools: ../libmpi.a $(TOOLS)

basecvt: basecvt.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

exptmod: exptmod.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

fact: fact.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

gcd: gcd.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

invmod: invmod.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

isprime: isprime.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

makeprime: makeprime.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

mpfactor: mpfactor.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

mpicalc: mpicalc.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

mulsqr: mulsqr.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

multime: multime.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

metime: metime.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

pi: pi.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

primegen: primegen.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

sieve: sieve.c
	$(CC) $(TCFLAGS) -o $@ $<

mult: mult.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

bin2mag: bin2mag.c
	$(CC) $(TCFLAGS) -o $@ $< -lmpi $(LIBS)

clean:
	rm -f *.o core *~

distclean: clean
	rm -f $(TOOLS)

# -- end --
