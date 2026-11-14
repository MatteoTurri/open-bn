/*
 * CMP_* modular add, subtract, multiply and inverse.
 */

#include "cmp.h"


/* dst = (a + b) mod m for reduced a and b: one conditional subtraction, CMP_ModularReduce only as
 * the fallback. */
int CMP_ModAdd(CMPInt *a, CMPInt *b, CMPInt *m, CMPInt *dst)
{
    CMPInt t;
    int rc;

    CMP_Constructor(&t);
    do {
        if ((rc = CMP_Add(a, b, dst)) != 0)
            break;
        if (CMP_Compare(dst, m) < 0)
            break;
        if ((rc = CMP_SubtractInPlace(m, dst)) != 0)
            break;
        if (CMP_Compare(dst, m) < 0)
            break;
        if ((rc = CMP_ModularReduce(dst, m, &t)) != 0)
            break;
        rc = CMP_Move(&t, dst);
    } while (0);
    CMP_Destructor(&t);
    return rc;
}

/* dst = (a - b) mod m: with a >= b the difference is reduced; otherwise m - (b - a), reducing b - a
 * first when it exceeds m (a zero remainder gives 0, not m). */
int CMP_ModSubtract(CMPInt *a, CMPInt *b, CMPInt *m, CMPInt *dst)
{
    CMPInt t;
    int rc;

    CMP_Constructor(&t);
    do {
        if (CMP_Compare(a, b) >= 0) {
            if ((rc = CMP_Subtract(a, b, dst)) != 0)
                break;
            if (CMP_Compare(dst, m) < 0)
                break;
            if ((rc = CMP_ModularReduce(dst, m, &t)) != 0)
                break;
            rc = CMP_Move(&t, dst);
            break;
        }
        if ((rc = CMP_Subtract(b, a, &t)) != 0)
            break;
        if (CMP_Compare(&t, m) <= 0) {
            rc = CMP_Subtract(m, &t, dst);
            break;
        }
        if ((rc = CMP_Move(&t, dst)) != 0)
            break;
        if ((rc = CMP_ModularReduce(dst, m, &t)) != 0)
            break;
        if (t.length == 1 && t.value[0] == 0) {
            dst->length = 1;
            dst->value[0] = 0;
            break;
        }
        rc = CMP_Subtract(m, &t, dst);
    } while (0);
    CMP_Destructor(&t);
    return rc;
}

/* dst = (a * b) mod m, not in Montgomery form. */
int CMP_ModMultiply(CMPInt *a, CMPInt *b, CMPInt *m, CMPInt *dst)
{
    CMPInt temp;
    int rc;

    CMP_Constructor(&temp);
    do {
        rc = CMP_Multiply(a, b, &temp);
        if (rc != 0)
            break;
        rc = CMP_ModularReduce(&temp, m, dst);
    } while (0);
    CMP_Destructor(&temp);
    return rc;
}

/* out = a^-1 mod m with the extended gcd. 0x108 when a >= m, 0x107 when the modulus is not
 * normalized (zero top word), 0x10B when a and m are not coprime. */
int CMP_ModInvert(CMPInt *a, CMPInt *m, CMPInt *out)
{
    CMPInt g;
    CMPInt u;
    int rc;

    CMP_Constructor(&g);
    CMP_Constructor(&u);
    do {
        if (CMP_Compare(a, m) >= 0) {
            rc = 0x108;
            if (m->value[m->length - 1] == 0)
                rc = 0x107;
            break;
        }
        if ((rc = CMP_ComputeExtendedGCD(a, m, out, &g, &u)) != 0)
            break;
        if (u.length != 1 || u.value[0] != 1)
            rc = 0x10b;
    } while (0);
    CMP_Destructor(&g);
    CMP_Destructor(&u);
    return rc;
}
