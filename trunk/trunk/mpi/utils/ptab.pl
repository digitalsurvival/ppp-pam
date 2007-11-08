#!/usr/local/bin/perl

if(-f "mpi.h") {
    open(MPIH, "<mpi.h");
    while(<MPIH>) {
	chomp; 

	if(/^\s*\#define\s+DIGIT_FMT\s+\"(\S+)\"\s*$/) {
	    $format = $1;
	    last;
	}
    }
    close(MPIH);
}
$format = "%04X" unless $format;

while(<>) {
    chomp;
    push(@primes, $_);
}

printf("int       prime_tab_size = %d;\n", ($#primes + 1));
print ("mp_digit  prime_tab[] = {\n");

print "\t";
$last = pop(@primes);
$format = sprintf("0x%s", $format);
foreach $prime (sort {$a<=>$b} @primes) {
    printf($format . ", ", $prime);
    $brk = ($brk + 1) % 8;
    print "\n\t" if(!$brk);
}
printf($format, $last);
print "\n" if($brk);
print "};\n\n";

exit 0;
