/*
 * A_RC6Encrypt_20: RC6-32/20 encryption of one 128-bit block (Rivest, Robshaw, Sidney, Yin 1998).
 * The 20 rounds are unrolled and the names A, B, C, D rotate through the macro instead of being
 * reassigned; 20 is a multiple of 4, so they end where they started.
 */

#include "rc6.h"

#define RC6_ROUND(A,B,C,D,i) \
do { unsigned int m1, m2; \
	t = ((B) + (B)) * (B) + (B); \
	u = ((D) + (D)) * (D) + (D); \
	t = RC6_ROTL(t, 5); \
	u = RC6_ROTL(u, 5); \
	(A) = (A) ^ t; \
	(C) = (C) ^ u; \
	m1 = 32 - u; \
	m2 = 32 - t; \
	(A) = ((A) >> m1 | (A) << u) + S[i]; \
	(C) = ((C) >> m2 | (C) << t) + S[(i) + 1]; \
} while (0)

int A_RC6Encrypt_20(key, out, in)
RC6_KEY *key;
unsigned int *out;
unsigned int *in;
	{
	unsigned int *S;
	unsigned int B,A,C,D,t,u;

	S = key->S;
	A = in[0];
	B = in[1];
	C = in[2];
	D = in[3];
	B = B + S[0];
	D = D + S[1];

	{ unsigned int m1, m2;
	t = (B + B) * B + B;
	u = (D + D) * D + D;
	t = RC6_ROTL(t, 5);
	u = RC6_ROTL(u, 5);
	A = A ^ t;
	m1 = 32 - u;
	A = (A >> m1 | A << u) + S[2];
	C = C ^ u;
	m2 = 32 - t;
	C = (C >> m2 | C << t) + S[3];
	}
	RC6_ROUND(B,C,D,A,4);
	RC6_ROUND(C,D,A,B,6);
	RC6_ROUND(D,A,B,C,8);
	RC6_ROUND(A,B,C,D,10);
	RC6_ROUND(B,C,D,A,12);
	RC6_ROUND(C,D,A,B,14);
	RC6_ROUND(D,A,B,C,16);
	RC6_ROUND(A,B,C,D,18);
	RC6_ROUND(B,C,D,A,20);
	RC6_ROUND(C,D,A,B,22);
	RC6_ROUND(D,A,B,C,24);
	RC6_ROUND(A,B,C,D,26);
	RC6_ROUND(B,C,D,A,28);
	RC6_ROUND(C,D,A,B,30);
	RC6_ROUND(D,A,B,C,32);
	RC6_ROUND(A,B,C,D,34);
	RC6_ROUND(B,C,D,A,36);
	RC6_ROUND(C,D,A,B,38);
	{ unsigned int m1, m2;
	t = (A + A) * A + A;
	u = (C + C) * C + C;
	t = RC6_ROTL(t, 5);
	u = RC6_ROTL(u, 5);
	D = D ^ t;
	B = B ^ u;
	m1 = 32 - u;
	m2 = 32 - t;
	A = A + S[42];
	D = (D >> m1 | D << u) + S[40];
	C = C + S[43];
	B = (B >> m2 | B << t) + S[41];
	}
	out[0] = A;
	out[1] = B;
	out[2] = C;
	out[3] = D;
	return(0);
	}
