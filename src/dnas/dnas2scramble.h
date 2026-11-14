/*
 * DNAS2 code transforms run by the table dispatchers. Each one transforms
 * every word in [start, end) except those whose offset from base appears in the exclusion list
 * excl[0..nexcl).
 */

#ifndef DNAS2SCRAMBLE_H
#define DNAS2SCRAMBLE_H

/* Entry of the handler tables of the scramble and descramble dispatchers. */
typedef int (*DnasHandler)(void *, void *, void *, void *, void *);

/* Module 11 */
int __sce_dnas2_symbol11007(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11008(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11009(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11010(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11011(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11013(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11014(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11015(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11033(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11034(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11035(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11036(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11037(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11038(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11039(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol11040(int start, int end, char *base, int nexcl, const int *excl);

/* Module 12 */
int __sce_dnas2_symbol12007(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12008(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12009(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12010(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12011(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12013(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12014(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12015(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12033(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12034(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12035(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12036(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12037(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12038(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12039(int start, int end, char *base, int nexcl, const int *excl);
int __sce_dnas2_symbol12040(int start, int end, char *base, int nexcl, const int *excl);

#endif
