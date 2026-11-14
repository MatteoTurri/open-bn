/*
 * BSAFE records and algorithm methods used by the DNAS2 cipher wrappers.
 */

#ifndef DNAS2_BSAFE_H
#define DNAS2_BSAFE_H

#include "../../ref/rcom-1.1.0/aglobal.h"
#include "../../ref/rcom-1.1.0/bsafe_amtype.h"

/* The algorithm methods the DNAS2 choosers list (defined in RCOM). */
extern B_AlgorithmMethod AM_DES_ENCRYPT, AM_DES_DECRYPT, AM_ECB_ENCRYPT, AM_ECB_DECRYPT;
extern B_AlgorithmMethod AM_CBC_ENCRYPT, AM_CBC_DECRYPT;
extern B_AlgorithmMethod AM_DES_EDE_ENCRYPT, AM_DES_EDE_DECRYPT;
extern B_AlgorithmMethod AM_RC6_ENCRYPT, AM_RC6_DECRYPT;
extern B_AlgorithmMethod AM_RSA_DECRYPT;

/* A method name ("des", "cbc", "nopad"...), stored XORed with a key. */
typedef struct {
	/* 0x00 */ unsigned char b[8];
} Bsafe8; /* size = 0x08 */

#endif
