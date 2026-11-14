/*
 * Base64 encoder and decoder, one-shot and stateful (bn_base64.cpp).
 */

#ifndef BN_BASE64_H
#define BN_BASE64_H

/* Streaming decoder state: the characters of the pending quartet. */
typedef struct {
	int count;
	char buf[4];
} BN_B64_DECODER;

extern "C" {

void bn_base64_encode(unsigned char *out, const unsigned char *in, int n);
int bn_base64_decode(unsigned char *out, const char *in, int n);
BN_B64_DECODER *bn_base64_decoder_new(void);
void bn_base64_decoder_free(BN_B64_DECODER *d);
int bn_base64_decoder_decode(BN_B64_DECODER *d, const char *in, unsigned char *out);
}

#endif
