/*
 * rsa_padding_check_pkcs1_type_1.
 */

/* Checks a block of type 1 (00 01 FF..FF 00 message); the length of the message goes to *outlen.
 * Returns 0, or 2 when the padding is not valid. */
int rsa_padding_check_pkcs1_type_1(unsigned char *to, int *outlen, unsigned char *from, int flen, int num)
{
    unsigned char *p;
    unsigned char *end;
    int j;

    if (flen >= num) {
        goto err;
    }
    if (flen < 10) {
        goto err;
    }

    p = from;
    end = from + flen;

    if (*p != 1) {
        goto err;
    }
    p++;

    for (j = 0; j < 8; j++) {
        if (*p++ != 0xff) {
            goto err;
        }
    }

    while (p != end) {
        if (*p != 0xff) {
            break;
        }
        p++;
    }

    if (p == end) {
        goto err;
    }
    if (*p++ != 0) {
        goto err;
    }

    *outlen = end - p;

    while (p != end) {
        *to++ = *p++;
    }

    return 0;

err:
    return 2;
}
