#!/usr/bin/perl

#
# types.pl - find recommended type definitions for digits and words
#
# by Michael J. Fromberger <sting@linguist.dartmouth.edu>
# Copyright (C) 2000 Michael J. Fromberger, All Rights Reserved
#
# This script scans the Makefile for the C compiler and compilation
# flags currently in use, and using this combination, attempts to
# compile a simple test program that outputs the sizes of the various
# unsigned integer types, in bytes.  Armed with these, it finds all
# the "viable" type combinations for mp_digit and mp_word, where
# viability is defined by the requirement that mp_word be at least two
# times the precision of mp_digit.
#
# Of these, the one with the largest digit size is chosen, and
# appropriate typedef statements are written to standard output.
# 
# $Id: types.pl,v 1.1 2004/02/08 04:29:30 sting Exp $
#

@_=split(/\//,$0);chomp($prog=pop(@_));

$target_size = shift(@ARGV);

# The array of integer types to be considered...
@TYPES = ( 
	   "unsigned char", 
	   "unsigned short", 
	   "unsigned int", 
	   "unsigned long",
#	   "uint64_t",
);

# Macro names for the maximum unsigned value of each type
%TMAX = ( 
	  "unsigned char"   => "UCHAR_MAX",
	  "unsigned short"  => "USHRT_MAX",
	  "unsigned int"    => "UINT_MAX",
	  "unsigned long"   => "ULONG_MAX",
#	  "uint64_t"        => "UINT64_MAX",
);

# Include these headers...
@INCL = ( "<stdio.h>", 
#	  "<inttypes.h>",
);

# Read the Makefile to find out which C compiler to use
# open(MFP, "<Makefile.base") or die "$prog: Makefile.base: $!\n";
# while(<MFP>) {
#     chomp;
#     if(/^CC=(.*)$/) {
# 	$cc = $1;
# 	last if $cflags;
#     } elsif(/^CFLAGS=(.*)$/) {
# 	$cflags = $1;
# 	last if $cc;
#     }
# }
# close(MFP);
$cc = $ARGV[0];
$cflags = $ARGV[1];

# If we couldn't find that, use 'cc' by default
$cc = "cc" unless $cc;

printf STDERR "Using '%s' as the C compiler.\n", $cc;
printf STDERR ("Using '%s' as options.\n", $cflags) if $cflags;

print STDERR "Determining type sizes ... \n";
$SIG{'INT'} = 'IGNORE';
$SIG{'HUP'} = 'IGNORE';
open(OFP, ">tc$$.c") or die "$prog: tc$$.c: $!\n";
print OFP map { sprintf("#include %s\n", $_) } @INCL;
print OFP "\nint main(void)\n{\n";
foreach $type (@TYPES) {
    printf OFP "\tprintf(\"%%d\\n\", (int)sizeof(%s));\n", $type;
}
print OFP "\n\treturn 0;\n}\n";
close(OFP);

system("$cc $cflags -o tc$$ tc$$.c");

die "$prog: unable to build test program\n" unless(-x "tc$$");

open(IFP, "./tc$$|") or die "$prog: can't execute test program\n";
$ix = 0;
while(<IFP>) {
    chomp;
    $size{$TYPES[$ix++]} = $_;
}
close(IFP);

unlink("tc$$");
unlink("tc$$.c");

$SIG{'INT'} = 'DEFAULT';
$SIG{'HUP'} = 'DEFAULT';

print STDERR "Selecting viable combinations ... \n";
while(($type, $size) = each(%size)) {
    push(@ts, [ $size, $type ]);
}

# Sort them ascending by size 
@ts = sort { $a->[0] <=> $b->[0] } @ts;

# Try all possible combinations, finding pairs in which the word size
# is twice the digit size.  The number of possible pairs is too small
# to bother doing this more efficiently than by brute force
for($ix = 0; $ix <= $#ts; $ix++) {
    $w = $ts[$ix];

    for($jx = 0; $jx <= $#ts; $jx++) {
	$d = $ts[$jx];

	if($w->[0] == 2 * $d->[0]) {
	    push(@valid, [ $d, $w ]);
	}
    }
}

# Sort descending by digit size
@valid = sort { $b->[0]->[0] <=> $a->[0]->[0] } @valid;

# Select the maximum value which has the requested digit size, as the
# recommended combination
do {
    $rec = shift(@valid);
    
    # If there's no way to fulfil the user's request, take the biggest
    # thing we can get...
    $target_size = $rec->[0]->[0] if($rec->[0]->[0] < $target_size);
} while($target_size && $rec->[0]->[0] != $target_size && $#valid >= 0);

print ("/* Type definitions generated by 'types.pl' */\n\n");
printf("typedef %-18s mp_sign;\n", "char");
printf("typedef %-18s mp_digit;  /* %d byte type */\n", 
       $rec->[0]->[1], $rec->[0]->[0]);
printf("typedef %-18s mp_word;   /* %d byte type */\n", 
       $rec->[1]->[1], $rec->[1]->[0]);
printf("typedef %-18s mp_size;\n", "unsigned int");
printf("typedef %-18s mp_err;\n\n", "int");

printf("#define %-18s (CHAR_BIT*sizeof(mp_digit))\n", "MP_DIGIT_BIT");
printf("#define %-18s %s\n", "MP_DIGIT_MAX", $TMAX{$rec->[0]->[1]});
printf("#define %-18s (CHAR_BIT*sizeof(mp_word))\n", "MP_WORD_BIT");
printf("#define %-18s %s\n\n", "MP_WORD_MAX", $TMAX{$rec->[1]->[1]});
printf("#define %-18s (MP_DIGIT_MAX+1)\n\n", "RADIX");

printf("#define %-18s %d\n", "MP_DIGIT_SIZE", $rec->[0]->[0]);
printf("#define %-18s \"%%0%dX\"\n", "DIGIT_FMT", (2 * $rec->[0]->[0]));

exit 0;
