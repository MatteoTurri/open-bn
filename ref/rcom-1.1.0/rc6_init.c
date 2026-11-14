/*
 * The RC6 key schedule (A_RC6Init_20).
 */

#include "rc6.h"
#include "bsafe.h"

/* The tables of the next index. */
static const unsigned char rc6NextS[44] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 0,
	};

/* 64 entries: c <= 64 for a key of at most 255 bytes. */
static const unsigned char rc6NextL[64] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 0,
	};

#define RC6_KEY_STEP \
{ unsigned int m, x, y, ab; \
	ab = A + B; \
	x = S[i] + ab; \
	A = S[i] = RC6_ROTL(x, 3); \
	i = rc6NextS[i]; \
	rot = A + B; \
	y = L[j] + rot; \
	m = w - rot; \
	B = L[j] = (y >> m) | (y << rot); \
	j = idx[j]; \
}


int A_RC6Init_20(key, item, rounds)
RC6_KEY *key;
ITEM *item;
int rounds;
	{
	unsigned int L[64];
	unsigned char idx[256];
	unsigned int *S;
	unsigned int A,B,c,len,w;
	unsigned int rot;
	int v,t,i,j;
	unsigned char *p;
	w = 32;
	len = item->len;
	if (len - 1 >= 255)
		return(12);

	c = (len + 3) / 4;
	p = item->data;
	S = key->S;
	key->rounds = rounds;
	L[c - 1] = 0;
	T_memcpy(L, p, len);

	T_memcpy(idx, rc6NextL, c);
	/* t = words of S (2r + 4), v = count of the loop, max(c, t) */
	t = 2 * rounds + 4;
	idx[c - 1] = 0;
	T_memcpy(S, RonsTableOfPQ, t * 4);

	v = t;
	if (v < c)
		v = c;
	A = B = i = j = 0;
	for (; v > 0; v--)
		{
		RC6_KEY_STEP
		RC6_KEY_STEP
		RC6_KEY_STEP
		}
	T_memset(L, 0, 256);
	return(0);
	}

#include "rc6.h"

#define RC6_FIXED_KEY_STEP(k, jj) \
{ unsigned int m, x, y, ab; \
	ab = A + B; \
	x = S[k] + ab; \
	A = S[k] = RC6_ROTL(x, 3); \
	rot = A + B; \
	y = L[jj] + rot; \
	m = w - rot; \
	B = L[jj] = (y >> m) | (y << rot); \
}

int A_RC6Init_20_128(key, item, rounds)
RC6_KEY *key;
ITEM *item;
int rounds;
	{
	unsigned int L[4];
	unsigned int *S;
	unsigned int A,B,w;
	unsigned int rot;	/* rotation count of L, A + B */
	int v;
	unsigned char *p;

	w = 32;
	p = item->data;
	S = key->S;
	key->rounds = rounds;
	T_memcpy(L, p, 16);
	T_memcpy(S, RonsTableOfPQ, rounds * 8 + 16);
	B = 0;
	A = 0;
	v = 2;
	do
		{
		v = v - 1;
		RC6_FIXED_KEY_STEP(0, 0)
		RC6_FIXED_KEY_STEP(1, 1)
		RC6_FIXED_KEY_STEP(2, 2)
		RC6_FIXED_KEY_STEP(3, 3)
		RC6_FIXED_KEY_STEP(4, 0)
		RC6_FIXED_KEY_STEP(5, 1)
		RC6_FIXED_KEY_STEP(6, 2)
		RC6_FIXED_KEY_STEP(7, 3)
		RC6_FIXED_KEY_STEP(8, 0)
		RC6_FIXED_KEY_STEP(9, 1)
		RC6_FIXED_KEY_STEP(10, 2)
		RC6_FIXED_KEY_STEP(11, 3)
		RC6_FIXED_KEY_STEP(12, 0)
		RC6_FIXED_KEY_STEP(13, 1)
		RC6_FIXED_KEY_STEP(14, 2)
		RC6_FIXED_KEY_STEP(15, 3)
		RC6_FIXED_KEY_STEP(16, 0)
		RC6_FIXED_KEY_STEP(17, 1)
		RC6_FIXED_KEY_STEP(18, 2)
		RC6_FIXED_KEY_STEP(19, 3)
		RC6_FIXED_KEY_STEP(20, 0)
		RC6_FIXED_KEY_STEP(21, 1)
		RC6_FIXED_KEY_STEP(22, 2)
		RC6_FIXED_KEY_STEP(23, 3)
		RC6_FIXED_KEY_STEP(24, 0)
		RC6_FIXED_KEY_STEP(25, 1)
		RC6_FIXED_KEY_STEP(26, 2)
		RC6_FIXED_KEY_STEP(27, 3)
		RC6_FIXED_KEY_STEP(28, 0)
		RC6_FIXED_KEY_STEP(29, 1)
		RC6_FIXED_KEY_STEP(30, 2)
		RC6_FIXED_KEY_STEP(31, 3)
		RC6_FIXED_KEY_STEP(32, 0)
		RC6_FIXED_KEY_STEP(33, 1)
		RC6_FIXED_KEY_STEP(34, 2)
		RC6_FIXED_KEY_STEP(35, 3)
		RC6_FIXED_KEY_STEP(36, 0)
		RC6_FIXED_KEY_STEP(37, 1)
		RC6_FIXED_KEY_STEP(38, 2)
		RC6_FIXED_KEY_STEP(39, 3)
		RC6_FIXED_KEY_STEP(40, 0)
		RC6_FIXED_KEY_STEP(41, 1)
		RC6_FIXED_KEY_STEP(42, 2)
		RC6_FIXED_KEY_STEP(43, 3)
		}
	while (v >= 0);
	T_memset(L, 0, 16);
	return(0);
	}

