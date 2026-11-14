/*
 * The DES core of RCOM: key schedules and the single-block cipher.
 */

#ifndef RCOM_DESCORE_H
#define RCOM_DESCORE_H

typedef unsigned int DES_LONG;

void DESDecryptInit(DES_LONG *schedule, DES_LONG *key);
void DESEncryptInit(DES_LONG *schedule, DES_LONG *key);
void DES(DES_LONG *schedule, DES_LONG *out, DES_LONG *in);

#endif
