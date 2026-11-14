/* rcom-1.1.0/rc6.h -- RC6 (Rivest/Robshaw/Sidney/Yin 1998), RSA RCOM fork.
 * Reconstructed from the asm.
 *
 * The context: the round count (written by the Init functions), the encrypt and
 * decrypt methods the AM_RC6_* Init methods store, and S[2r+4] (r = 20: 44 words).
 */

#ifndef HEADER_RC6_H
#define HEADER_RC6_H

#include "aglobal.h"

typedef struct rc6_key_st
	{
	/* 0x00 */ unsigned int rounds;
	/* 0x04 */ int (*encrypt)();  /* set by the AM_RC6_* Init methods */
	/* 0x08 */ int (*decrypt)();
	/* 0x0C */ unsigned int S[44];
	} RC6_KEY;

int A_RC6Init_20(RC6_KEY *key, ITEM *item, int rounds);
int A_RC6Init_20_128(RC6_KEY *key, ITEM *item, int rounds);
int A_RC6Init_20_192(RC6_KEY *key, ITEM *item, int rounds);
int A_RC6Init_20_256(RC6_KEY *key, ITEM *item, int rounds);
int A_RC6Encrypt_20(RC6_KEY *key, unsigned int *out, unsigned int *in);
int A_RC6Decrypt_20(RC6_KEY *key, unsigned int *out, unsigned int *in);
int A_RC6_NOT_SUPPORTED();

/* The initial S table (P32 + i * Q32), rc6_pqtable.c. */
extern const unsigned int RonsTableOfPQ[];

#define RC6_ROTL(x,n)	(((x) << (n)) | ((x) >> (32 - (n))))
#define RC6_ROTR(x,n)	(((x) >> (n)) | ((x) << (32 - (n))))

#endif
