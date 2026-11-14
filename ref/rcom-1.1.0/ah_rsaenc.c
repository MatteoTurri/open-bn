/*
 * AH_RSAEncryption: the AH object of RSA encryption and of RSA signature verification.
 */
#include "ah.h"


/* The method tables of AH_RSAEncryption and AH_RSASignature: Update serves both directions, the
 * signature only changes DecryptFinal. */
extern int AH_RSAEncryptionGetBlockLen(), AH_RSAEncryptionEncryptInit();
extern int AH_RSAEncryptionDecryptInit(), AH_RSAEncryptionUpdate();
extern int AH_RSAEncryptionEncryptFinal(), AH_RSAEncryptionDecryptFinal();

static BsafeAlgMethods ahRsaEncryptionMethods = {
    AH_RSAEncryptionDestructor, AH_RSAEncryptionGetBlockLen,
    AH_RSAEncryptionEncryptInit, AH_RSAEncryptionDecryptInit,
    AH_RSAEncryptionUpdate, AH_RSAEncryptionUpdate,
    AH_RSAEncryptionEncryptFinal, AH_RSAEncryptionDecryptFinal
};
static BsafeAlgMethods ahRsaSignatureMethods = {
    AH_RSAEncryptionDestructor, AH_RSAEncryptionGetBlockLen,
    AH_RSAEncryptionEncryptInit, AH_RSAEncryptionDecryptInit,
    AH_RSAEncryptionUpdate, AH_RSAEncryptionUpdate,
    AH_RSAEncryptionEncryptFinal, AH_RSASignatureDecryptFinal
};

static int AH_RSAEncryptionInitCommon(AH_RSAEncryption *this_, int isEncrypt);


void AH_RSAEncryptionConstructor1(void *this_, BsafeExtendedError *extError, void *tag)
{
    AH_RSAEncryption *t = this_;
    AHChooseEncryptConstructor2(&t->base, extError, tag, 0);
    T_memset(&t->accumBuffer, 0, 8);
    t->base.base.methods = &ahRsaEncryptionMethods;
}

void AH_RSAEncryptionDestructor(AH_RSAEncryption *this_)
{
    T_memset(this_->accumBuffer, 0, this_->accumAllocSize);
    T_free(this_->accumBuffer);
    AHChooseEncryptDestructor(&this_->base);
}

int AH_RSAEncryptionGetBlockLen(void *this_, unsigned int *outLen)
{
    (void)this_;
    (void)outLen;
    return BE_NOT_SUPPORTED;
}

int AH_RSAEncryptionEncryptInit(AH_RSAEncryption *this_, void *key, void *chooser, void *surrender)
{
    int status;
    int (*overrideFn)();

    this_->base.base.base.extError->methodOverride = 0;
    status = AHChooseEncryptEncryptInit(&this_->base, key, chooser, surrender);
    if (status != 0)
        return status;
    overrideFn = this_->base.base.base.extError->methodOverride;
    if (overrideFn != 0)
        this_->rsaMethods->pad = overrideFn;
    return AH_RSAEncryptionInitCommon(this_, 1);
}

int AH_RSAEncryptionDecryptInit(AH_RSAEncryption *this_, void *key, void *chooser, void *surrender)
{
    int status;
    int (*overrideFn)();

    this_->base.base.base.extError->methodOverride = 0;
    status = AHChooseEncryptDecryptInit(&this_->base, key, chooser, surrender);
    if (status != 0)
        return status;
    overrideFn = this_->base.base.base.extError->methodOverride;
    if (overrideFn != 0)
        this_->rsaMethods->unpad = overrideFn;
    return AH_RSAEncryptionInitCommon(this_, 0);
}

int AH_RSAEncryptionUpdate(AH_RSAEncryption *this_, void *unused1, unsigned int *outLen, void *unused3, void *in, unsigned int inLen)
{
    (void)unused1;
    (void)unused3;
    *outLen = 0;
    if (this_->accumCapacity < this_->accumUsed + inLen)
        return BE_INPUT_LEN;
    T_memcpy(this_->accumBuffer + this_->accumUsed, in, inLen);
    this_->accumUsed += inLen;
    return 0;
}

