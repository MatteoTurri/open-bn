/*
 * AHFeedbackCipher: a block cipher in a feedback mode, both chosen at Init (see ah.h).
 */
#include "ah.h"


/* The method table of AHFeedbackCipher: Destructor, GetBlockLen, Encrypt/DecryptInit,
 * Encrypt/DecryptUpdate, Encrypt/DecryptFinal. */
extern int AHFeedbackCipherGetBlockLen(), AHFeedbackCipherEncryptInit();
extern int AHFeedbackCipherDecryptInit(), AHFeedbackCipherEncryptUpdate();
extern int AHFeedbackCipherDecryptUpdate(), AHFeedbackCipherEncryptFinal();

static BsafeAlgMethods ahFeedbackCipherMethods = {
    AHFeedbackCipherDestructor, AHFeedbackCipherGetBlockLen,
    AHFeedbackCipherEncryptInit, AHFeedbackCipherDecryptInit,
    AHFeedbackCipherEncryptUpdate, AHFeedbackCipherDecryptUpdate,
    AHFeedbackCipherEncryptFinal, AHFeedbackCipherDecryptFinal
};


void *AHFeedbackCipherConstructor(void *this_, BsafeExtendedError *extError, void *encryptionAIT, void *encryptionInfo,
                                    void *feedbackAIT, void *feedbackInfo, int paddingType, int feedbackType)
{
    AHFeedbackCipher *t = this_;
    if (t == 0) {
        t = T_malloc(sizeof(AHFeedbackCipher));
        if (t == 0)
            return 0;
    }
    T_memset(t, 0, sizeof(AHFeedbackCipher));
    AHEncryptDecryptConstructor(t, extError);
    t->base.methods = &ahFeedbackCipherMethods;
    t->paddingType = paddingType;
    t->feedbackType = feedbackType;
    AHChooseFeedbackConstructor(&t->feedback, extError, feedbackAIT, feedbackInfo);
    AHChooseBlockCryptorConstructor(&t->blockCryptor, extError, encryptionAIT, encryptionInfo);
    return t;
}

void AHFeedbackCipherDestructor(AHFeedbackCipher *this_)
{
    AHChooseFeedbackDestructor(&this_->feedback);
    AHChooseBlockCryptorDestructor(&this_->blockCryptor);
    if (this_->buffer != 0) {
        T_memset(this_->buffer, 0, this_->blockLen * 2);
        T_free(this_->buffer);
    }
}

int AHFeedbackCipherGetBlockLen(AHFeedbackCipher *this_, unsigned int *outLen)
{
    *outLen = this_->blockLen;
    return 0;
}

int AHFeedbackCipherEncryptInit(AHFeedbackCipher *this_, void *key, void *chooser, void *surrender)
{
    unsigned int blockLenA, blockLenB;
    void *newBuf;
    int status;

    status = AHChooseBlockCryptorEncryptInit(&this_->blockCryptor, key, chooser, surrender);
    if (status != 0)
        return status;
    status = AHChooseBlockCryptorGetBlockLen(&this_->blockCryptor, &blockLenA);
    if (status != 0)
        return status;
    status = AHChooseFeedbackEncryptInit(&this_->feedback, chooser, surrender);
    if (status != 0)
        return status;
    status = AHChooseFeedbackGetBlockLen(&this_->feedback, &blockLenB);
    if (status != 0)
        return status;
    if (blockLenB != blockLenA)
        return BE_INPUT_DATA;

    if (this_->buffer != 0) {
        T_memset(this_->buffer, 0, this_->blockLen * 2);
        T_free(this_->buffer);
    }
    this_->blockLen = blockLenB;
    newBuf = T_malloc(blockLenB * 2);
    this_->buffer = newBuf;
    this_->bufferedLen = 0;
    return newBuf != 0 ? 0 : BE_ALLOC;
}

int AHFeedbackCipherDecryptInit(AHFeedbackCipher *this_, void *key, void *chooser, void *surrender)
{
    unsigned int blockLenA, blockLenB;
    void *newBuf;
    int status;

    if ((unsigned int)(this_->feedbackType - 4) < 4)
        status = AHChooseBlockCryptorEncryptInit(&this_->blockCryptor, key, chooser, surrender);
    else
        status = AHChooseBlockCryptorDecryptInit(&this_->blockCryptor, key, chooser, surrender);
    if (status != 0)
        return status;
    status = AHChooseBlockCryptorGetBlockLen(&this_->blockCryptor, &blockLenA);
    if (status != 0)
        return status;
    status = AHChooseFeedbackDecryptInit(&this_->feedback, chooser, surrender);
    if (status != 0)
        return status;
    status = AHChooseFeedbackGetBlockLen(&this_->feedback, &blockLenB);
    if (status != 0)
        return status;
    if (blockLenB != blockLenA)
        return BE_INPUT_DATA;

    if (this_->buffer != 0) {
        T_memset(this_->buffer, 0, this_->blockLen * 2);
        T_free(this_->buffer);
    }
    this_->blockLen = blockLenB;
    newBuf = T_malloc(blockLenB * 2);
    this_->buffer = newBuf;
    this_->bufferedLen = 0;
    return newBuf != 0 ? 0 : BE_ALLOC;
}

