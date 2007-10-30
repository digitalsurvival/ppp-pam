#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>

#include "mpi.h"
#include "mprsa.h"

char *g_message = "You can't keep a secret for the life of you.";
char *g_modulus = 
"83274231253085465096943841678296886335681962490673038705724389198"
"65474669167807962621458902263032323334899267161962205385173787115"
"612127292324765919274701";
char *g_e = "65537";
char *g_d = 
"19960555087274662728045086118137323762870255714573184079653088025"
"41567993316053323488373760710861512967149119329674542725895098992"
"464916732941604352939501";

void myrand(char *out, int len);

int main(int argc, char *argv[])
{
  mp_int  modulus, e, d;
  char   *out, *ptx;
  int     olen;
  mp_err  res;

  srand(time(NULL));

  mp_init(&modulus);
  mp_init(&e);
  mp_init(&d);

  mp_read_radix(&modulus, g_modulus, 10);
  mp_read_radix(&e, g_e, 10);
  mp_read_radix(&d, g_d, 10);
  
  printf("Maximum message length: %d\n\n", 
	 mp_pkcs1v15_maxlen(&modulus));

  printf("Encrypting message, %d bytes:\n\t\"%s\"\n", 
	 strlen(g_message),
	 g_message);
  res = mp_pkcs1v15_encrypt(g_message, strlen(g_message),
			    &e, &modulus, &out, &olen, myrand);
  printf("Result:  %s\n", mp_strerror(res));

  if(res != MP_OKAY)
    goto CLEANUP;
  else {
    int  ix, brk;

    printf("Encrypted message length: %d\n", olen);
    printf("Packet data:\n");
    for(ix = 0; ix < olen; ix++) {
      printf("%02X ", out[ix]);
      ++brk;
      if(brk >= 16) {
	fputc('\n', stdout);
	brk = 0;
      }
    }
  }

  printf("\nDecrypting message ... \n");
  res = mp_pkcs1v15_decrypt(out, olen, 
			    &d, &modulus, &ptx, &olen);
  printf("Result:  %s\n", mp_strerror(res));

  if(res != MP_OKAY) {
    free(out);
    goto CLEANUP;
  } else {
    int  ix;

    printf("Decrypted message length: %d\n", olen);
    printf("Message data:\n");
    for(ix = 0; ix < olen; ix++) {
      fputc(ptx[ix], stdout);
    }
    fputc('\n', stdout);
    free(ptx);
  }

  free(out);

 CLEANUP:
  mp_clear(&d);
  mp_clear(&e);
  mp_clear(&modulus);

  return 0;
}

void myrand(char *out, int len)
{
  int  ix, val;

  for(ix = 0; ix < len; ix++) {
    do {
      val = rand() % UCHAR_MAX;
    } while(val == 0);
    out[ix] = val;
  }
}
