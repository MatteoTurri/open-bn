/*
 * RSA_verify of the SSL-C fork.
 */

#include <string.h>
#include <stdio.h>
#include "cryptlib.h"
#include "r_eitems.h"
#include "pk_int.h"
#include "rsa.h"
#include "rsa_int.h"

int RSA_verify(int type, unsigned char *m, unsigned int m_len,
               unsigned char *sigbuf, unsigned int siglen, RSA *rsa)
{
    int result = 0;
    int itemLen2;
    unsigned char *buf = 0;
    void *ctx = 0;
    R_EITEM *item = 0;
    int itemLen = 0;

    ctx = rsa_get_pk_ctx(rsa);
    if (ctx == 0)
        goto end;

    if (R_EITEMS_find_item(rsa->eitems, 16, 16, &itemLen, &item, 0) != 0)
        goto end;

    if (item == 0)
        goto end;

    itemLen2 = item->length;
    if ((buf = R_malloc(itemLen2)) == 0)
        goto end;

    if (PK_pub_dec(ctx, buf, &itemLen2, (void *)itemLen2, sigbuf, (void *)siglen, 0) != 0)
        goto end;

    if ((m_len != 0) && ((unsigned int)itemLen2 > m_len) &&
        (memcmp(m, (char *)buf + (itemLen2 - m_len), m_len) == 0))
        result = 1;
    else
        result = 0;

end:
    if (buf != 0)
        R_free(buf);
    return result;
}

