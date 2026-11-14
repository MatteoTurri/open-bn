/*
 * AHChooseBlockCryptor and AHChooseFeedback, the AH object base constructor and the
 * B_Algorithm{En,De}crypt{Init,Update,Final} wrappers that dispatch to the AH object of an algorithm.
 */
#include "ah.h"
#include "bsafe.h"


static int AHChooseBlockCryptorInstantiate(AlgaChoice *slot, void *info, void *initArg, AlgaEntry *alg,
                                           void *extError, void *surrender);
static int AHChooseFeedbackInstantiate(AlgaChoice *slot, void *info, void *initArg, AlgaEntry *alg,
                                       void *extError, void *surrender);


void *AHChooseBlockCryptorConstructor(void *this_, BsafeExtendedError *extError, void *tag, void *params)
{
    AHChooseBlockCryptor *t = this_;
    if (t == 0) {
        t = T_malloc(sizeof(AHChooseBlockCryptor));
        if (t == 0)
            return 0;
    }
    t->base.destroy = 0;
    t->base.extError = extError;
    ResizeContextConstructor(&t->choice.context);
    t->choice.instantiate = (AlgaChooseFn)AHChooseBlockCryptorInstantiate;
    t->choice.tag = tag;
    t->choice.params = params;
    return t;
}

void AHChooseBlockCryptorDestructor(AHChooseBlockCryptor *this_)
{
    ResizeContextDestructor(&this_->choice.context);
}

int AHChooseBlockCryptorGetBlockLen(AHChooseBlockCryptor *this_, unsigned int *outLen)
{
    AHGetBlockLenFn fn;
    int status;
    fn = ((A_BlockCipherAlga *)this_->choice.methods)->GetBlockLen;
    status = fn(this_->choice.context.dataPtr, outLen);
    if (status != 0)
        return ConvertAlgaeError(status);
    return 0;
}

int AHChooseBlockCryptorEncryptInit(void *this_, void *key, void *chooser, void *surrender)
{
    return AlgaChoiceChoose(this_, &((AHChooseBlockCryptor *)this_)->choice, 1, key, 0, chooser, surrender);
}

int AHChooseBlockCryptorDecryptInit(void *this_, void *key, void *chooser, void *surrender)
{
    return AlgaChoiceChoose(this_, &((AHChooseBlockCryptor *)this_)->choice, 0, key, 0, chooser, surrender);
}

/* Instantiates the chosen block cipher in the resize context: asks the algorithm for its context
 * size, allocates it, runs the optional preparation step and the init. */
static int AHChooseBlockCryptorInstantiate(AlgaChoice *slot, void *info,
                                           void *initArg, AlgaEntry *alg,
                                           void *extError, void *surrender)
{
    unsigned int size;
    AHAlgPrepFn prepFn;
    int status;

    status = ((A_BlockCipherAlga *)alg->methods)->Query(&size, info, slot->params, alg, extError);
    if (status != 0)
        return ConvertAlgaeError(status);
    status = ResizeContextMakeNewContext(&slot->context, size);
    if (status != 0)
        return status;
    prepFn = ((A_BlockCipherAlga *)alg->methods)->SetContextDestructor;
    if (prepFn != 0) {
        status = prepFn(&slot->context);
        if (status != 0)
            return ConvertAlgaeError(status);
    }
    status = ((A_BlockCipherAlga *)alg->methods)->Init(slot->context.dataPtr, info, slot->params,
                                                        initArg, alg, extError, surrender);
    if (status != 0)
        return ConvertAlgaeError(status);
    return 0;
}

void *AHChooseFeedbackConstructor(void *this_, BsafeExtendedError *extError, void *tag, void *params)
{
    AHChooseFeedback *t = this_;
    if (t == 0) {
        t = T_malloc(sizeof(AHChooseFeedback));
        if (t == 0)
            return 0;
    }
    t->base.destroy = 0;
    t->base.extError = extError;
    ResizeContextConstructor(&t->choice.context);
    t->choice.instantiate = (AlgaChooseFn)AHChooseFeedbackInstantiate;
    t->choice.tag = tag;
    t->choice.params = params;
    t->ivFlag = 0;
    return t;
}

void AHChooseFeedbackDestructor(AHChooseFeedback *this_)
{
    ResizeContextDestructor(&this_->choice.context);
}

