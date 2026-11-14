/*
 * Text strings of the IME (tfont.c): converts EUC-JP and UCS-2 text to UTF-8 and builds, draws
 * and measures it through the host text callbacks of TextIM_arg. `size` is the character height
 * the host font is asked for.
 */
#ifndef TFONT_H
#define TFONT_H

struct PLTM_String;

int tfont_init(void);
void tfont_shutdown(void);
void tfont_newstr_euc(const char *s, int len, struct PLTM_String *str, int size);
void tfont_deletestr(struct PLTM_String *str);
int tfont_gettextrect(struct PLTM_String *str);
void tfont_drawstr(int x, int y, struct PLTM_String *str, void *attr);
void tfont_newstr_ucs2(const unsigned short *s, int nbytes, struct PLTM_String *str, int size);
void tfont_newstr_ucs2_2(const unsigned short *s, int nbytes, struct PLTM_String *str, int size);

#endif
