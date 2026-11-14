/*
 * pk_dh_sslc_compute_key, pk_dh_sslc_final and pk_dh_sslc_get.
 */
#include "pk_dh_impl.h"

int pk_dh_sslc_compute_key(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr, unsigned int bufCap, unsigned char *peerBytes, int peerLen)
{
    int ret;
    DhImpl *dh;
    int pLen;
    int pubLen;
    int padLen;
    BN_CTX *ctx;
    BIGNUM peerPub;
    BIGNUM secret;

    ret = 1;
    ctx = 0;

    if (op != 6)
        return 2;

    if ((ctx = BN_CTX_new()) == 0)
        return 1;

    BN_init(&peerPub);
    BN_init(&secret);

    dh = ctxArg->impl;

    if ((dh->flags & 0x5) != 5)
        return 3;

    pLen = (BN_num_bits(&dh->p) + 7) / 8;
    if (bufCap < pLen)
        return 4;

    BN_bin2bn(peerBytes, peerLen, &peerPub);

    if (peerPub.top == 0 || (peerPub.top == 1 && *(unsigned int *)peerPub.d == 0) ||
        (peerPub.top == 1 && *(unsigned int *)peerPub.d == 1))
        return 5;

    ret = BN_ME_CTX_mod_exp(dh->modExpCtx, &secret, &peerPub, &dh->privKey, 0, ctx);
    if (ret != 0) {
    } else {
        pubLen = (BN_num_bits(&secret) + 7) / 8;
        padLen = pLen - pubLen;
        if (padLen != 0)
            memset(outBuf, 0, padLen);
        *outLenPtr = BN_bn2bin(&secret, (char *)outBuf + padLen) + padLen;
        ret = 0;
    }

    if (ctx != 0)
        BN_CTX_free(ctx);
    BN_free(&peerPub);
    BN_free(&secret);

    return ret;
}

/* The final slot of the DH method: returns pubKey, padded on the left with zeros to the length of
 * p. */
int pk_dh_sslc_final(PK_CTX *ctxArg, int op, void *outBuf, int *outLenPtr)
{
    DhImpl *dh;
    int pubLen;
    int padLen;
    int pLen;

    if (op != 8)
        return 2;

    dh = ctxArg->impl;
    if (!(dh->flags & 0x2))
        return 3;

    pLen = (BN_num_bits(&dh->p) + 7) / 8;
    pubLen = (BN_num_bits(&dh->pubKey) + 7) / 8;
    padLen = pLen - pubLen;

    if (outBuf != 0) {
        if (padLen != 0)
            memset(outBuf, 0, padLen);
        *outLenPtr = BN_bn2bin(&dh->pubKey, (char *)outBuf + padLen) + padLen;
    } else {
        *outLenPtr = pubLen + padLen;
    }

    return 0;
}

/* Without a modular exponentiation method, info 258 is "-". */

int pk_dh_sslc_get(PK_CTX *ctxArg, int type, void *which, void *out)
{
    DhImpl *dh;
    unsigned char *buf;
    int pLen;
    R_EITEM *item;
    void *ptr;

    if (ctxArg == 0 || ctxArg->impl == 0)
        return 2;
    dh = ctxArg->impl;

    if (type == 257) {
        item = (R_EITEM *)out;
        if (item == 0)
            return 2;

        if (which == (void *)4) {
            ptr = &dh->privKey;
        } else if (which == (void *)3 && (dh->flags & 0x2)) {
            ptr = &dh->pubKey;
        } else if (which == (void *)1) {
            ptr = &dh->p;
        } else if (which == (void *)2) {
            ptr = &dh->g;
        } else {
            return 2;
        }

        if (ptr == 0)
            return 3;

        pLen = (BN_num_bits(ptr) + 7) / 8;
        if ((buf = (unsigned char *)R_malloc(pLen)) == 0)
            return 1;

        BN_bn2bin(ptr, buf);
        R_EITEM_set(item, 18, (int)which, buf, pLen, 0);
        item->flags = item->flags | 2;
        item->type = 2;
        return 0;
    } else if (type == 258) {
        ptr = dh->modExpCtx;
        if (ptr == 0 || ((BN_ME_CTX *)ptr)->meth == 0) {
            *(void **)out = "-";
        } else {
            *(void **)out = ((BN_ME_CTX *)ptr)->meth->name;
        }
        return 0;
    } else if (type == 259) {
        ptr = out;
        *(int *)ptr = (BN_num_bits(&dh->p) + 7) / 8;
    }

    return 0;
}
