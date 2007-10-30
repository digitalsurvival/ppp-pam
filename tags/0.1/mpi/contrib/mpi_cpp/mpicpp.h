

/**************************

  This is the C++ interface header file for the mpi library
   by scroussette@yahoo.com

 *************************/

#ifndef MPICPP_H
#define MPICPP_H

#include <stdlib.h>
#include <stdio.h>
#include <iostream.h>

typedef int		mp_bool;
#define MP_EQ	0
#define MP_GT	1
#define MP_LT	-1
#define TRUE	1
#define FALSE	0

// extern C is needed if mpi.c is compiled as a C file and not as
// a C++ file
extern "C"                    
{
    #include "mpi.h"
}

// This the class definition for an object of mp_int type

class Mpi
{
    // Private mpi object accessible only via member
	// functions or friend functions
    mp_int mpi_n;

public:
	// This public member variable will contain the result of the last 
	// operation performed on this object.  If it's other than MP_OKAY
	// then there was an error.
	mp_err err;

	// The constructors
	Mpi(void)					{err=mp_init(&mpi_n);} 
    Mpi(mp_digit i)				{if ((err=mp_init(&mpi_n))==MP_OKAY) mp_set(&mpi_n,i);}  
    Mpi(mp_int *a)				{err=mp_init_copy(&mpi_n,a);} 
    Mpi(Mpi &a)					{err=mp_init_copy(&mpi_n, &(a.mpi_n));} 
    Mpi(char *s,mp_word base=10) 
			{	err=mp_init(&mpi_n);
				if(err==MP_OKAY) err=mp_read_radix(&mpi_n,s,base);
			}

	// The overloaded operators
    Mpi& operator=(mp_digit i)	{mp_set(&mpi_n,i); return *this;}
    Mpi& operator=(Mpi &a)		{err=mp_copy(&(a.mpi_n),&mpi_n); return *this;}
    Mpi& operator=(mp_int *a)	{err=mp_copy(a,&mpi_n); return *this;}
	// for mp_read_radix we use base 10 by default
    Mpi& operator=(char* s)		{err=mp_read_radix(&mpi_n,s,10);return *this;}

    Mpi& operator++()			{err=mp_add_d(&mpi_n,1,&mpi_n); return *this;}
    Mpi& operator+=(mp_digit i)	{err=mp_add_d(&mpi_n,i,&mpi_n); return *this;}
    Mpi& operator+=(mp_int *a)	{err=mp_add(&mpi_n,a,&mpi_n); return *this;}
    Mpi& operator+=(Mpi &a)		{err=mp_add(&mpi_n,&(a.mpi_n),&mpi_n); return *this;}

    Mpi& operator--()			{err=mp_sub_d(&mpi_n,1,&mpi_n); return *this;}
    Mpi& operator-=(mp_digit i)	{err=mp_sub_d(&mpi_n,i,&mpi_n); return *this;}
    Mpi& operator-=(mp_int *a)	{err=mp_sub(&mpi_n,a,&mpi_n); return *this;}
    Mpi& operator-=(Mpi &a)		{err=mp_sub(&mpi_n,&(a.mpi_n),&mpi_n); return *this;}

    Mpi& operator*=(mp_digit i) {err=mp_mul_d(&mpi_n,i,&mpi_n); return *this;}
    Mpi& operator*=(mp_int *i)  {err=mp_mul(&mpi_n,i,&mpi_n); return *this;}
    Mpi& operator*=(Mpi &i)		{err=mp_mul(&mpi_n,&(i.mpi_n),&mpi_n); return *this;}

    Mpi& operator/=(mp_digit i) {err=mp_div_d(&mpi_n,i,&mpi_n,NULL);return *this;}
    Mpi& operator/=(mp_int *i)  {err=mp_div(&mpi_n,i,&mpi_n,NULL); return *this;}
    Mpi& operator/=(Mpi &i)		{err=mp_div(&mpi_n,&(i.mpi_n),&mpi_n,NULL); return *this;}

    Mpi& operator%=(mp_digit i)	{	
				mp_digit a; 
				if ((err==mp_div_d(&mpi_n,i,NULL,&a))==MP_OKAY) 
					mp_set(&mpi_n,a);
				return *this;
			}
    Mpi& operator%=(mp_int *i)	{err=mp_div(&mpi_n,i,NULL,&mpi_n);return *this;}
    Mpi& operator%=(Mpi &i)		{err=mp_div(&mpi_n,&(i.mpi_n),NULL,&mpi_n); return *this;}

	mp_digit operator[](mp_word i){return DIGIT(&mpi_n,i);};