int AH_RSAEncryptionEncryptFinal(AH_RSAEncryption *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                    void *randomAlg, void *surrender)
{
    AHRsaPrepFn prepFn;
    int status;
    unsigned int scratch;

    prepFn = this_->rsaMethods->pad;
    status = prepFn(this_, randomAlg, surrender);
    if (status != 0)
        return status;
    status = AHChooseEncryptEncryptUpdate(&this_->base, output, outLen, maxOutLen,
                                            this_->accumBuffer, this_->accumAllocSize, 0, surrender);
    if (status != 0)
        return status;
    status = AHChooseEncryptEncryptFinal(&this_->base, 0, &scratch, 0, 0, surrender);
    if (status != 0)
        return status;
    this_->accumUsed = 0;
    return 0;
}

int AH_RSAEncryptionDecryptFinal(AH_RSAEncryption *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                    void *randomAlg, void *surrender)
{
    unsigned int decryptedLen;
    ITEM unpadded;
    int status;
    (void)randomAlg;

    status = AHChooseEncryptDecryptUpdate(&this_->base, this_->accumBuffer, &decryptedLen, this_->accumAllocSize,
                                            this_->accumBuffer, this_->accumUsed, 0, surrender);
    if (status != 0)
        return status;
    {
        unsigned int scratch2;
        status = AHChooseEncryptDecryptFinal(&this_->base, 0, &scratch2, 0, 0, surrender);
    }
    if (status != 0)
        return status;
    this_->accumUsed = 0;
    {
        AHRsaCryptFn decryptFn;
        decryptFn = this_->rsaMethods->unpad;
        status = decryptFn(this_, &unpadded, decryptedLen);
    }
    if (status != 0)
        return status;
    if (this_->accumAllocSize - 11 < unpadded.len)
        return BE_INPUT_DATA;
    *outLen = unpadded.len;
    if (maxOutLen < unpadded.len)
        return BE_OUTPUT_LEN;
    T_memcpy(output, unpadded.data, unpadded.len);
    return 0;
}

int AH_RSASignatureDecryptFinal(AH_RSAEncryption *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                   void *randomAlg, void *surrender)
{
    unsigned int decryptedLen;
    ITEM unpadded;
    int status;
    (void)randomAlg;

    decryptedLen = *outLen;
    status = AHChooseEncryptDecryptUpdate(&this_->base, output, &decryptedLen, this_->accumAllocSize,
                                            this_->accumBuffer, this_->accumUsed, 0, surrender);
    if (status != 0)
        return status;
    T_memcpy(this_->accumBuffer, output, decryptedLen);
    {
        unsigned int scratch2;
        status = AHChooseEncryptDecryptFinal(&this_->base, 0, &scratch2, 0, 0, surrender);
    }
    if (status != 0)
        return status;
    this_->accumUsed = 0;
    {
        AHRsaCryptFn decryptFn;
        decryptFn = this_->rsaMethods->unpad;
        status = decryptFn(this_, &unpadded, decryptedLen);
    }
    if (status != 0)
        return status;
    if (this_->accumAllocSize - 11 < unpadded.len)
        return BE_INPUT_DATA;
    *outLen = unpadded.len;
    if (maxOutLen < unpadded.len)
        return BE_OUTPUT_LEN;
    T_memcpy(output, unpadded.data, unpadded.len);
    return 0;
}

/* Sizes the input buffer for the block length of the chosen algorithm: to encrypt, the block
 * minus the 11 bytes of PKCS #1 padding. */
static int AH_RSAEncryptionInitCommon(AH_RSAEncryption *this_, int isEncrypt)
{
    unsigned int blockLen;
    int status;
    void *newBuf;

    status = AHChooseEncryptGetBlockLen(&this_->base, &blockLen);
    if (status != 0)
        return status;
    if (blockLen < 12)
        return BE_NOT_SUPPORTED;
    this_->accumCapacity = isEncrypt ? (blockLen - 11) : blockLen;
    this_->accumUsed = 0;
    T_memset(this_->accumBuffer, 0, this_->accumAllocSize);
    newBuf = T_realloc(this_->accumBuffer, blockLen);
    this_->accumBuffer = newBuf;
    if (newBuf == 0) {
        this_->accumAllocSize = 0;
        return BE_ALLOC;
    }
    this_->accumAllocSize = blockLen;
    return 0;
}

void *AH_RSAEncrypSignatureConstructor2(void *this_)
{
    BsafeAlgMethods *methods = &ahRsaSignatureMethods;
    ((AH_RSAEncryption *)this_)->base.base.methods = methods;
    return methods;
}
