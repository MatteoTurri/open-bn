/*
 * BASE64 encoder and line decoder of the HTTP library (scebase64.c).
 */

#ifndef SCEBASE64_H
#define SCEBASE64_H

int sceBASE64Encoder(unsigned char *src, char *dst, int len);
int sceBASE64LineDecoder(const char *src, char *dst, int len);

#endif
