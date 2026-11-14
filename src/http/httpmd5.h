/*
 * MD5 message digest (RFC 1321) for the HTTP Digest authentication (httpmd5.c).
 */

#ifndef HTTPMD5_H
#define HTTPMD5_H

typedef struct {
	unsigned int state[4];   /* A, B, C, D */
	unsigned int count[2];   /* number of bits, modulo 2^64, low word first */
	unsigned char buffer[64];
} MD5_CTX;

void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context, const unsigned char *input, unsigned int inputLen);
void MD5Final(unsigned char digest[16], MD5_CTX *context);

#endif
