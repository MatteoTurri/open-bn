/*
 * B_EncryptInit/Update/Final and B_DecryptInit/Update/Final.
 */
#include "bsafe.h"


int B_EncryptInit(BsafeAlgorithmObj *obj, BsafeKeyObj *key, void *chooser, BsafeAlgorithmObj *surrender)
{
    int status = AlgorithmWrapCheck(obj);
    if (status != 0)
        return status;
    status = KeyWrapCheck(key);
    if (status != 0)
        return status;
    return B_AlgorithmEncryptInit(obj, key, chooser, surrender);
}

int B_EncryptUpdate(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen, unsigned int maxOutLen,
                     void *input, unsigned int inputLen, BsafeAlgorithmObj *randomAlg, void *surrender)
{
    int status = AlgorithmWrapCheck(obj);
    if (status != 0)
        return status;
    status = RandomAlgorithmCheck(randomAlg);
    if (status != 0)
        return status;
    return B_AlgorithmEncryptUpdate(obj, output, outLen, maxOutLen, input, inputLen, randomAlg, surrender);
}

int B_EncryptFinal(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen, unsigned int maxOutLen,
                    BsafeAlgorithmObj *randomAlg, void *surrender)
{
    int status = AlgorithmWrapCheck(obj);
    if (status != 0)
        return status;
    status = RandomAlgorithmCheck(randomAlg);
    if (status != 0)
        return status;
    return B_AlgorithmEncryptFinal(obj, output, outLen, maxOutLen, randomAlg, surrender);
}

int B_DecryptInit(BsafeAlgorithmObj *obj, BsafeKeyObj *key, void *chooser, BsafeAlgorithmObj *surrender)
{
    int status = AlgorithmWrapCheck(obj);
    if (status != 0)
        return status;
    status = KeyWrapCheck(key);
    if (status != 0)
        return status;
    return B_AlgorithmDecryptInit(obj, key, chooser, surrender);
}

int B_DecryptUpdate(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen, unsigned int maxOutLen,
                     void *input, unsigned int inputLen, BsafeAlgorithmObj *randomAlg, void *surrender)
{
    int status = AlgorithmWrapCheck(obj);
    if (status != 0)
        return status;
    status = RandomAlgorithmCheck(randomAlg);
    if (status != 0)
        return status;
    return B_AlgorithmDecryptUpdate(obj, output, outLen, maxOutLen, input, inputLen, randomAlg, surrender);
}

int B_DecryptFinal(BsafeAlgorithmObj *obj, void *output, unsigned int *outLen, unsigned int maxOutLen,
                    BsafeAlgorithmObj *randomAlg, void *surrender)
{
    int status = AlgorithmWrapCheck(obj);
    if (status != 0)
        return status;
    status = RandomAlgorithmCheck(randomAlg);
    if (status != 0)
        return status;
    return B_AlgorithmDecryptFinal(obj, output, outLen, maxOutLen, randomAlg, surrender);
}
