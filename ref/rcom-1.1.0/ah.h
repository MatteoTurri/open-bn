/*
 * The AH objects of BSAFE, which an algorithm object dispatches to. AHChooseBlockCryptor,
 * AHChooseFeedback and AHChooseEncrypt wait for a choice: at Init AlgaChoiceChoose picks an algorithm
 * and calls the `instantiate` callback, which builds the state of the algorithm in the resize context
 * of the choice. AH_RSAEncryption embeds an AHChooseEncrypt and replaces its method table;
 * AHFeedbackCipher embeds an AHChooseFeedback and an AHChooseBlockCryptor. The methods of a chosen
 * algorithm are the A_*Alga tables of bsafe_amtype.h.
 */
#ifndef AH_H
#define AH_H

#include "bsafe.h"
#include "bsafe_amtype.h"

/* Function pointer types read from the method tables. */
typedef int (*AHGetBlockLenFn)(void *, unsigned int *);
typedef int (*AHFeedbackFinalFn)(void *, void *, void *);
typedef int (*AHRsaPrepFn)(void *, void *, void *);
typedef int (*AHRsaCryptFn)(void *, void *, unsigned int);

/* The RSA padding of an AH_RSAEncryption: pad before encrypting, unpad after decrypting. */
typedef struct AHRsaMethods {
    /* 0x00 */ AHRsaPrepFn pad;
    /* 0x04 */ AHRsaCryptFn unpad;
} AHRsaMethods; /* size = 0x08 */
typedef int (*AHAlgPrepFn)(void *);

/* An algorithm a chooser can pick: AlgaChoiceChoose walks a null-terminated list of them. */
typedef struct AlgaEntry {
    /* 0x00 */ void *tag;  /* compared with AlgaChoice.tag */
    /* 0x04 */ int mode;  /* 1 encrypt, 0 decrypt */
    /* 0x08 */ void *infoType;  /* key info to fetch with B_KeyGetInfo; 0 = none */
    /* 0x0C */ void *methods;  /* installed in AlgaChoice.methods on success */
} AlgaEntry; /* size = 0x10 */

typedef struct AlgaChoice AlgaChoice;

/* Instantiates the chosen algorithm in the context of the choice. */
typedef int (*AlgaChooseFn)(AlgaChoice *choice, void *info, int initArg, AlgaEntry *entry,
                            void *extError, void *surrender);

/* The algorithm an AHChoose* object picks at Init (AlgaChoiceChoose) and its context. */
struct AlgaChoice {
    /* 0x00 */ void *methods;  /* methods of the chosen algorithm */
    /* 0x04 */ void *tag;
    /* 0x08 */ void *params;
    /* 0x0C */ AlgaChooseFn instantiate;
    /* 0x10 */ BsafePoolEntry context;  /* resize context: the state of the algorithm */
}; /* size = 0x1C */

/* Picks the block cipher of an AHFeedbackCipher. */
typedef struct AHChooseBlockCryptor {
    /* 0x00 */ AHObject base;  /* destroy is 0 */
    /* 0x0C */ AlgaChoice choice;
} AHChooseBlockCryptor; /* size = 0x28 */

/* Picks the feedback mode of an AHFeedbackCipher. */
typedef struct AHChooseFeedback {
    /* 0x00 */ AHObject base;  /* destroy is 0 */
    /* 0x0C */ AlgaChoice choice;
    /* 0x28 */ int ivFlag;  /* set by SetNewIVFlag, consumed by Final */
} AHChooseFeedback; /* size = 0x2C */

/* A cipher chosen at Init (base.chosen = &choice after a successful Init). */
typedef struct AHChooseEncrypt {
    /* 0x00 */ BsafeAlgSub base;  /* methods: overwritten by AH_RSAEncryption */
    /* 0x10 */ AlgaChoice choice;
    /* 0x2C */ int initArg;  /* passed by AlgaChoiceChoose to the Init of the chosen alga; 241 in the RSA public encryptor */
} AHChooseEncrypt; /* size = 0x30 */

/* A block cipher in a feedback mode, both chosen at Init. */
typedef struct AHFeedbackCipher {
    /* 0x00 */ BsafeAlgSub base;
    /* 0x10 */ AHChooseFeedback feedback;
    /* 0x3C */ AHChooseBlockCryptor blockCryptor;
    /* 0x64 */ int paddingType;  /* 0 none, 1 pad, 2 stream */
    /* 0x68 */ int feedbackType;  /* 1 cbc ... 7 cfb_pipelined (the tables of bsafe_ai.c) */
    /* 0x6C */ int bufferedLen;  /* bytes buffered by Update */
    /* 0x70 */ void *buffer;  /* IV and leftover input, 2 * blockLen bytes */
    /* 0x74 */ unsigned int blockLen;  /* returned by GetBlockLen */
} AHFeedbackCipher; /* size = 0x78 */

/* The RSA encryption AH object: an AHChooseEncrypt with its own method table, and the buffer that
 * collects the input until Final. */
typedef struct AH_RSAEncryption {
    /* 0x00 */ AHChooseEncrypt base;
    /* 0x30 */ unsigned char *accumBuffer;
    /* 0x34 */ unsigned int accumAllocSize;
    /* 0x38 */ unsigned int accumUsed;
    /* 0x3C */ unsigned int accumCapacity;  /* bytes the buffer may collect */
    /* 0x40 */ AHRsaMethods *rsaMethods;
} AH_RSAEncryption; /* size = 0x44 */