	// Negation operator: x = -a
    friend Mpi operator-(Mpi &a);

    friend Mpi operator+(Mpi &a,	mp_digit i);
    friend Mpi operator+(mp_digit i, Mpi &a);
    friend Mpi operator+(Mpi&a,		Mpi&b);

    friend Mpi operator-(Mpi &a,	mp_digit i);
    friend Mpi operator-(mp_digit i, Mpi &a);
    friend Mpi operator-(Mpi&a,		Mpi&b);

    friend Mpi operator*(Mpi &a,	mp_digit i);
    friend Mpi operator*(mp_digit i, Mpi &a);
    friend Mpi operator*(Mpi&a,		Mpi&b);

    friend Mpi operator/(Mpi &a,	mp_digit i);
    friend Mpi operator/(Mpi&a,		Mpi&b);
    
    friend mp_digit operator%(Mpi &a,	mp_digit i);
    friend Mpi operator%(Mpi&a,		Mpi&b);

	// Exponentiation:
    friend Mpi operator^(Mpi&a,		Mpi&b);
    friend Mpi operator^(Mpi&a,		mp_digit b);


	// Comparison operators
    friend mp_bool operator<=(Mpi &a, Mpi &b){
		if (mp_cmp(&(a.mpi_n),&(b.mpi_n))<=MP_EQ)
			return TRUE;
		else 
			return FALSE;
	}
    friend mp_bool operator>=(Mpi &a, Mpi &b){
		if (mp_cmp(&(a.mpi_n),&(b.mpi_n))>=MP_EQ)
			return TRUE;
		else 
			return FALSE;
	}
    friend mp_bool operator==(Mpi &a, Mpi &b){
		if (mp_cmp(&(a.mpi_n),&(b.mpi_n))==MP_EQ)
			return TRUE;
		else 
			return FALSE;
	}
    friend mp_bool operator!=(Mpi &a, Mpi &b){
		if (mp_cmp(&(a.mpi_n),&(b.mpi_n))!=MP_EQ)
			return TRUE;
		else 
			return FALSE;
	}
    friend mp_bool operator<(Mpi &a, Mpi &b){
		if (mp_cmp(&(a.mpi_n),&(b.mpi_n))==MP_LT)
			return TRUE;
		else 
			return FALSE;
	}
    friend mp_bool operator>(Mpi &a, Mpi &b){
		if (mp_cmp(&(a.mpi_n),&(b.mpi_n))==MP_GT)
			return TRUE;
		else 
			return FALSE;
	}
    friend mp_bool operator<=(Mpi &a, mp_digit b){
		if (mp_cmp_d(&(a.mpi_n),b)<=MP_EQ)
			return TRUE;
		else 
			return FALSE;
	}
    friend mp_bool operator>=(Mpi &a, mp_digit b){
		if (mp_cmp_d(&(a.mpi_n),b)>=MP_EQ)
			return TRUE;
		else 
			return FALSE;
	}
    friend mp_bool operator==(Mpi &a, mp_digit b){
		if (mp_cmp_d(&(a.mpi_n),b)==MP_EQ)
			return TRUE;
		else 
			return FALSE;
	}
    friend mp_bool operator!=(Mpi &a, mp_digit b){
		if (mp_cmp_d(&(a.mpi_n),b)!=MP_EQ)
			return TRUE;
		else 
			return FALSE;
	}
    friend mp_bool operator<(Mpi &a, mp_digit b){
		if (mp_cmp_d(&(a.mpi_n),b)==MP_LT)
			return TRUE;
		else 
			return FALSE;
	}
    friend mp_bool operator>(Mpi &a, mp_digit b){
		if (mp_cmp_d(&(a.mpi_n),b)==MP_GT)
			return TRUE;
		else 
			return FALSE;
	}
	// send the Mpi to a character string
    friend char * operator<<(char * s,Mpi &a);
	//		or to a stream
	friend ostream &operator<<(ostream &s, Mpi &a);

