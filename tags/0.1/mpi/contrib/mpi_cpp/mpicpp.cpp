


/**************************

  This is the C++ interface function file for the mpi library
   by scroussette@yahoo.com

 *************************/


// extern C is needed if mpi.c is compiled as a C file and not as
// a C++ file
extern "C"                    
{
    #include "mpi.h"
}

#include "mpicpp.h"

// this is a buffer used by the << output stream operator below:
char iobuf[50000];

ostream &operator<<(ostream &s, Mpi &a)
{
	long flags;
	mp_word base;
	flags=s.flags();
	// curently, only bases 8,10,16 are supported. If we had a 
	// global variable with a default radix for the conversions, 
	// we could support all of them.
	if (flags & ios::hex)
		base=16;
	else {
		if (flags & ios::oct)
			base=8;
		else
			base=10;
	}
	a.err=mp_toradix(&(a.mpi_n),(char *)iobuf,base);
	s << (char *)iobuf; 
	return s;
}


char *operator<<(char *s,Mpi &a)
{
	// Same here. Because there is no way to specify a radix, 
	// we use 10 by default
    a.err=mp_toradix(&(a.mpi_n),s,10);
    return s;
}

// Operateur negation a=(-a);
Mpi operator-(Mpi &a){
	Mpi z(a);
	if (z.err==MP_OKAY) {
		if(SIGN(&(z.mpi_n))==NEG)	SIGN(&(z.mpi_n))=ZPOS;
		else						SIGN(&(z.mpi_n))=NEG;
	}
	return z;
}

// z=a+i
Mpi operator+(Mpi &a, mp_digit i){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_add_d(&(a.mpi_n),i,&(z.mpi_n)); 
	return z;
}

Mpi operator+(mp_digit i, Mpi &a){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_add_d(&(a.mpi_n),i,&(z.mpi_n)); 
	return z;
}

Mpi operator+(Mpi &a, Mpi &b){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_add(&(a.mpi_n),&(b.mpi_n),&(z.mpi_n)); 
	return z;
}

Mpi operator-(Mpi &a, mp_digit i){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_sub_d(&(a.mpi_n),i,&(z.mpi_n)); 
	return z;
}

Mpi operator-(mp_digit i, Mpi &a){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_sub_d(&(a.mpi_n),i,&(z.mpi_n)); 
	return z;
}

Mpi operator-(Mpi &a, Mpi &b){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_sub(&(a.mpi_n),&(b.mpi_n),&(z.mpi_n)); 
	return z;
}

Mpi operator*(Mpi &a, mp_digit i){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_mul_d(&(a.mpi_n),i,&(z.mpi_n)); 
	return z;
}

Mpi operator*(mp_digit i, Mpi &a){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_mul_d(&(a.mpi_n),i,&(z.mpi_n)); 
	return z;
}

Mpi operator*(Mpi &a, Mpi &b){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_mul(&(a.mpi_n),&(b.mpi_n),&(z.mpi_n)); 
	return z;
}

Mpi operator/(Mpi &a, mp_digit i){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_div_d(&(a.mpi_n),i,&(z.mpi_n),NULL); 
	return z;
}

Mpi operator/(Mpi &a, Mpi &b){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_div(&(a.mpi_n),&(b.mpi_n),&(z.mpi_n),NULL); 
	return z;
}

// Mod operator: use divide or mp_mod_d?
mp_digit operator%(Mpi &a, mp_digit i){
	mp_digit j=0;
	// Here, to detect a divide error, the program should check the
	// "err" member variable in the object "a".
	a.err=mp_div_d(&(a.mpi_n),i,NULL,&j); 
	return j;
}

// Mod operator: use divide or mp_mod?
Mpi operator%(Mpi &a, Mpi &b){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_div(&(a.mpi_n),&(b.mpi_n),NULL,&(z.mpi_n)); 
	return z;
}

// Exponentiation:
Mpi operator^(Mpi &a, Mpi &b){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_expt(&(a.mpi_n),&(b.mpi_n),&(z.mpi_n)); 
	return z;
}

Mpi operator^(Mpi &a, mp_digit b){
	Mpi z;
	if (z.err==MP_OKAY)
		z.err=mp_expt_d(&(a.mpi_n),b,&(z.mpi_n)); 
	return z;
}