int AHChooseFeedbackGetBlockLen(AHChooseFeedback *this_, unsigned int *outLen)
{
    AHGetBlockLenFn fn;
    int status;
    fn = ((A_FeedbackAlga *)this_->choice.methods)->GetBlockLen;
    status = fn(this_->choice.context.dataPtr, outLen);
    if (status != 0)
        return ConvertAlgaeError(status);
    return 0;
}

/* No key: a feedback mode has none. */
int AHChooseFeedbackEncryptInit(void *this_, void *chooser, void *surrender)
{
    return AlgaChoiceChoose(this_, &((AHChooseFeedback *)this_)->choice, 1, 0, 0, chooser, surrender);
}

int AHChooseFeedbackDecryptInit(void *this_, void *chooser, void *surrender)
{
    return AlgaChoiceChoose(this_, &((AHChooseFeedback *)this_)->choice, 0, 0, 0, chooser, surrender);
}


int AHChooseFeedbackUpdate(AHChooseFeedback *this_, void *blockMethods, void *blockState,
                            void *output, unsigned int *outLen, void *input, unsigned int inputLen,
                            void *surrender)
{
    void *cipherState;
    int status;
    cipherState = this_->choice.context.dataPtr;
    status = ((A_FeedbackAlga *)this_->choice.methods)->Update(cipherState, blockMethods, blockState, output, outLen,
                                                               input, inputLen, surrender);
    if (status != 0)
        return ConvertAlgaeError(status);
    return 0;
}


int AHChooseFeedbackSetNewIVFlag(AHChooseFeedback *this_)
{
    this_->ivFlag = 1;
    return 1;
}

/* The method at slot 8 (the IV step) takes two arguments. */
typedef int (*AHIvStep2Fn)(void *, void *);

int AHChooseFeedbackFinal(AHChooseFeedback *this_, void *blockMethods, void *blockState)
{
    int status;
    if (this_->ivFlag == 1) {
        AHIvStep2Fn stepFn;
        stepFn = ((A_FeedbackAlga *)this_->choice.methods)->ResetIV;
        status = stepFn(this_->choice.context.dataPtr, this_->choice.params);
        if (status != 0)
            return ConvertAlgaeError(status);
    }
    this_->ivFlag = 0;
    {
        AHFeedbackFinalFn finalFn;
        finalFn = ((A_FeedbackAlga *)this_->choice.methods)->Final;
        status = finalFn(this_->choice.context.dataPtr, blockMethods, blockState);
    }
    if (status != 0)
        return ConvertAlgaeError(status);
    return 0;
}

/* The instantiate callback of AHChooseFeedback: as AHChooseBlockCryptorInstantiate, with the
 * preparation step at +24 of the method table instead of +20. */
static int AHChooseFeedbackInstantiate(AlgaChoice *slot, void *info,
                                       void *initArg, AlgaEntry *alg,
                                       void *extError, void *surrender)
{
    unsigned int size;
    AHAlgPrepFn prepFn;
    int status;

    status = ((A_FeedbackAlga *)alg->methods)->Query(&size, info, slot->params, alg, extError);
    if (status != 0)
        return ConvertAlgaeError(status);
    status = ResizeContextMakeNewContext(&slot->context, size);
    if (status != 0)
        return status;
    prepFn = ((A_FeedbackAlga *)alg->methods)->SetContextDestructor;
    if (prepFn != 0) {
        status = prepFn(&slot->context);
        if (status != 0)
            return ConvertAlgaeError(status);
    }
    status = ((A_FeedbackAlga *)alg->methods)->Init(slot->context.dataPtr, info, slot->params,
                                                     initArg, alg, extError, surrender);
    if (status != 0)
        return ConvertAlgaeError(status);
    return 0;
}

/* The class tag of the cipher AH objects (the random generators have their own). */
static void AHCipherDestroy(void *self);

/* The base of every AH object of a cipher: the class tag and the extended error object. */
void AHEncryptDecryptConstructor(void *this_, BsafeExtendedError *extError)
{
    AHObject *obj = this_;
    obj->destroy = AHCipherDestroy;
    obj->extError = extError;
}


