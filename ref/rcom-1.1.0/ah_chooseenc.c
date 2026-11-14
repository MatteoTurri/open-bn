/*
 * AHChooseEncrypt, the random generator AH object and the B_AlgorithmRandom* wrappers.
 */
#include "ah.h"


/* The method table of AHChooseEncrypt. */
extern int AHChooseEncryptGetBlockLen(), AHChooseEncryptEncryptInit();
extern int AHChooseEncryptDecryptInit(), AHChooseEncryptEncryptUpdate();
extern int AHChooseEncryptDecryptUpdate(), AHChooseEncryptEncryptFinal();

static BsafeAlgMethods ahChooseEncryptMethods = {
    AHChooseEncryptDestructor, AHChooseEncryptGetBlockLen,
    AHChooseEncryptEncryptInit, AHChooseEncryptDecryptInit,
    AHChooseEncryptEncryptUpdate, AHChooseEncryptDecryptUpdate,
    AHChooseEncryptEncryptFinal, AHChooseEncryptDecryptFinal
};


static int AHChooseEncryptInstantiate(AlgaChoice *slot, void *info, void *initArg, AlgaEntry *alg,
                                      void *extError, void *surrender);


void *AHChooseEncryptConstructor2(void *this_, BsafeExtendedError *extError, void *tag, void *params)
{
    AHChooseEncrypt *t = this_;
    if (t == 0) {
        t = T_malloc(sizeof(AHChooseEncrypt));
        if (t == 0)
            return 0;
    }
    AHEncryptDecryptConstructor(t, extError);
    ResizeContextConstructor(&t->choice.context);
    t->choice.instantiate = (AlgaChooseFn)AHChooseEncryptInstantiate;
    t->base.methods = &ahChooseEncryptMethods;
    t->choice.tag = tag;
    t->choice.params = params;
    t->initArg = 0;
    return t;
}

void AHChooseEncryptDestructor(AHChooseEncrypt *this_)
{
    ResizeContextDestructor(&this_->choice.context);
}

int AHChooseEncryptGetBlockLen(AHChooseEncrypt *this_, unsigned int *outLen)
{
    AHGetBlockLenFn fn;
    int status;
    fn = ((A_PublicKeyAlga *)this_->choice.methods)->GetBlockLen;
    status = fn(this_->choice.context.dataPtr, outLen);
    if (status != 0)
        return ConvertAlgaeError(status);
    return 0;
}

int AHChooseEncryptEncryptInit(AHChooseEncrypt *this_, void *key, void *chooser, void *surrender)
{
    void *slot = &this_->choice;
    int status = AlgaChoiceChoose(this_, slot, 1, key, this_->initArg, chooser, surrender);
    if (status != 0)
        return status;
    this_->base.base.chosen = slot;
    return 0;
}

int AHChooseEncryptDecryptInit(AHChooseEncrypt *this_, void *key, void *chooser, void *surrender)
{
    void *slot = &this_->choice;
    int status = AlgaChoiceChoose(this_, slot, 0, key, this_->initArg, chooser, surrender);
    if (status != 0)
        return status;
    this_->base.base.chosen = slot;
    return 0;
}

int AHChooseEncryptEncryptUpdate(AHChooseEncrypt *this_, void *out, unsigned int *outLen, unsigned int maxOutLen,
                                  void *in, unsigned int inLen, void *randomAlg, void *surrender)
{
    void *cipherState;
    int status;
    (void)randomAlg;
    cipherState = this_->choice.context.dataPtr;
    status = ((A_PublicKeyAlga *)this_->choice.methods)->Update(cipherState, out, outLen, maxOutLen, in, inLen, surrender);
    if (status != 0)
        return ConvertAlgaeError(status);
    return 0;
}

int AHChooseEncryptDecryptUpdate(AHChooseEncrypt *this_, void *out, unsigned int *outLen, unsigned int maxOutLen,
                                   void *in, unsigned int inLen, void *randomAlg, void *surrender)
{
    void *cipherState;
    int status;
    (void)randomAlg;
    cipherState = this_->choice.context.dataPtr;
    status = ((A_PublicKeyAlga *)this_->choice.methods)->Update(cipherState, out, outLen, maxOutLen, in, inLen, surrender);
    if (status != 0)
        return ConvertAlgaeError(status);
    return 0;
}

