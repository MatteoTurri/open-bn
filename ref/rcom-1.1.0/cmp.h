/*
 * The multi-precision integers of BSAFE (CMP_*).
 */
#ifndef RCOM_CMP_H
#define RCOM_CMP_H

typedef struct CMPInt {
    /* 0x00 */ int allocated;  /* words allocated in value */
    /* 0x04 */ int length;  /* words in use */
    /* 0x08 */ unsigned long *value;  /* least significant word first */
} CMPInt; /* size = 0x0C */

/* cmp.c */
int CMP_Add(CMPInt *a, CMPInt *b, CMPInt *dst);
int CMP_AddCMPWord(unsigned long word, CMPInt *x);
int CMP_BitLengthOfCMPInt(CMPInt *x);
int CMP_ClearBit(int bitIndex, CMPInt *x);
int CMP_Compare(CMPInt *a, CMPInt *b);
int CMP_GetBit(int bitIndex, CMPInt *x, int *out);
int CMP_Move(CMPInt *src, CMPInt *dst);
int CMP_SetBit(int bitIndex, CMPInt *x);
int CMP_Subtract(CMPInt *a, CMPInt *b, CMPInt *dst);
int CMP_SubtractCMPWord(unsigned long word, CMPInt *x);
int CMP_SwapContents(CMPInt *a, CMPInt *b);

/* cmp_a2.c */
int CMP_CMPIntToOctetString(CMPInt *x, unsigned int cap, int *outLen, unsigned char *buf);
int CMP_CMPIntToFixedLenOctetStr(CMPInt *x, unsigned int fixedLen, unsigned int cap, int *outLen,
                                 unsigned char *buf);
int CMP_CMPWordToCMPInt(unsigned long word, CMPInt *x);
int CMP_OctetStringToCMPInt(const unsigned char *buf, unsigned int len, CMPInt *x);
int CMP_PowerOfTwo(int bits, CMPInt *out);

/* cmp_addinplace.c */
int CMP_AddInPlace(CMPInt *a, CMPInt *b);

/* cmp_shl.c */
int CMP_GetOffsetOfMSB(CMPInt *x, int *out);
int CMP_IsCMPEven(CMPInt *x);
int CMP_IsCMPOdd(CMPInt *x);
int CMP_RecomputeLength(int hi, CMPInt *x);
int CMP_ShiftLeftByBits(int bits, CMPInt *x);
int CMP_ShiftLeftByCMPWords(int n, CMPInt *x);
int CMP_ShiftRight1Bit(CMPInt *x);
int CMP_ShiftRight1BitNotInPlace(CMPInt *src, CMPInt *dst);
int CMP_ShiftRightByBits(int bits, CMPInt *x);
int CMP_ShiftRightByCMPWords(int n, CMPInt *x);
int CMP_SubtractInPlace(CMPInt *a, CMPInt *b);
int CMP_SubtractSigned(CMPInt *a, int signA, CMPInt *b, int signB, CMPInt *dst, int *sign_out);

/* cmp_realloc.c */
void CMP_Constructor(CMPInt *x);
void CMP_Destructor(CMPInt *x);
int CMP_realloc(int newAllocated, CMPInt *x);
int CMP_reallocNoCopy(int newAllocated, CMPInt *x);

/* cmp_vecmul.c */
void CMP_AddInTrace(CMPInt *a, CMPInt *dst);
void CMP_VectorMultiply(unsigned long word, CMPInt *src, int srcStart, int count, CMPInt *dst, int dstStart);

/* cmp_div.c */
int CMP_AbsDifference(CMPInt *a, CMPInt *b, int *sign, CMPInt *dst);
int CMP_AppendWord(unsigned long word, CMPInt *x);
int CMP_ComputeExtendedGCD(CMPInt *a, CMPInt *b, CMPInt *u, CMPInt *v, CMPInt *gcd);
int CMP_ComputeGCD(CMPInt *a, CMPInt *b, CMPInt *out);
void CMP_DivOneAndHalfWordsByWord(unsigned long divisor, unsigned long hi, unsigned long lo,
                                  unsigned long *quot, unsigned long *rem);
int CMP_Divide(CMPInt *x, CMPInt *modulus, CMPInt *quot, CMPInt *rem);
void CMP_DivideTwoWordsByWord(unsigned long divisor, unsigned long hi, unsigned long lo,
                              unsigned long *quot, unsigned long *rem);
int CMP_EstimateMSWQuotient(CMPInt *rem, CMPInt *modulus, CMPInt *q, CMPInt *prod);
int CMP_FullCMPWordModReduce(CMPInt *x, unsigned long word, unsigned long *out);
int CMP_ModularReduce(CMPInt *x, CMPInt *modulus, CMPInt *out);
int CMP_Multiply(CMPInt *a, CMPInt *b, CMPInt *dst);
int CMP_UpdateCoefficients(CMPInt *A, CMPInt *B, CMPInt *q, int *signA, int *signB, CMPInt *C, CMPInt *T);

/* cmp_modular.c */
int CMP_ModInvert(CMPInt *a, CMPInt *m, CMPInt *out);
int CMP_ModMultiply(CMPInt *a, CMPInt *b, CMPInt *m, CMPInt *dst);

/* cmp_mont.c */
int CMP_ComputeMontCoeff(CMPInt *n, int *out);
int CMP_ConvertFromMont(CMPInt *mont_in, CMPInt *n, unsigned long n0, CMPInt *out);
int CMP_ConvertToMont(CMPInt *a, CMPInt *n, CMPInt *out);
int CMP_MontProduct(CMPInt *a, CMPInt *b, CMPInt *n, unsigned long n0, CMPInt *out);
int CMP_MontSquare(CMPInt *a, CMPInt *n, unsigned long n0, CMPInt *out);

/* cmp_modexp.c */
int CMP_ComputeWindowTable(CMPInt *e, int *wbits, unsigned long **table, int *tableLen, int *maxWindow);
int CMP_GetBitsAtIndex(CMPInt *x, int bitIndex, int maxBits, int *outUsed, int *outValue);
int CMP_GetWindowSize(int bits);
int CMP_ModCube(CMPInt *base, CMPInt *mod, CMPInt *out, void *ctx);
int CMP_ModExp(CMPInt *base, CMPInt *e, CMPInt *mod, CMPInt *out, void *ctx);
int CMP_ModExpCRT(CMPInt *out, CMPInt *p, CMPInt *q, CMPInt *dp, CMPInt *dq, CMPInt *coeff,
                  CMPInt *base, void *ctx);
int CMP_ModExpMultiPrimeCRT(CMPInt *base, int nprimes, CMPInt *primes, CMPInt *exps, CMPInt *coeff,
                            CMPInt *out, void *ctx);
int CMP_ModExpPrecomp1(CMPInt *base, unsigned long *table, int tableLen, int wbits, int maxWindow,
                       CMPInt *mod, unsigned long n0, CMPInt *out, void *ctx);
int CMP_ModExpPrecomp2(CMPInt *tbl, int count, unsigned long *table, int tableLen,
                       unsigned long n0, CMPInt *t, CMPInt *mod, CMPInt *out, void *ctx);
int CMP_ModExpSpecialExponent(CMPInt *base, CMPInt *e, CMPInt *mod, CMPInt *out, int ebits, void *ctx);

#endif
