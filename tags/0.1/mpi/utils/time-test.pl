#!/usr/local/bin/perl

#
# time-test.pl
#
# Perform timing tests for the system using the current build settings
# for the MPI library.  Outputs the results in a format suitable for
# inclusion in the timing results file distributed with the library.
#
# by Michael J. Fromberger <sting@linguist.dartmouth.edu>
# Copyright (C) 2000 Michael J. Fromberger, All Rights Reserved
#
# $Id: time-test.pl,v 1.1 2004/02/08 04:28:33 sting Exp $
#

@_=split(/\//,$0);chomp($prog=pop(@_));


# We'll use a fixed random seed for the timing tests so that the
# results will be consistent on a particular platform.  Most of the
# timing tools will read the 'SEED' environment variable to obtain
# this value.
$SEED = 45904523;

# For multiplication tests, these are the bit sizes of the values
# we'll test.
@mt_sizes = ( 1024, 1536, 2048, 2560, 3072, 3584, 4096, 8192 );

# How many multiplications to perform at each size in order to get the
# timing statistics.
$mt_tests = 20000;

# For modular exponentiation tests, these are the bit sizes of the
# values we'll test.
@me_sizes = ( 128, 192, 256, 320, 384, 448, 512, 640, 768, 896, 1024 );

# How many modular exponentiations to perform at each size in order to
# get the timing statistics.
$me_tests = 200;

# For prime generation tests, these are the bit sizes of the values
# we'll test.
@pg_sizes = ( 128, 192, 256, 320, 384, 448, 512 );

# How many primes should we generate at each size in order to get the
# timing statistics.
$pg_tests = 200;

# Build current versions of all the timing tools...
build("metime") or die "$prog: cannot build 'metime': $!\n";
build("multime") or die "$prog: cannot build 'multime': $!\n";
build("primegen") or die "$prog: cannot build 'primegen': $!\n";
build("pi") or die "$prog: cannot build 'pi': $!\n";

$ENV{'SEED'} = $SEED;

printf ("- Running multiplication timing tests (%d):\n", 
	$mt_tests);

foreach $size (@mt_sizes) {
    printf ("%4d bits: ", $size);
    ($total, $each) = run_mtime("multime", $mt_tests, $size);
    printf ("%s total, %s each\n", $total, $each);
}

printf ("- Running modular exponentiation timing tests (%d):\n", 
	$me_tests);

foreach $size (@me_sizes) {
    printf ("%4d bits: ", $size);
    ($total, $each) = run_mtime("metime", $me_tests, $size);
    printf ("%s total, %s each\n", $total, $each);
}

printf ("- Running prime generation timing tests (%d):\n",
	$pg_tests);

foreach $size (@pg_sizes) {
    printf ("%4d bits: ", $size);
    @samples = run_primegen($pg_tests, $size);

    %stats = compute_stats(@samples);
    printf ("min=%.2f, avg=%.2f, max=%.2f, total=%.2f\n",
	    $stats{"min"}, $stats{"avg"}, $stats{"max"},
	    $stats{"sum"});
}

exit 0;

#------------------------------------------------------------------------
# S U B R O U T I N E S
#========================================================================

sub compute_stats {
    my(@samples) = sort { $a <=> $b } @_;
    my(%stats, $sum, $num);
    
    $num = @samples;

    $stats{"num"} = $num;
    $stats{"min"} = $samples[0];
    $stats{"max"} = $samples[$#samples];

    $sum = 0.0;
    foreach (@samples) { $sum += $_; }

    $stats{"sum"} = $sum;
    $stats{"avg"} = $sum / $num;

    %stats;
}

sub run_primegen {
    my($ntests, $bits) = @_;
    my(@res);

    open(IFP, "./primegen $bits $ntests|") or return ();
    @res = map { /\((.+) seconds\)$/; 0.0+$1; } grep { /^This/ } <IFP>;
    close(IFP);

    @res;
}

sub run_mtime {
    my($which, $ntests, $bits) = @_;
    my(@res);

    open(IFP, "./$which $ntests $bits|") or return ();
    @res = grep { /^(Total|Individual):/ } map { chomp; $_; } <IFP>;
    close(IFP);

    map { (split(/\s+/, $_))[1]; } @res;
}

sub build {
    my($pname) = shift;

    return 0 unless system("make", $pname) == 0;

    return (-e $pname && -f _ && -x _);
}

#------------------------------------------------------------------------
# HERE THERE BE DRAGONS
