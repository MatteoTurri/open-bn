/*
 * The BSAFE byte string (ITEM): keys, IVs and other parameters, and the entries the algorithm info
 * types copy them into.
 */
#ifndef AGLOBAL_H
#define AGLOBAL_H

typedef struct {
    /* 0x00 */ unsigned char *data;
    /* 0x04 */ unsigned int len;
} ITEM; /* size = 0x08 */

#endif
