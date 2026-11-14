/*
 * URL percent-encoding and decoding (sceurl.c).
 */

#ifndef SCEURL_H
#define SCEURL_H

#ifdef __cplusplus
extern "C" {
#endif

char *sceURLEscape(const char *str);
char *sceURLLibEscapeN(const unsigned char *s, int n);
char *sceURLUnescape(const char *escaped);

#ifdef __cplusplus
}
#endif

#endif
