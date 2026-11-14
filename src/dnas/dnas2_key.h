/*
 * RSA key records of the DNAS2 content verifiers.
 */

#ifndef DNAS2_KEY_H
#define DNAS2_KEY_H

/* A BER-encoded RSA public key as stored in the program. */
struct DnasPubKey {
	/* 0x00 */ unsigned char b[162];
}; /* size = 0xA2 */

/* Key given to the RSA decryption wrappers: length, then the BER key. */
struct DnasKey {
	/* 0x000 */ unsigned int len;
	/* 0x004 */ unsigned char data[252];
}; /* size = 0x100 */

#endif