void *AHChooseBlockCryptorConstructor(void *this_, BsafeExtendedError *extError, void *tag, void *params);
void AHChooseBlockCryptorDestructor(AHChooseBlockCryptor *this_);
int AHChooseBlockCryptorGetBlockLen(AHChooseBlockCryptor *this_, unsigned int *outLen);
int AHChooseBlockCryptorEncryptInit(void *this_, void *key, void *chooser, void *surrender);
int AHChooseBlockCryptorDecryptInit(void *this_, void *key, void *chooser, void *surrender);

void *AHChooseFeedbackConstructor(void *this_, BsafeExtendedError *extError, void *tag, void *params);
void AHChooseFeedbackDestructor(AHChooseFeedback *this_);
int AHChooseFeedbackGetBlockLen(AHChooseFeedback *this_, unsigned int *outLen);
int AHChooseFeedbackEncryptInit(void *this_, void *chooser, void *surrender);
int AHChooseFeedbackDecryptInit(void *this_, void *chooser, void *surrender);
/* The feedback mode works on top of the block cipher: Update and Final get the methods and the
 * state of the block cipher. */
int AHChooseFeedbackUpdate(AHChooseFeedback *this_, void *blockMethods, void *blockState,
                            void *output, unsigned int *outLen, void *input, unsigned int inputLen,
                            void *surrender);
int AHChooseFeedbackFinal(AHChooseFeedback *this_, void *blockMethods, void *blockState);
int AHChooseFeedbackSetNewIVFlag(AHChooseFeedback *this_);

void *AHChooseEncryptConstructor2(void *this_, BsafeExtendedError *extError, void *tag, void *params);
void AHChooseEncryptDestructor(AHChooseEncrypt *this_);
int AHChooseEncryptGetBlockLen(AHChooseEncrypt *this_, unsigned int *outLen);
int AHChooseEncryptEncryptInit(AHChooseEncrypt *this_, void *key, void *chooser, void *surrender);
int AHChooseEncryptDecryptInit(AHChooseEncrypt *this_, void *key, void *chooser, void *surrender);
int AHChooseEncryptEncryptUpdate(AHChooseEncrypt *this_, void *out, unsigned int *outLen, unsigned int maxOutLen,
                                  void *in, unsigned int inLen, void *randomAlg, void *surrender);
int AHChooseEncryptDecryptUpdate(AHChooseEncrypt *this_, void *out, unsigned int *outLen, unsigned int maxOutLen,
                                   void *in, unsigned int inLen, void *randomAlg, void *surrender);
int AHChooseEncryptEncryptFinal(AHChooseEncrypt *this_, void *out, unsigned int *outLen, unsigned int maxOutLen,
                                  void *randomAlg, void *surrender);
int AHChooseEncryptDecryptFinal(AHChooseEncrypt *this_, void *out, unsigned int *outLen, unsigned int maxOutLen,
                                   void *randomAlg, void *surrender);

extern void AHEncryptDecryptConstructor(void *this_, BsafeExtendedError *extError);
/* The sixth argument is the list of candidates, closed by 0. */
extern int AlgaChoiceChoose(void *this_, void *methodsSlot, int mode, void *key, int initArg,
                            AlgaEntry **chooser, void *surrender);
extern int ConvertAlgaeError(int status);

void *AHFeedbackCipherConstructor(void *this_, BsafeExtendedError *extError, void *encryptionAIT, void *encryptionInfo,
                                    void *feedbackAIT, void *feedbackInfo, int paddingType, int feedbackType);
void AHFeedbackCipherDestructor(AHFeedbackCipher *this_);
int AHFeedbackCipherGetBlockLen(AHFeedbackCipher *this_, unsigned int *outLen);
int AHFeedbackCipherEncryptInit(AHFeedbackCipher *this_, void *key, void *chooser, void *surrender);
int AHFeedbackCipherDecryptInit(AHFeedbackCipher *this_, void *key, void *chooser, void *surrender);
int AHFeedbackCipherEncryptFinal(AHFeedbackCipher *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                   void *randomAlg, void *surrender);
int AHFeedbackCipherDecryptFinal(AHFeedbackCipher *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                   void *randomAlg, void *surrender);
int AHFeedbackCipherEncryptUpdate(AHFeedbackCipher *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                    void *in, unsigned int inLen, void *randomAlg, void *surrender);
int AHFeedbackCipherDecryptUpdate(AHFeedbackCipher *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                    void *in, unsigned int inLen, void *randomAlg, void *surrender);
int AHFeedbackCipherReInitIV(AHFeedbackCipher *this_, ITEM *curInfo, ITEM *newInfo);

void AH_RSAEncryptionConstructor1(void *this_, BsafeExtendedError *extError, void *tag);
void AH_RSAEncryptionDestructor(AH_RSAEncryption *this_);
int AH_RSAEncryptionGetBlockLen(void *this_, unsigned int *outLen);
int AH_RSAEncryptionEncryptInit(AH_RSAEncryption *this_, void *key, void *chooser, void *surrender);
int AH_RSAEncryptionDecryptInit(AH_RSAEncryption *this_, void *key, void *chooser, void *surrender);
int AH_RSAEncryptionUpdate(AH_RSAEncryption *this_, void *unused1, unsigned int *outLen, void *unused3, void *in, unsigned int inLen);
int AH_RSAEncryptionEncryptFinal(AH_RSAEncryption *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                    void *randomAlg, void *surrender);
int AH_RSAEncryptionDecryptFinal(AH_RSAEncryption *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                    void *randomAlg, void *surrender);
int AH_RSASignatureDecryptFinal(AH_RSAEncryption *this_, void *output, unsigned int *outLen, unsigned int maxOutLen,
                                   void *randomAlg, void *surrender);
void *AH_RSAEncrypPublicConstructor(void *this_, BsafeExtendedError *extError);
void *AH_RSAEncrypSignatureConstructor2(void *this_);


#endif
