/*
 * Getters of the current GS state and the buffer chain setup of the fw framework (fw_gsenv.c).
 */

#ifndef FW_GSENV_H
#define FW_GSENV_H

#ifdef __cplusplus
extern "C" {
#endif

int fwGsGetZValue(void);
int fwGsGetFieldFrame(void);
void fwGetScreenSize(int *w, int *h);
int fwGsGetOddEven(void);
int fwGsGetOddEven_SingleBuffer(void);
void fwGetXYOffset(int *outX, int *outY, int *outOddEven);
void fwGetXYOffset_SingleBuffer(int *outX, int *outY, int *outOddEven);
void fwGetFBPandZBP(int *fbp, int *unk, int *zbp);
int fwGsGetFrameCounter(void);
int fwGsBufInit(int mode, int *pDraw, int *pDisp, int *pZ, int *pNone, int *pTexLarge, int *pTexSmall);

#ifdef __cplusplus
}
#endif

#endif
