/*
 * A_RC6Decrypt_20: RC6-32/20 decryption of one 128-bit block, the 20 rounds unrolled with the names
 * A, B, C, D rotating the other way.
 */

#include "rc6.h"

/* A variable rotation to the right, through the named complements to 32. */

#define RC6_RROUND(A,B,C,D,i) \
{ unsigned int m1, m2; \
	t = ((B) + (B)) * (B) + (B); \
	u = ((D) + (D)) * (D) + (D); \
	t = RC6_ROTL(t, 5); \
	u = RC6_ROTL(u, 5); \
	(A) = (A) - S[i]; \
	(C) = (C) - S[(i) + 1]; \
	m1 = 32 - u; \
	m2 = 32 - t; \
	(A) = ((A) << m1 | (A) >> u) ^ t; \
	(C) = ((C) << m2 | (C) >> t) ^ u; \
}

int A_RC6Decrypt_20(key, out, in)
RC6_KEY *key;
unsigned int *out;
unsigned int *in;
	{
	unsigned int *S;
	unsigned int A,B,C,D,t,u;

	S = key->S;
	A = in[0];
	B = in[1];
	C = in[2];
	D = in[3];
	A = A - S[42];
	C = C - S[43];

	RC6_RROUND(D,A,B,C,40)
	RC6_RROUND(C,D,A,B,38)
	RC6_RROUND(B,C,D,A,36)
	RC6_RROUND(A,B,C,D,34)
	RC6_RROUND(D,A,B,C,32)
	RC6_RROUND(C,D,A,B,30)
	RC6_RROUND(B,C,D,A,28)
	RC6_RROUND(A,B,C,D,26)
	RC6_RROUND(D,A,B,C,24)
	RC6_RROUND(C,D,A,B,22)
	RC6_RROUND(B,C,D,A,20)
	RC6_RROUND(A,B,C,D,18)
	RC6_RROUND(D,A,B,C,16)
	RC6_RROUND(C,D,A,B,14)
	RC6_RROUND(B,C,D,A,12)
	RC6_RROUND(A,B,C,D,10)
	RC6_RROUND(D,A,B,C,8)
	RC6_RROUND(C,D,A,B,6)
	RC6_RROUND(B,C,D,A,4)
	RC6_RROUND(A,B,C,D,2)

	B = B - S[0];
	D = D - S[1];
	out[0] = A;
	out[1] = B;
	out[2] = C;
	out[3] = D;
	return(0);
	}
