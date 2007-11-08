

// Test program and examples for the C++ extension of the MPI library
//   by scroussette@yahoo.com

#include <stdio.h>
#include <iostream.h>

// extern C is needed if mpi.c is compiled as a C file and not as
// a C++ file
extern "C"                    
{
    #include "mpi.h"
}

#include "mpicpp.h"

void testallfunctions(void);

void main(void) {
	// Init 'a' in decimal
	Mpi a("10000001");
	// Init 'b' with 100 in decimal or 144 in octal
	Mpi b("144",8),d;
	Mpi c, *p, *q;
	int j;
	mp_digit digit;

	cout << "a=" << a << " b=" << b;

	if (a==b)
		cout << " a==b"<< endl;
	else
		cout << " a!=b"<<endl;

	// Assign a value to d in decimal
	d="123456789012345678901234567890";
	cout << "d=" << d << endl;

	// make arithmetic operations
	c=a+b;
	cout << "c=a+b=" << c << endl;
	j=(c+1)%3;
	cout << (c+1) << " mod 3=" << j << endl;
	d=a%b;
	cout << "c%b =" << d << endl;
	d=a-b;
	cout << "a-b=" << d << endl;
	d=c/b;
	cout << "c/b=" << d << endl;
	d=-c/b;
	cout << "-c/b=" << d << endl;

	if (a%2==0)
		cout << a << " is even"<<endl;
	else
		cout << a << " is odd"<<endl;

	mp_mod(a, b, c);
	if (c.err==MP_OKAY)
		cout << a << " mod " << b << " = " << c << endl;
	else
		cout << "error"<<endl;

	digit=a[1];
	// it would be nice if we had a global variable with a default 
	// radix for the conversions, like "s_mp_defradix", and 
	// functions to read and set this value.  We wouldn't have
	// to use the setf function of the stream.
	cout.setf(ios::hex);
	cout << "digit 1 of 0x" << a  <<	"=0x" << digit << endl;
	cout.setf(ios::dec);

	// exemple on how to initialize a new object in hexadecimal
	p=new Mpi("c7ae466bcafa7c994480ade2e2bd4d121840a000",16);
	q=new Mpi("8eeae81b84c7f27e080fde64ff05254000000000",16);
	// and divide
	if (p->err==MP_OKAY && q->err==MP_OKAY)
		if (mp_div(*p,*q,a,b)==MP_OKAY)
			cout <<*p << " div " << *q << " = " << a << " rem=" << b << endl;

	// test exponentiation operator
	a="25";
	b="26";
	c=a^b;
	cout << a << " exp " << b << "=" << c << endl;
	c=a^3;
	cout << a << " exp " << 3 << "=" << c << endl;

	cout << endl;


	testallfunctions();

}

// This is simply to check that all functions run correctly
void testallfunctions(void) {
	Mpi mp, mp1, mp2;
	Mpi from, to;
	Mpi a,b,c,g,m,x,y;
	mp_digit d,r,k;
	char str[500];
	int radix,len;
	long z;
	mp_size prec;

	d=r=k=1;
	radix=10;
	len=5;
	z=2;
	prec=3;

	// The Mpi objects are always initialized by the constructors
	// when declared.
//	mp_init(mp);										
//	mp_init_size(mp, prec);                   
//	mp_init_copy(mp, from);

	// To test mp_clear:
	Mpi p;
	mp_clear(p);                                  

	mp_copy(from, to);                           
	mp_exch(mp1, mp2);                        
	mp_zero(mp);                                   
	mp_set(mp, d);                        
	mp_set_int(mp, z);                           


	mp_add_d(a, d, b);                  
	mp_sub_d(a, d, b);                  
	mp_mul_d(a, d, b);                  
	mp_mul_2(a, c);                              
	mp_div_d(a, d, c, &r);     
	mp_div_2(a, c);                              
	mp_expt_d(a, d, c);                 


	mp_abs(a, b);                                
	mp_neg(a, b);                                


	mp_add(a, b, c);                        
	mp_sub(a, b, c);                        
	mp_mul(a, b, c);                        
#if MP_SQUARE
	mp_sqr(a, b);                                
#endif
	mp_div(a, b, x, y);                
	mp_div_2d(a, d, x, y);         
	mp_expt(a, b, c);                       
	mp_2expt(a, k);                          
	mp_sqrt(a, b);                               


#if MP_MODARITH
	mp_mod(a, m, c);                        
	mp_mod_d(a, d, &r);             
	mp_addmod(a, b, m, c);             
	mp_submod(a, b, m, c);             
	mp_mulmod(a, b, m, c);             
#if MP_SQUARE
	mp_sqrmod(a, m, c);                     
#endif
	mp_exptmod(a, b, m, c);            
	mp_exptmod_d(a, d, m, c);      
#endif 


	mp_cmp_z(a);                                   
	mp_cmp_d(a, d);                       
	mp_cmp(a, b);                             
	mp_cmp_mag(a, b);                         
	mp_cmp_int(a, z);                         
	mp_isodd(a);                                   
	mp_iseven(a);                                  


#if MP_NUMTH
	mp_gcd(a, b, c);                        
	mp_lcm(a, b, c);                        
	mp_xgcd(a, b, g, x, y);       
	mp_invmod(a, m, c);                     
#endif


#if MP_IOFUNC
	FILE *ofp;
	ofp=fopen( "testcpp.txt", "w");
	if (ofp!=NULL) {
		mp_print(mp, ofp);
		fclose(ofp);
	}
#endif


	mp_read_raw(mp, str, len);              
	mp_raw_size(mp);                               
	mp_toraw(mp, str);                          
	mp_read_radix(mp, str, radix);          
	mp_radix_size(mp, radix);                  
	mp_toradix(mp, str, radix);             


}