/* The type of an algorithm object is the address of a function: for the ciphers the forwarder to
 * slot 0 of the methods (AHCipherDestroy below), for the random generators AHRandomDestroy.
 * B_AlgorithmCheckType compares that pointer. */

/* The Init wrappers clear the choice of the AH object, call its Init method and, if the method
 * chose an algorithm, record the device name with SetDeviceName. Each wrapper calls one slot of
 * BsafeAlgMethods. */
int B_AlgorithmEncryptInit(BsafeAlgorithmObj *obj, BsafeKeyObj *key, void *chooser, BsafeAlgorithmObj *surrender)
{
    BsafeAlgSub *sub;
    int status = B_AlgorithmCheckType(obj, (void *)AHCipherDestroy);
    if (status != 0)
        return status;
    obj->sub->base.chosen = 0;
    sub = obj->sub;
    status = sub->methods->encryptInit(sub, key, chooser, surrender);
    if (status != 0)
        return status;
    if (obj->sub->base.chosen != 0) {
        AlgaChoice *chosen = obj->sub->base.chosen;
        status = SetDeviceName(obj, chosen->tag, 1, chosen->methods, chooser);
        if (status != 0)
            return status;
    }
    obj->flags |= 1;
    return 0;
}

int B_AlgorithmDecryptInit(BsafeAlgorithmObj *obj, BsafeKeyObj *key, void *chooser, BsafeAlgorithmObj *surrender)
{
    BsafeAlgSub *sub;
    int status = B_AlgorithmCheckType(obj, (void *)AHCipherDestroy);
    if (status != 0)
        return status;
    obj->sub->base.chosen = 0;
    sub = obj->sub;
    status = sub->methods->decryptInit(sub, key, chooser, surrender);
    if (status != 0)
        return status;
    if (obj->sub->base.chosen != 0) {
        AlgaChoice *chosen = obj->sub->base.chosen;
        status = SetDeviceName(obj, chosen->tag, 0, chosen->methods, chooser);
        if (status != 0)
            return status;
    }
    obj->flags |= 1;
    return 0;
}

int B_AlgorithmEncryptUpdate(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen, unsigned int maxOutLen,
                              void *input, unsigned int inputLen, BsafeAlgorithmObj *randomAlg, void *surrender)
{
    BsafeAlgSub *sub;
    int status = B_AlgorithmCheckTypeAndInitFlag(obj, (void *)AHCipherDestroy);
    if (status != 0)
        return status;
    sub = obj->sub;
    return sub->methods->encryptUpdate(sub, output, outLen, maxOutLen, input, inputLen, randomAlg, surrender);
}

int B_AlgorithmDecryptUpdate(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen, unsigned int maxOutLen,
                              void *input, unsigned int inputLen, BsafeAlgorithmObj *randomAlg, void *surrender)
{
    BsafeAlgSub *sub;
    int status = B_AlgorithmCheckTypeAndInitFlag(obj, (void *)AHCipherDestroy);
    if (status != 0)
        return status;
    sub = obj->sub;
    return sub->methods->decryptUpdate(sub, output, outLen, maxOutLen, input, inputLen, randomAlg, surrender);
}

int B_AlgorithmEncryptFinal(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen, unsigned int maxOutLen,
                             BsafeAlgorithmObj *randomAlg, void *surrender)
{
    BsafeAlgSub *sub;
    int status = B_AlgorithmCheckTypeAndInitFlag(obj, (void *)AHCipherDestroy);
    if (status != 0)
        return status;
    sub = obj->sub;
    return sub->methods->encryptFinal(sub, output, outLen, maxOutLen, randomAlg, surrender);
}

int B_AlgorithmDecryptFinal(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen, unsigned int maxOutLen,
                             BsafeAlgorithmObj *randomAlg, void *surrender)
{
    BsafeAlgSub *sub;
    int status = B_AlgorithmCheckTypeAndInitFlag(obj, (void *)AHCipherDestroy);
    if (status != 0)
        return status;
    sub = obj->sub;
    return sub->methods->decryptFinal(sub, output, outLen, maxOutLen, randomAlg, surrender);
}

/* Forwards to slot 0 of the methods of the AH object. */

static void AHCipherDestroy(void *self)
{
    BsafeAlgSub *sub = self;
    sub->methods->destroy(sub);
}
