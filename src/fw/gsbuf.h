/*
 * GS frame buffer setup and the defocus and preview window effects (gsbuf.c).
 */

#ifndef GSBUF_H
#define GSBUF_H

/* GsBufInit(mode): the frame buffer layouts. */
#define GSBUF_640x224 0
#define GSBUF_1024x768 1  /* XGA mode */
#define GSBUF_640x448 2
#define GSBUF_896x512 5  /* SPECIAL mode */

#ifdef __cplusplus
extern "C" {
#endif

/* Frame buffer handles set by GsBufInit (fwGsSetBufEnv handles). */
extern int fbuf0;
extern int fbuf1;
extern int zbuf0;
extern int zbuf1;

void GsBufInit(int mode);
void DefocusInit(void);
void DefocusExec(int target, int passes);
void VisualWindowInit(void);
void VisualWindowExec(int target);

#ifdef __cplusplus
}
#endif

#endif
