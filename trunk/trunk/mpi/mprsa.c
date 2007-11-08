/*
  mprsa.c

  Simple implementation of RSA encryption and signing based on the
  PKCS #1, v. 1.5 encoding standard.  Uses the MPI library for the
  mathematical computations.

  by Michael J. Fromberger <http://www.dartmouth.edu/~sting/>
  Copyright (C) 2001 Michael J. Fromberger, All Rights Reserved

  $Id: mprsa.c,v 1.1 2004/02/08 04:29:29 sting Exp $
 */

#include "mprsa.h"
#include <stdlib.h>
#include <string.h>

/* {{{ mp_i2osp(x, out, len) */

mp_err   mp_i2osp(mp_int *x, char *out, int len)
{
  int    xlen;

  ARGCHK(x != NULL && out != NULL && len > 0, MP_BADARG);

  if((xlen = mp_unsigned_bin_size(x)) > len) {
    return MP_RANGE;
  }

  xlen -= len;
  if(xlen > 0)
    memset(out, 0, xlen);

  mp_to_unsigned_bin(x, (unsigned char *)out + xlen);

  return MP_OKAY;
 
} /* end mp_i2osp() */

/* }}} */

/* {{{ mp_os2ip(x, in, len) */

mp_err   mp_os2ip(mp_int *x, char *in, int len)
{
  return mp_read_unsigned_bin(x, (unsigned char *)in, len);

} /* end mp_os2ip() */

/* }}} */

/* {{{ mp_rsaep(msg, e, modulus, cipher) */

mp_err   mp_rsaep(mp_int *msg, mp_int *e, mp_int *modulus, mp_int *cipher)
{
  ARGCHK(msg != NULL && e != NULL && 
	 modulus != NULL && cipher != NULL, MP_BADARG);

  /* Insure that message representative is in range of modulus */
  if((mp_cmp_z(msg) < 0) ||
     (mp_cmp(msg, modulus) >= 0)) {
    return MP_RANGE;
  }

  return mp_exptmod(msg, e, modulus, cipher);

} /* end mp_rsaep() */

/* }}} */

/* {{{ mp_rsadp(cipher, d, modulus, msg) */

mp_err   mp_rsadp(mp_int *cipher, mp_int *d, mp_int *modulus, mp_int *msg)
{
  ARGCHK(cipher != NULL && d != NULL && 
	 modulus != NULL && msg != NULL, MP_BADARG);

  /* Insure that ciphertext representative is in range of modulus */
  if((mp_cmp_z(cipher) < 0) || 
     (mp_cmp(cipher, modulus) >= 0)) {
    return MP_RANGE;
  }

  return mp_exptmod(cipher, d, modulus, msg);

} /* end mp_rsadp() */

/* }}} */

/* {{{ mp_rsasp(msg, d, modulus, sig) */

mp_err   mp_rsasp(mp_int *msg, mp_int *d, mp_int *modulus, mp_int *sig)
{
  ARGCHK(msg != NULL && d != NULL &&
	 modulus != NULL && sig != NULL, MP_BADARG);

  if((mp_cmp_z(msg) < 0) ||
     (mp_cmp(msg, modulus) >= 0)) {
    return MP_RANGE;
  }

  return mp_exptmod(msg, d, modulus, sig);

} /* end mp_rsasp() */

/* }}} */

/* {{{ mp_rsavp(sig, e, modulus, msg) */

mp_err   mp_rsavp(mp_int *sig, mp_int *e, mp_int *modulus, mp_int *msg)
{
  ARGCHK(sig != NULL && e != NULL &&
	 modulus != NULL && msg != NULL, MP_BADARG);

  if((mp_cmp_z(sig) < 0) || 
     (mp_cmp(sig, modulus) >= 0)) {
    return MP_RANGE;
  }

  return mp_exptmod(sig, e, modulus, msg);

} /* end mp_rsavp() */

/* }}} */

/* {{{ mp_pkcs1v15_encode(msg, mlen, emsg, emlen, rand) */

mp_err   mp_pkcs1v15_encode(char *msg, int mlen, 
			    char *emsg, int emlen, rnd_f rand)
{
  int    poffset;

  ARGCHK(msg != NULL && mlen >= 0 &&
	 emsg != NULL && emlen > 0 && rand != NULL, MP_BADARG);

  if(mlen > emlen - 10)
    return MP_RANGE;

  emsg[0] = 0x02;
  poffset = emlen - mlen;        /* offset of end of padding */
  (rand)(emsg + 1, poffset - 2); /* generate random pad      */
  emsg[poffset - 1] = 0x00;      /* zero separator           */

  memcpy(emsg + poffset, msg, mlen);

  return MP_OKAY;

} /* end mp_pkcs1v15_encode() */

/* }}} */

/* {{{ mp_pkcs1v15_decode(emsg, emlen, msg, mlen) */

