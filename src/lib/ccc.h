/*
 * sceCcc*: character encoding conversion (SJIS, JIS, UCS2/UTF-16, UTF-8), ccc.c.
 */

#ifndef CCC_H
#define CCC_H

#ifdef __cplusplus
extern "C" {
#endif

int sceCccDecodeSJIS(const unsigned char **s);
int sceCccDecodeSJIS_forceErrCode(const unsigned char **s, unsigned short errCode);
void sceCccEncodeSJIS(unsigned char **s, unsigned int c);
int sceCccUTF8toUTF16(unsigned short *dst, int n, const unsigned char *src);
int sceCccUTF8toSJIS(unsigned char *dst, int n, const unsigned char *src);
int sceCccSJIStoUTF8(unsigned char *dst, int n, const unsigned char *src);
int sceCccSJIStoUTF16(unsigned short *dst, int n, const unsigned char *src);
int sceCccUTF16toSJIS(unsigned char *dst, int n, const unsigned short *src);
int sceCccUTF16toUTF8(unsigned char *dst, int n, const unsigned short *src);
void sceCccSetTable(void *jisToUcs, void *ucsToJis);
int sceCccStrlenUTF16(const unsigned short *s);
int sceCccStrlenUTF8(const unsigned char *s);
int sceCccStrlenSJIS(const unsigned char *s);
int sceCccIsValidUCS4(int c);
int sceCccIsValidUCS2(unsigned int c);
int sceCccIsValidJIS(unsigned short code);
int sceCccIsValidUTF8(int c);
int sceCccIsValidUTF16(unsigned int c);
int sceCccIsValidSJIS(unsigned short code);
int sceCccDecodeUTF16(const unsigned short **s);
int sceCccDecodeUTF8(const unsigned char **s);
void sceCccEncodeUTF16(unsigned short **s, unsigned int ucs);
void sceCccEncodeUTF8(unsigned char **s, unsigned int ucs);
unsigned int sceCccUCStoJIS(unsigned int ucs, unsigned short def);
unsigned int sceCccJIStoUCS(unsigned int code, unsigned int def);
unsigned short sceCccSetErrorCharSJIS(unsigned short c);
unsigned int sceCccSetErrorCharUTF8(unsigned int c);
unsigned int sceCccSetErrorCharUTF16(unsigned int c);

#ifdef __cplusplus
}
#endif

#endif
