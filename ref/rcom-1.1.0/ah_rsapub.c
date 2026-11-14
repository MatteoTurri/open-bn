/*
 * The RSA public key encryptor (AH_RSAEncrypPublicConstructor) and its PKCS #1 padding.
 */
#include "ah.h"
#include "bsafe.h"
#include "bsafe_aitype.h"


/* Type 2 padding to encrypt, type 1 unpadding to verify. */
static int AHRsaPkcsPad();
static int AHRsaPkcsUnpad();
static AHRsaMethods ahRsaPublicMethods = { AHRsaPkcsPad, AHRsaPkcsUnpad };


void *AH_RSAEncrypPublicConstructor(void *this_, BsafeExtendedError *extError)
{
    AH_RSAEncryption *t = this_;
    if (t == 0) {
        t = T_malloc(sizeof(AH_RSAEncryption));
        if (t == 0)
            return 0;
    }
    AH_RSAEncryptionConstructor1(t, extError, &AIT_RSAPublic);
    t->rsaMethods = &ahRsaPublicMethods;
    return t;
}

/* Applies PKCS #1 type 2 padding to the input buffer: moves the input to the end, writes 00 02,
 * fills the rest with nonzero random bytes and the 00 separator. BE_INPUT_LEN when there is no room for the
 * header. */
static int AHRsaPkcsPad(AH_RSAEncryption *this_, void *randomAlg, void *surrender)
{
    unsigned char b;
    unsigned int i;
    unsigned int padLen;
    unsigned int avail;
    int status;

    if (this_->accumAllocSize < this_->accumUsed + 3)
        return BE_INPUT_LEN;

    avail = this_->accumAllocSize - 3;
    padLen = avail - this_->accumUsed;
    T_memmove(this_->accumBuffer + padLen + 3,
              this_->accumBuffer, this_->accumUsed);
    this_->accumBuffer[0] = 0;
    this_->accumBuffer[1] = 2;

    for (i = 2; i < padLen + 2; i++) {
        do {
            status = B_AlgorithmGenerateRandomBytes(randomAlg, &b, 1, surrender);
            if (status != 0)
                return status;
        } while (b == 0);
        this_->accumBuffer[i] = b;
    }
    this_->accumBuffer[padLen + 2] = 0;
    return 0;
}

/* The inverse for a type 1 block (signature): checks 00 01, skips the 0xFF bytes, requires the 00
 * separator and returns the rest of the block in out. BE_INPUT_DATA when the padding is broken. */
static int AHRsaPkcsUnpad(AH_RSAEncryption *this_, ITEM *out, unsigned int len)
{
    unsigned int i;

    if (this_->accumBuffer[0] != 0 ||
        this_->accumBuffer[1] != 1)
        return BE_INPUT_DATA;
    for (i = 2; i < len && this_->accumBuffer[i] == 0xff; i++)
        ;
    i++;
    if (i > len || this_->accumBuffer[i - 1] != 0)
        return BE_INPUT_DATA;
    out->len = len - i;
    out->data = this_->accumBuffer + i;
    return 0;
}