int A_RC6Init_20_192(key, item, rounds)
RC6_KEY *key;
ITEM *item;
int rounds;
	{
	unsigned int L[6];
	unsigned int *S;
	unsigned int A,B,w;
	unsigned int rot;	/* rotation count of L, A + B */
	int v;
	unsigned char *p;

	w = 32;
	p = item->data;
	S = key->S;
	key->rounds = rounds;
	T_memcpy(L, p, 24);
	T_memcpy(S, RonsTableOfPQ, rounds * 8 + 16);
	B = 0;
	A = 0;
	v = 2;
	do
		{
		v = v - 1;
		RC6_FIXED_KEY_STEP(0, 0)
		RC6_FIXED_KEY_STEP(1, 1)
		RC6_FIXED_KEY_STEP(2, 2)
		RC6_FIXED_KEY_STEP(3, 3)
		RC6_FIXED_KEY_STEP(4, 4)
		RC6_FIXED_KEY_STEP(5, 5)
		RC6_FIXED_KEY_STEP(6, 0)
		RC6_FIXED_KEY_STEP(7, 1)
		RC6_FIXED_KEY_STEP(8, 2)
		RC6_FIXED_KEY_STEP(9, 3)
		RC6_FIXED_KEY_STEP(10, 4)
		RC6_FIXED_KEY_STEP(11, 5)
		RC6_FIXED_KEY_STEP(12, 0)
		RC6_FIXED_KEY_STEP(13, 1)
		RC6_FIXED_KEY_STEP(14, 2)
		RC6_FIXED_KEY_STEP(15, 3)
		RC6_FIXED_KEY_STEP(16, 4)
		RC6_FIXED_KEY_STEP(17, 5)
		RC6_FIXED_KEY_STEP(18, 0)
		RC6_FIXED_KEY_STEP(19, 1)
		RC6_FIXED_KEY_STEP(20, 2)
		RC6_FIXED_KEY_STEP(21, 3)
		RC6_FIXED_KEY_STEP(22, 4)
		RC6_FIXED_KEY_STEP(23, 5)
		RC6_FIXED_KEY_STEP(24, 0)
		RC6_FIXED_KEY_STEP(25, 1)
		RC6_FIXED_KEY_STEP(26, 2)
		RC6_FIXED_KEY_STEP(27, 3)
		RC6_FIXED_KEY_STEP(28, 4)
		RC6_FIXED_KEY_STEP(29, 5)
		RC6_FIXED_KEY_STEP(30, 0)
		RC6_FIXED_KEY_STEP(31, 1)
		RC6_FIXED_KEY_STEP(32, 2)
		RC6_FIXED_KEY_STEP(33, 3)
		RC6_FIXED_KEY_STEP(34, 4)
		RC6_FIXED_KEY_STEP(35, 5)
		RC6_FIXED_KEY_STEP(36, 0)
		RC6_FIXED_KEY_STEP(37, 1)
		RC6_FIXED_KEY_STEP(38, 2)
		RC6_FIXED_KEY_STEP(39, 3)
		RC6_FIXED_KEY_STEP(40, 4)
		RC6_FIXED_KEY_STEP(41, 5)
		RC6_FIXED_KEY_STEP(42, 0)
		RC6_FIXED_KEY_STEP(43, 1)
		{ unsigned int t0, t1;
		t0 = L[0];
		t1 = L[1];
		L[0] = L[2];
		L[1] = L[3];
		L[2] = L[4];
		L[3] = L[5];
		L[4] = t0;
		L[5] = t1;
		}
		}
	while (v >= 0);
	T_memset(L, 0, 24);
	return(0);
	}