	//*********
	// The other functions


friend mp_err mp_init(Mpi &mp)										{return (mp.err=  mp_init(&(mp.mpi_n)));};
friend mp_err mp_init_size(Mpi &mp, mp_size prec)                   {return (mp.err=  mp_init_size(&(mp.mpi_n), prec));};
friend mp_err mp_init_copy(Mpi &mp, Mpi &from)                      {return (mp.err=  mp_init_copy(&(mp.mpi_n), &(from.mpi_n)));};
friend mp_err mp_copy(Mpi &from, Mpi &to)                           {return (to.err=  mp_copy(&(from.mpi_n), &(to.mpi_n)));};
friend	void   mp_exch(Mpi &mp1, Mpi &mp2)                          {				  mp_exch(&(mp1.mpi_n), &(mp2.mpi_n));};
friend	void   mp_clear(Mpi &mp)                                    {				  mp_clear(&(mp.mpi_n));};
friend	void   mp_zero(Mpi &mp)                                     {				  mp_zero(&(mp.mpi_n));};
friend	void   mp_set(Mpi &mp, mp_digit d)                          {				  mp_set(&(mp.mpi_n), d);};
friend mp_err mp_set_int(Mpi &mp, long z)                           {return (mp.err=  mp_set_int(&(mp.mpi_n),z));};


friend mp_err mp_add_d(Mpi &a, mp_digit d, Mpi &b)                  {return (b.err=  mp_add_d(&(a.mpi_n),  d, &(b.mpi_n)));};
friend mp_err mp_sub_d(Mpi &a, mp_digit d, Mpi &b)                  {return (b.err=  mp_sub_d(&(a.mpi_n),  d, &(b.mpi_n)));};
friend mp_err mp_mul_d(Mpi &a, mp_digit d, Mpi &b)                  {return (b.err=  mp_mul_d(&(a.mpi_n),  d, &(b.mpi_n)));};
friend mp_err mp_mul_2(Mpi &a, Mpi &c)                              {return (c.err=  mp_mul_2(&(a.mpi_n), &(c.mpi_n)));};
friend mp_err mp_div_d(Mpi &a, mp_digit d, Mpi &q, mp_digit *r)     {return (q.err=  mp_div_d(&(a.mpi_n),  d, &(q.mpi_n), r));};
friend mp_err mp_div_2(Mpi &a, Mpi &c)                              {return (c.err=  mp_div_2(&(a.mpi_n), &(c.mpi_n)));};
friend mp_err mp_expt_d(Mpi &a, mp_digit d, Mpi &c)                 {return (c.err=  mp_expt_d(&(a.mpi_n),  d, &(c.mpi_n)));};


friend mp_err mp_abs(Mpi &a, Mpi &b)                                {return (b.err=  mp_abs(&(a.mpi_n), &(b.mpi_n)));};
friend mp_err mp_neg(Mpi &a, Mpi &b)                                {return (b.err=  mp_neg(&(a.mpi_n), &(b.mpi_n)));};


friend mp_err mp_add(Mpi &a, Mpi &b, Mpi &c)                        {return (c.err=  mp_add(&(a.mpi_n), &(b.mpi_n), &(c.mpi_n)));};
friend mp_err mp_sub(Mpi &a, Mpi &b, Mpi &c)                        {return (c.err=  mp_sub(&(a.mpi_n), &(b.mpi_n), &(c.mpi_n)));};
friend mp_err mp_mul(Mpi &a, Mpi &b, Mpi &c)                        {return (c.err=  mp_mul(&(a.mpi_n), &(b.mpi_n), &(c.mpi_n)));};
#if MP_SQUARE
friend mp_err mp_sqr(Mpi &a, Mpi &b)                                {return (b.err=  mp_sqr(&(a.mpi_n), &(b.mpi_n)));};
#endif
friend mp_err mp_div(Mpi &a, Mpi &b, Mpi &q, Mpi &r)                {return (q.err=  mp_div(&(a.mpi_n), &(b.mpi_n), &(q.mpi_n), &(r.mpi_n)));};
friend mp_err mp_div_2d(Mpi &a, mp_digit d, Mpi &q, Mpi &r)         {return (q.err=  mp_div_2d(&(a.mpi_n),  d, &(q.mpi_n), &(r.mpi_n)));};
friend mp_err mp_expt(Mpi &a, Mpi &b, Mpi &c)                       {return (c.err=  mp_expt(&(a.mpi_n), &(b.mpi_n), &(c.mpi_n)));};
friend mp_err mp_2expt(Mpi &a, mp_digit k)                          {return (a.err=  mp_2expt(&(a.mpi_n),  k));};
friend mp_err mp_sqrt(Mpi &a, Mpi &b)                               {return (b.err=  mp_sqrt(&(a.mpi_n), &(b.mpi_n)));};


#if MP_MODARITH
friend mp_err mp_mod(Mpi &a, Mpi &m, Mpi &c)                        {return (c.err=  mp_mod(&(a.mpi_n), &(m.mpi_n), &(c.mpi_n)));};
friend mp_err mp_mod_d(Mpi &a, mp_digit d, mp_digit *c)             {return (a.err=  mp_mod_d(&(a.mpi_n),  d, c));};
friend mp_err mp_addmod(Mpi &a, Mpi &b, Mpi &m, Mpi &c)             {return (c.err=  mp_addmod(&(a.mpi_n), &(b.mpi_n), &(m.mpi_n), &(c.mpi_n)));};
friend mp_err mp_submod(Mpi &a, Mpi &b, Mpi &m, Mpi &c)             {return (c.err=  mp_submod(&(a.mpi_n), &(b.mpi_n), &(m.mpi_n), &(c.mpi_n)));};
friend mp_err mp_mulmod(Mpi &a, Mpi &b, Mpi &m, Mpi &c)             {return (c.err=  mp_mulmod(&(a.mpi_n), &(b.mpi_n), &(m.mpi_n), &(c.mpi_n)));};
#if MP_SQUARE
friend mp_err mp_sqrmod(Mpi &a, Mpi &m, Mpi &c)                     {return (c.err=  mp_sqrmod(&(a.mpi_n), &(m.mpi_n), &(c.mpi_n)));};
#endif
friend mp_err mp_exptmod(Mpi &a, Mpi &b, Mpi &m, Mpi &c)            {return (c.err=  mp_exptmod(&(a.mpi_n), &(b.mpi_n), &(m.mpi_n), &(c.mpi_n)));};
friend mp_err mp_exptmod_d(Mpi &a, mp_digit d, Mpi &m, Mpi &c)      {return (c.err=  mp_exptmod_d(&(a.mpi_n),  d, &(m.mpi_n), &(c.mpi_n)));};
#endif 


friend	int    mp_cmp_z(Mpi &a)                                     {return mp_cmp_z(&(a.mpi_n));};
friend	int    mp_cmp_d(Mpi &a, mp_digit d)                         {return mp_cmp_d(&(a.mpi_n),  d);};
friend	int    mp_cmp(Mpi &a, Mpi &b)                               {return mp_cmp(&(a.mpi_n), &(b.mpi_n));};
friend	int    mp_cmp_mag(Mpi &a, Mpi &b)                           {return mp_cmp_mag(&(a.mpi_n), &(b.mpi_n));};
friend	int    mp_cmp_int(Mpi &a, long z)                           {return mp_cmp_int(&(a.mpi_n),z);};
friend	int    mp_isodd(Mpi &a)                                     {return mp_isodd(&(a.mpi_n));};
friend	int    mp_iseven(Mpi &a)                                    {return mp_iseven(&(a.mpi_n));};


#if MP_NUMTH
friend mp_err mp_gcd(Mpi &a, Mpi &b, Mpi &c)                        {return (c.err=  mp_gcd(&(a.mpi_n), &(b.mpi_n), &(c.mpi_n)));};
friend mp_err mp_lcm(Mpi &a, Mpi &b, Mpi &c)                        {return (c.err=  mp_lcm(&(a.mpi_n), &(b.mpi_n), &(c.mpi_n)));};
friend mp_err mp_xgcd(Mpi &a, Mpi &b, Mpi &g, Mpi &x, Mpi &y)       {return (g.err=  mp_xgcd(&(a.mpi_n), &(b.mpi_n), &(g.mpi_n), &(x.mpi_n), &(y.mpi_n)));};
friend mp_err mp_invmod(Mpi &a, Mpi &m, Mpi &c)                     {return (c.err=  mp_invmod(&(a.mpi_n), &(m.mpi_n), &(c.mpi_n)));};
#endif


#if MP_IOFUNC
friend	void   mp_print(Mpi &mp, FILE *ofp)                         {return mp_print(&(mp.mpi_n),ofp);};
#endif


friend mp_err mp_read_raw(Mpi &mp, char *str, int len)              {return (mp.err=  mp_read_raw(&(mp.mpi_n),str,len));};
friend	int    mp_raw_size(Mpi &mp)                                 {return			  mp_raw_size(&(mp.mpi_n));};
friend mp_err mp_toraw(Mpi &mp, char *str)                          {return (mp.err=  mp_toraw(&(mp.mpi_n),str));};
friend mp_err mp_read_radix(Mpi &mp, char *str, int radix)          {return (mp.err=  mp_read_radix(&(mp.mpi_n),str,radix));};
friend	int    mp_radix_size(Mpi &mp, int radix)                    {return (mp.err=  mp_radix_size(&(mp.mpi_n),radix));};
friend mp_err mp_toradix(Mpi &mp, char *str, int radix)             {return (mp.err=  mp_toradix(&(mp.mpi_n),str,radix));};


	// The destructor: clear the mpi
    ~Mpi() { mp_clear(&mpi_n); }
};


#endif

