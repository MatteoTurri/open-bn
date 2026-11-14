/*
 * KIT_PKCS_RSAPrivate and KIT_PKCS_RSAMultiPrimePrivate: the key info types of an RSA private key.
 * KIT_PKCS_RSAPrivateAddInfo turns a two-prime key into the multi-prime form (the arrays point into
 * the caller's key) and calls KIT_PKCS_RSAMultiPrimeAddInfo.
 */

#include "aglobal.h"
#include "bsafe_kitype.h"


#include "bsafe.h"

B_KeyInfoType KIT_PKCS_RSAPrivate = { KIT_PKCS_RSAPrivateAddInfo, KIT_PKCS_RSAPrivateMakeInfo };
B_KeyInfoType KIT_PKCS_RSAMultiPrimePrivate = { KIT_PKCS_RSAMultiPrimeAddInfo, KIT_PKCS_RSAMultiPrimeMakeInfo };

int KI_PKCS_RSAPrivate(void **info)
{
    *info = &KIT_PKCS_RSAPrivate;
    return 1;
}

int KI_PKCS_RSAMultiPrimePrivate(void **info)
{
    *info = &KIT_PKCS_RSAMultiPrimePrivate;
    return 1;
}

int KIT_PKCS_RSAPrivateAddInfo(void *ctx, KitRsaPrivateKey *header)
{
    KitMpKey mp;

    mp.n.data = header->modulus.data;
    mp.n.len = header->modulus.len;
    mp.e.data = header->publicExponent.data;
    mp.e.len = header->publicExponent.len;
    mp.d.data = header->privateExponent.data;
    mp.d.len = header->privateExponent.len;
    mp.primeCount = 2;
    mp.primes = header->prime;
    mp.exps = header->primeExponent;
    mp.coeffs = &header->coefficient;

    return KIT_PKCS_RSAMultiPrimeAddInfo(ctx, &mp);
}

/* Copies a multi-prime RSA key from the caller's header into a block of the pool of the key
 * object. Two parallel arrays of item pointers (source and destination), 3 * primeCount + 2 long
 * (n, e, d, the primes, their exponents and the primeCount - 1 CRT coefficients), go to
 * AllocAndCopyIntegerItems one item at a time; they are freed on every path. */
int KIT_PKCS_RSAMultiPrimeAddInfo(void *ctx, KitMpKey *header)
{
    KitMpKey *info;
    ITEM **src;
    ITEM **dst;
    int status;
    int n;
    int i, k;

    info = 0;
    dst = 0;
    n = 3 * header->primeCount + 2;

    do {
        status = BE_ALLOC;
        if ((src = (ITEM **)T_malloc(n * sizeof(ITEM *))) == 0)
            break;
        if ((dst = (ITEM **)T_malloc(n * sizeof(ITEM *))) == 0)
            break;
        if ((status = B_MemoryPoolAlloc(ctx, (void **)&info, sizeof(KitMpKey))) != 0)
            break;
        if ((status = B_MemoryPoolAlloc(ctx, (void **)&info->primes,
                                   header->primeCount * sizeof(ITEM))) != 0)
            break;
        if ((status = B_MemoryPoolAlloc(ctx, (void **)&info->exps,
                                   header->primeCount * sizeof(ITEM))) != 0)
            break;
        if ((status = B_MemoryPoolAlloc(ctx, (void **)&info->coeffs,
                                   (header->primeCount - 1) * sizeof(ITEM))) != 0)
            break;

        src[0] = &header->n;
        src[1] = &header->e;
        src[2] = &header->d;
        dst[0] = &info->n;
        dst[1] = &info->e;
        dst[2] = &info->d;

        k = 3;
        i = 0;
        for (;;) {
            src[k] = &header->primes[i];
            dst[k] = &info->primes[i];
            src[k + header->primeCount] = &header->exps[i];
            dst[k + header->primeCount] = &info->exps[i];
            if (i == header->primeCount - 1)
                break;
            src[k + 2 * header->primeCount] = &header->coeffs[i];
            dst[k + 2 * header->primeCount] = &info->coeffs[i];
            k++;
            i++;
        }
        info->primeCount = header->primeCount;

        for (k = 0; k < n; k++) {
            status = AllocAndCopyIntegerItems(dst[k], src[k], src[0], src, 1, ctx);
            if (status != 0)
                break;
        }
    } while (0);

    T_free(src);
    T_free(dst);
    if (status != 0)
        return status;
    return B_InfoCacheAddInfo(ctx, &KIT_PKCS_RSAMultiPrimePrivate, info);
}

/* Copies a two-prime multi-prime key back into a KitRsaPrivateKey. */
int KIT_PKCS_RSAPrivateMakeInfo(KitRsaPrivateKey **out, void *key)
{
    KitMpKey *data;
    KitRsaPrivateKey *dst;
    int status = B_KeyGetInfo(key, (void **)&data, &KIT_PKCS_RSAMultiPrimePrivate);
    if (status != 0)
        return status;
    if (data->primeCount != 2)
        return BE_KEY_INFO;
    if ((status = B_MemoryPoolAlloc(key, (void **)out, 64)) != 0)
        return status;
    dst = *out;
    dst->modulus.data = data->n.data;
    dst->modulus.len = data->n.len;
    dst->publicExponent.data = data->e.data;
    dst->publicExponent.len = data->e.len;
    dst->privateExponent.data = data->d.data;
    dst->privateExponent.len = data->d.len;
    dst->prime[0].data = data->primes[0].data;
    dst->prime[0].len = data->primes[0].len;
    dst->prime[1].data = data->primes[1].data;
    dst->prime[1].len = data->primes[1].len;
    dst->primeExponent[0].data = data->exps[0].data;
    dst->primeExponent[0].len = data->exps[0].len;
    dst->primeExponent[1].data = data->exps[1].data;
    dst->primeExponent[1].len = data->exps[1].len;
    dst->coefficient.data = data->coeffs[0].data;
    dst->coefficient.len = data->coeffs[0].len;
    return 0;
}

int KIT_PKCS_RSAMultiPrimeMakeInfo(void)
{
    return BE_KEY_INFO;
}
