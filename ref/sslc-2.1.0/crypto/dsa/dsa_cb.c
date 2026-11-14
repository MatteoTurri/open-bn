/*
 * The default DSA methods of SSL-C (dsa_size_cb, dsa_sign_setup_cb, dsa_sign_cb, dsa_verify_cb), the
 * functions of the table that DSA_size, DSA_sign_setup, DSA_sign and DSA_verify call.
 */

#include <stdio.h>
#include "asn1.h"
#include "bn.h"
#include "dsa.h"

int dsa_size_cb(DSA *dsa)
{
    int i;
    ASN1_INTEGER bs;
    unsigned char buf[4];

    i = BN_num_bits(dsa->q);
    bs.length = (i + 7) / 8;
    bs.data = buf;
    bs.type = V_ASN1_INTEGER;
    buf[0] = 0xff;

    i = i2d_ASN1_INTEGER(&bs, 0);
    i += i;
    i = ASN1_object_size(1, i, V_ASN1_SEQUENCE);
    return i;
}