int A_RC6Init_20_256(key, item, rounds)
RC6_KEY *key;
ITEM *item;
int rounds;
	{
	unsigned int L[8];
	unsigned int *S;
	unsigned int A,B,w;
	unsigned int rot;	/* rotation count of L, A + B */
	int v;
	unsigned char *p;

	w = 32;
	p = item->data;
	S = key->S;
	key->rounds = rounds;
	T_memcpy(L, p, 32);
	T_memcpy(S, RonsTableOfPQ, rounds * 8 + 16);
	B = 0;
	A = 0;
	v = 2;
	do
		{
		v = v - 1;
		RC6_FIXED_KEY_STEP(0, 0)
		RC6_FIXED_KEY_STEP(1, 1)
		RC6_FIXED_KEY_STEP(2, 2)
		RC6_FIXED_KEY_STEP(3, 3)
		RC6_FIXED_KEY_STEP(4, 4)
		RC6_FIXED_KEY_STEP(5, 5)
		RC6_FIXED_KEY_STEP(6, 6)
		RC6_FIXED_KEY_STEP(7, 7)
		RC6_FIXED_KEY_STEP(8, 0)
		RC6_FIXED_KEY_STEP(9, 1)
		RC6_FIXED_KEY_STEP(10, 2)
		RC6_FIXED_KEY_STEP(11, 3)
		RC6_FIXED_KEY_STEP(12, 4)
		RC6_FIXED_KEY_STEP(13, 5)
		RC6_FIXED_KEY_STEP(14, 6)
		RC6_FIXED_KEY_STEP(15, 7)
		RC6_FIXED_KEY_STEP(16, 0)
		RC6_FIXED_KEY_STEP(17, 1)
		RC6_FIXED_KEY_STEP(18, 2)
		RC6_FIXED_KEY_STEP(19, 3)
		RC6_FIXED_KEY_STEP(20, 4)
		RC6_FIXED_KEY_STEP(21, 5)
		RC6_FIXED_KEY_STEP(22, 6)
		RC6_FIXED_KEY_STEP(23, 7)
		RC6_FIXED_KEY_STEP(24, 0)
		RC6_FIXED_KEY_STEP(25, 1)
		RC6_FIXED_KEY_STEP(26, 2)
		RC6_FIXED_KEY_STEP(27, 3)
		RC6_FIXED_KEY_STEP(28, 4)
		RC6_FIXED_KEY_STEP(29, 5)
		RC6_FIXED_KEY_STEP(30, 6)
		RC6_FIXED_KEY_STEP(31, 7)
		RC6_FIXED_KEY_STEP(32, 0)
		RC6_FIXED_KEY_STEP(33, 1)
		RC6_FIXED_KEY_STEP(34, 2)
		RC6_FIXED_KEY_STEP(35, 3)
		RC6_FIXED_KEY_STEP(36, 4)
		RC6_FIXED_KEY_STEP(37, 5)
		RC6_FIXED_KEY_STEP(38, 6)
		RC6_FIXED_KEY_STEP(39, 7)
		RC6_FIXED_KEY_STEP(40, 0)
		RC6_FIXED_KEY_STEP(41, 1)
		RC6_FIXED_KEY_STEP(42, 2)
		RC6_FIXED_KEY_STEP(43, 3)
		{ unsigned int t0, t1, t2, t3;
		t0 = L[0];
		t1 = L[1];
		t2 = L[2];
		t3 = L[3];
		L[0] = L[4];
		L[1] = L[5];
		L[2] = L[6];
		L[3] = L[7];
		L[4] = t0;
		L[5] = t1;
		L[6] = t2;
		L[7] = t3;
		}
		}
	while (v >= 0);
	T_memset(L, 0, 32);
	return(0);
	}