int AHChooseEncryptEncryptFinal(AHChooseEncrypt *this_, void *out, unsigned int *outLen, unsigned int maxOutLen,
                                  void *randomAlg, void *surrender)
{
    void *cipherState;
    int status;
    (void)randomAlg;
    cipherState = this_->choice.context.dataPtr;
    status = ((A_PublicKeyAlga *)this_->choice.methods)->Final(cipherState, out, outLen, maxOutLen, surrender);
    if (status != 0)
        return ConvertAlgaeError(status);
    return 0;
}

int AHChooseEncryptDecryptFinal(AHChooseEncrypt *this_, void *out, unsigned int *outLen, unsigned int maxOutLen,
                                   void *randomAlg, void *surrender)
{
    void *cipherState;
    int status;
    (void)randomAlg;
    cipherState = this_->choice.context.dataPtr;
    status = ((A_PublicKeyAlga *)this_->choice.methods)->Final(cipherState, out, outLen, maxOutLen, surrender);
    if (status != 0)
        return ConvertAlgaeError(status);
    return 0;
}


/* As AHChooseBlockCryptorInstantiate, for AHChooseEncrypt (its choice is at +16 instead of +12). */
static int AHChooseEncryptInstantiate(AlgaChoice *slot, void *info,
                                           void *initArg, AlgaEntry *alg,
                                           void *extError, void *surrender)
{
    unsigned int size;
    AHAlgPrepFn prepFn;
    int status;

    status = ((A_PublicKeyAlga *)alg->methods)->Query(&size, info, slot->params, alg, extError);
    if (status != 0)
        return ConvertAlgaeError(status);
    status = ResizeContextMakeNewContext(&slot->context, size);
    if (status != 0)
        return status;
    prepFn = ((A_PublicKeyAlga *)alg->methods)->SetContextDestructor;
    if (prepFn != 0) {
        status = prepFn(&slot->context);
        if (status != 0)
            return ConvertAlgaeError(status);
    }
    status = ((A_PublicKeyAlga *)alg->methods)->Init(slot->context.dataPtr, info, slot->params,
                                                      initArg, alg, extError, surrender);
    if (status != 0)
        return ConvertAlgaeError(status);
    return 0;
}

/* The class tag of the random generator AH objects. */
static void AHRandomDestroy(void *self);


/* The base of the random generator AH objects, with their own class tag. */
void AHRandomConstructor(void *this_, BsafeExtendedError *extError)
{
    AHObject *obj = this_;
    obj->destroy = AHRandomDestroy;
    obj->extError = extError;
}



int B_AlgorithmRandomInit(BsafeAlgorithmObj *obj, void *chooser, void *surrender)
{
    int status = B_AlgorithmCheckType(obj, (void *)AHRandomDestroy);
    if (status != 0)
        return status;
    status = ((BsafeRandomMethods *)obj->sub->methods)->randomInit(obj->sub, chooser, surrender);
    if (status != 0)
        return status;
    obj->flags |= 1;
    return 0;
}

int B_AlgorithmRandomUpdate(BsafeAlgorithmObj *obj, void *data, unsigned int len, void *surrender)
{
    int status = B_AlgorithmCheckTypeAndInitFlag(obj, (void *)AHRandomDestroy);
    if (status != 0)
        return (status == BE_ALGORITHM_NOT_INITIALIZED) ? BE_RANDOM_NOT_INITIALIZED : status;
    return ((BsafeRandomMethods *)obj->sub->methods)->randomUpdate(obj->sub, data, len, surrender);
}

int B_AlgorithmGenerateRandomBytes(BsafeAlgorithmObj *obj, void *data, unsigned int len, void *surrender)
{
    int status;
    if (obj == 0)
        return BE_RANDOM_OBJ;
    status = B_AlgorithmCheckTypeAndInitFlag(obj, (void *)AHRandomDestroy);
    if (status != 0)
        return (status == BE_ALGORITHM_NOT_INITIALIZED) ? BE_RANDOM_NOT_INITIALIZED : status;
    return ((BsafeRandomMethods *)obj->sub->methods)->generateBytes(obj->sub, data, len, surrender);
}

static void AHRandomDestroy(void *self)
{
    BsafeAlgSub *sub = self;
    ((BsafeRandomMethods *)sub->methods)->destroy(sub);
}
