/*
 * GS device, registered double buffer and callback queues of the fw framework (fw_gsdev.c).
 */

#ifndef FW_GSDEV_H
#define FW_GSDEV_H

/* fwGsInitAll(mode): the video mode in the low byte, FW_GS_RESET_GIF or'ed in. */
#define FW_GS_NTSC_FRAME 0
#define FW_GS_VESA_1024x768 1
#define FW_GS_NTSC_FIELD 2
#define FW_GS_DTV_720P 3
#define FW_GS_RESET_GIF 0x8000  /* reset only the GIF, not the whole GS */

#ifdef __cplusplus
extern "C" {
#endif

int fwGsClearDrawBuf(int handle);
int fwGsUpdateStatus(void);
int fwGsCheckUpdateStatus(void);
int fwGsRegistDBuf(int disp, int draw, int flag);
int fwGsCheckRegDBuf(int *disp, int *draw);
int fwGsRegistCrtBlendBuf(int disp, int spare);
int fwGsCheckCrtBlendBuf(int *disp, int *spare);
int fwGsCountFrameOdev(void);
int fwGsFlipRegDispBuf(int disp, int draw, int flag);
int fwGsFlipCrtBlendBuf(int disp, int spare, int draw);
int fwGsFlipRegDBuf(int unused, int notReady);
int fwGsFlipRegDBuf_cb(void);
int fwPkSyncV(void);
int fwVcInitEnable(int enable);
int fwGsStartDisplay(int disp, int draw, int flag);
int fwGsInitDevice(int mode, int inter, int scrmode, int ffmode);
int fwGsInitAll(int mode);
int fwGsReleaseDevice(void);
int fwGsReleaseAll(void);
int fwGsStoreImage(const char *file);
int _fwGsStoreImage_cb(void);
void fwResetCallBack_PreGsSync(void);
void fwResetCallBack_PostGsSync(void);
void fwResetCallBack_PreDmaKick(void);
void fwInitCallBack_All(void);
int fwSetCallBack_PreGsSync(void (*cb)(void));
int fwSetCallBack_PostGsSync(void (*cb)(void));
int fwSetCallBack_PreDmaKick(void (*cb)(void));
void fwExecCallBack_PreGsSync(void);
void fwExecCallBack_PostGsSync(void);
void fwExecCallBack_PreDmaKick(void);

#ifdef __cplusplus
}
#endif

#endif