/* Buffers to whole blocks: `bufferedLen` counts the bytes buffered. With padding type 2 (stream) the data
 * goes straight to the feedback mode. Otherwise a short input is only buffered; a longer one first
 * completes the buffered block, then processes the whole blocks and buffers the rest. The feedback
 * mode gets the methods and state of the block cipher. */
int AHFeedbackCipherEncryptUpdate(AHFeedbackCipher *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                    void *in, unsigned int inLen, void *randomAlg, void *surrender)
{
    unsigned int leftover;
    unsigned int blockLen;
    unsigned int totalAvailable;
    unsigned int produced = 0;
    int status;
    (void)randomAlg;

    *outLen = 0;
    leftover = this_->bufferedLen;
    blockLen = this_->blockLen;

    if (this_->paddingType == 2) {
        if (maxOutLen < inLen)
            return BE_OUTPUT_LEN;
        return AHChooseFeedbackUpdate(&this_->feedback, this_->blockCryptor.choice.methods,
                                      this_->blockCryptor.choice.context.dataPtr, output, outLen,
                                      in, inLen, surrender);
    }

    totalAvailable = leftover + inLen;
    if (totalAvailable < blockLen) {
        T_memcpy((char *)this_->buffer + leftover, in, inLen);
        this_->bufferedLen += inLen;
        return 0;
    }
    {
        unsigned int numBlocks = totalAvailable / blockLen;
        unsigned int bytesToProcess = numBlocks * blockLen;
        void *inCursor;
        void *outCursor;

        if (maxOutLen < bytesToProcess)
            return BE_OUTPUT_LEN;
        inCursor = in;
        outCursor = output;

        produced = 0;
        if (leftover != 0) {
            inCursor = this_->buffer;
            T_memcpy((char *)inCursor + leftover, in, blockLen - leftover);
            status = AHChooseFeedbackUpdate(&this_->feedback, this_->blockCryptor.choice.methods,
                                            this_->blockCryptor.choice.context.dataPtr, outCursor, &produced,
                                            inCursor, blockLen, surrender);
            if (status != 0)
                return status;
            inCursor = (char *)in + blockLen - leftover;
            outCursor = (char *)outCursor + produced;
            *outLen = produced;
        }
        if (produced < bytesToProcess) {
            status = AHChooseFeedbackUpdate(&this_->feedback, this_->blockCryptor.choice.methods,
                                            this_->blockCryptor.choice.context.dataPtr, outCursor, &produced,
                                            inCursor, bytesToProcess - produced, surrender);
            if (status != 0)
                return status;
            *outLen = *outLen + produced;
        }
        leftover = totalAvailable - bytesToProcess;
        if (bytesToProcess < totalAvailable) {
            T_memcpy(this_->buffer, (char *)in + inLen - leftover, leftover);
            this_->bufferedLen = leftover;
        } else {
            this_->bufferedLen = 0;
        }
        return 0;
    }
}

/* Final dispatches on the padding type (0 none, 1 pad, 2 stream) and hands the methods and state of
 * the block cipher to AHChooseFeedbackUpdate/Final. */
int AHFeedbackCipherEncryptFinal(AHFeedbackCipher *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                   void *randomAlg, void *surrender)
{
    int status;
    unsigned int leftover;
    unsigned int i;
    unsigned int pad;
    (void)randomAlg;

    switch ((unsigned int)this_->paddingType) {
    case 0:
        if (this_->bufferedLen != 0)
            return BE_INPUT_LEN;
        *outLen = 0;
        break;
    case 1:
        if (maxOutLen < this_->blockLen)
            return BE_INPUT_LEN;
        /* padding: every byte holds the number of bytes added */
        leftover = this_->bufferedLen;
        pad = this_->blockLen - leftover;
        for (i = leftover; i < this_->blockLen; i++)
            ((unsigned char *)this_->buffer)[i] = (unsigned char)pad;
        status = AHChooseFeedbackUpdate(&this_->feedback, this_->blockCryptor.choice.methods,
                                        this_->blockCryptor.choice.context.dataPtr, output, outLen,
                                        this_->buffer, this_->blockLen, surrender);
        if (status != 0)
            return status;
        this_->bufferedLen = 0;
        break;
    case 2:
        *outLen = 0;
        break;
    default:
        return BE_NOT_SUPPORTED;
    }
    return AHChooseFeedbackFinal(&this_->feedback, this_->blockCryptor.choice.methods,
                                 this_->blockCryptor.choice.context.dataPtr);
}

/* The mirror of EncryptUpdate, except that it keeps the last block of ciphertext buffered for
 * Final, which removes the padding. */