mp_err   mp_pkcs1v15_decode(char *emsg, int emlen, char *msg, int *mlen)
{
  int    ix, jx, outlen;

  ARGCHK(emsg != NULL, MP_BADARG);

  /* If message is less than minimum length, it's an error */
  if(emlen < 10)
    return MP_RANGE;

  /* Check the format of the packet, error if it's invalid */
  if(emsg[0] != 0x02)
    return MP_UNDEF;

  /* Look for zero separator */
  for(ix = 9; ix < emlen; ix++)
    if(emsg[ix] == 0x00)
      break;

  if(ix == emlen)
    return MP_UNDEF;

  /* Make sure padding bytes are valid */
  for(jx = 1; jx < ix; jx++)
    if(emsg[jx] == 0)
      return MP_UNDEF;

  outlen = emlen - (ix + 1);
  if(msg != NULL && outlen > 0)
    memmove(msg, emsg + (ix + 1), outlen);

  if(mlen != NULL)
    *mlen = outlen;
  
  return MP_OKAY;

} /* end mp_pkcs1v15_decode() */

/* }}} */

/* {{{ mp_pkcs1v15_encrypt(msg, mlen, e, modulus, out, olen, rand) */

mp_err   mp_pkcs1v15_encrypt(char *msg, int mlen,
			     mp_int *e, mp_int *modulus,
			     char **out, int *olen,
			     rnd_f rand)
{
  int    k;
  char   *buf;
  mp_err res;
  mp_int mrep;

  ARGCHK(msg != NULL && mlen >= 0 && e != NULL &&
	 modulus != NULL && out != NULL && olen != NULL, MP_BADARG);

  k = mp_unsigned_bin_size(modulus); /* length of modulus, in bytes */
  if((buf = malloc(k)) == NULL)
    return MP_MEM;

  /* Encode according to PKCS #1 v1.5 */
  if((res = mp_pkcs1v15_encode(msg, mlen, buf, k, rand)) != MP_OKAY) 
    goto CLEANUP;

  /* Convert encoded message to a big number for encryption */
  if((res = mp_init(&mrep)) != MP_OKAY)
    goto CLEANUP;

  if((res = mp_os2ip(&mrep, buf, k)) != MP_OKAY) {
    mp_clear(&mrep);
    goto CLEANUP;
  }

  /* Now, encrypt... */
  if((res = mp_rsaep(&mrep, e, modulus, &mrep)) != MP_OKAY) {
    mp_clear(&mrep);
    goto CLEANUP;
  }

  /* Unpack message representative... */
  if((res = mp_i2osp(&mrep, buf, k)) != MP_OKAY) {
    mp_clear(&mrep);
    goto CLEANUP;
  }

  mp_clear(&mrep);
  *out = buf;
  *olen = k;

  return MP_OKAY;

 CLEANUP:
  memset(buf, 0, k);
  free(buf);
  return res;

} /* end mp_pkcs1v15_encrypt() */

/* }}} */

/* {{{ mp_pkcs1v15_decrypt(msg, mlen, d, modulus, out, olen) */

mp_err   mp_pkcs1v15_decrypt(char *msg, int mlen,
			     mp_int *d, mp_int *modulus,
			     char **out, int *olen)
{
  int     k;
  char   *buf;
  mp_err  res;
  mp_int  mrep;

  ARGCHK(msg != NULL && d != NULL && modulus != NULL &&
	 out != NULL && olen != NULL, MP_BADARG);

  k = mp_unsigned_bin_size(modulus);  /* size of modulus, in bytes */
  if(mlen != k)
    return MP_UNDEF;
  if((buf = malloc(k)) == NULL)
    return MP_MEM;

  /* Convert ciphertext to integer representative */
  if((res = mp_init(&mrep)) != MP_OKAY) {
    free(buf);
    return res;
  }

  if((res = mp_os2ip(&mrep, msg, mlen)) != MP_OKAY) 
    goto CLEANUP;

  /* Decrypt ... */
  if((res = mp_rsadp(&mrep, d, modulus, &mrep)) != MP_OKAY)
    goto CLEANUP;

  if((res = mp_i2osp(&mrep, buf, k)) != MP_OKAY)
    goto CLEANUP;

  if((res = mp_pkcs1v15_decode(buf, k, buf, olen)) == MP_OKAY) {
    *out = buf;
    return MP_OKAY;
  } 

 CLEANUP:
  memset(buf, 0, k - 1);
  free(buf);
  mp_clear(&mrep);
  return res;
    
} /* end mp_pkcs1v15_decrypt() */

/* }}} */

/* {{{ mp_pkcs1v15_maxlen(modulus) */

int      mp_pkcs1v15_maxlen(mp_int *modulus)
{
  int    modlen = mp_unsigned_bin_size(modulus);

  if(modlen < 10)
    return 0;
  else
    return modlen - 10;

} /* end mp_pkcs1v15_maxlen() */

/* }}} */

/*------------------------------------------------------------------------*/
/* HERE THERE BE DRAGONS                                                  */
