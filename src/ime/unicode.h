/*
 * Charset conversions of the IME (EUC-JP, UCS-2, UTF-8, Shift-JIS), unicode.c. The converted
 * string is allocated with TextIM_arg.alloc.
 */
#ifndef UNICODE_H
#define UNICODE_H

int unicode_strlen(unsigned short *s);
int unicode_init(void);
void unicode_shutdown(void);
void *unicode_euc2uni(const char *in, int inlen);
void *unicode_uni2euc(const unsigned short *in, int inlen);
void *unicode_sjis2uni(char *s, int len);
void *unicode_euc2utf8(const char *in, int inlen);
void *unicode_uni2utf8(const unsigned short *in, int inbytes);

#endif