int AHFeedbackCipherDecryptUpdate(AHFeedbackCipher *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                    void *in, unsigned int inLen, void *randomAlg, void *surrender)
{
    unsigned int leftover;
    unsigned int blockLen;
    unsigned int totalAvailable;
    unsigned int bytesToProcess;
    unsigned int produced = 0;
    void *inCursor;
    void *outCursor;
    int status;
    int paddingType;
    (void)randomAlg;

    *outLen = 0;
    leftover = this_->bufferedLen;
    paddingType = this_->paddingType;
    blockLen = this_->blockLen;
    totalAvailable = leftover + inLen;

    if (paddingType == 2) {
        if (maxOutLen < inLen)
            return BE_OUTPUT_LEN;
        return AHChooseFeedbackUpdate(&this_->feedback, this_->blockCryptor.choice.methods,
                                      this_->blockCryptor.choice.context.dataPtr, output, outLen,
                                      in, inLen, surrender);
    }

    {
        unsigned int limit = blockLen - (paddingType == 0);
        if (totalAvailable <= limit) {
            T_memcpy((char *)this_->buffer + leftover, in, inLen);
            this_->bufferedLen += inLen;
            return 0;
        }
    }
    bytesToProcess = totalAvailable - (paddingType != 0);
    bytesToProcess = (bytesToProcess / blockLen) * blockLen;
    if (maxOutLen < bytesToProcess)
        return BE_OUTPUT_LEN;
    inCursor = in;
    outCursor = output;

    produced = 0;
    if (leftover != 0) {
        inCursor = this_->buffer;
        T_memcpy((char *)inCursor + leftover, in, blockLen - leftover);
        status = AHChooseFeedbackUpdate(&this_->feedback, this_->blockCryptor.choice.methods,
                                        this_->blockCryptor.choice.context.dataPtr, outCursor, &produced,
                                        inCursor, blockLen, surrender);
        if (status != 0)
            return status;
        inCursor = (char *)in + blockLen - leftover;
        outCursor = (char *)outCursor + produced;
        *outLen = produced;
    }
    if (produced < bytesToProcess) {
        status = AHChooseFeedbackUpdate(&this_->feedback, this_->blockCryptor.choice.methods,
                                        this_->blockCryptor.choice.context.dataPtr, outCursor, &produced,
                                        inCursor, bytesToProcess - produced, surrender);
        if (status != 0)
            return status;
        *outLen = *outLen + produced;
    }
    leftover = totalAvailable - bytesToProcess;
    if (bytesToProcess < totalAvailable) {
        T_memcpy(this_->buffer, (char *)in + inLen - leftover, leftover);
        this_->bufferedLen = leftover;
    } else {
        this_->bufferedLen = 0;
    }
    return 0;
}

/* The mirror of EncryptFinal: with padding type 1 it decrypts the last block and removes the
 * padding, checking that the last N bytes all hold N. */
int AHFeedbackCipherDecryptFinal(AHFeedbackCipher *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                   void *randomAlg, void *surrender)
{
    int status;
    unsigned char padValue;
    unsigned int i;
    (void)randomAlg;

    switch ((unsigned int)this_->paddingType) {
    case 0:
        if (this_->bufferedLen != 0)
            return BE_INPUT_LEN;
        *outLen = 0;
        break;
    case 1:
        if (this_->bufferedLen != this_->blockLen)
            return BE_INPUT_LEN;
        if (maxOutLen < this_->blockLen)
            return BE_OUTPUT_LEN;

        status = AHChooseFeedbackUpdate(&this_->feedback, this_->blockCryptor.choice.methods,
                                        this_->blockCryptor.choice.context.dataPtr, output, outLen,
                                        this_->buffer, this_->blockLen, surrender);
        if (status != 0)
            return status;

        padValue = ((unsigned char *)output)[*outLen - 1];
        if (padValue == 0)
            return BE_INPUT_DATA;
        if (this_->blockLen < padValue)
            return BE_INPUT_DATA;
        for (i = this_->blockLen - padValue; i < this_->blockLen; i++)
            if (((unsigned char *)output)[i] != padValue)
                return BE_INPUT_DATA;
        *outLen -= padValue;
        this_->bufferedLen = 0;
        break;
    case 2:
        *outLen = 0;
        break;
    default:
        return BE_NOT_SUPPORTED;
    }
    return AHChooseFeedbackFinal(&this_->feedback, this_->blockCryptor.choice.methods,
                                 this_->blockCryptor.choice.context.dataPtr);
}

int AHFeedbackCipherReInitIV(AHFeedbackCipher *this_, ITEM *curInfo, ITEM *newInfo)
{
    unsigned int len = curInfo->len;
    if (len != newInfo->len)
        return BE_INPUT_LEN;
    T_memcpy(curInfo->data, newInfo->data, len);
    AHChooseFeedbackSetNewIVFlag(&this_->feedback);
    return 0;
}
