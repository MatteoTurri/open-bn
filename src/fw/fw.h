/*
 * Declarations shared by the files of the fw graphics framework: the current
 * GS state, the two devices opened by fwGsInitDevice and the functions of fw.c.
 */

#ifndef FW_H
#define FW_H

#ifdef __cplusplus
extern "C" {
#endif

/* gs_env.ffMode: what one buffer holds in interlaced modes. */
#define FW_FF_FIELD 0  /* one field */
#define FW_FF_FRAME 1  /* a whole frame, both fields */

/* Current GS state. */
typedef struct {
	/* 0x00 */ int ffMode;     /* FW_FF_* */
	/* 0x04 */ int interlace;
	/* 0x08 */ int videoMode;  /* 2 NTSC, 3 PAL, 26-75 VESA, >= 76 DTV */
	/* 0x0C */ int visPsm;     /* of the visible buffer */
	/* 0x10 */ int visZpsm;
	/* 0x14 */ int visZtest;
	/* 0x18 */ int visWidth;
	/* 0x1C */ int visHeight;
	/* 0x20 */ int originX;    /* drawing origin (2048, 2048) */
	/* 0x24 */ int originY;
	/* 0x28 */ int oddEven;    /* current field, 1 = odd */
	/* 0x2C */ int multiBuffer; /* 1 after a multi-buffer field query, 0 after a single-buffer one */
	/* 0x30 */ int fbp;
	/* 0x34 */ int unk_34;
	/* 0x38 */ int zbp;
	/* 0x3C */ int pad_3C[2];
	/* 0x44 */ int unk_44;
} fwGsEnvState;  /* size = 0x48 */

extern int frame;
extern int odev;
extern fwGsEnvState gs_env;

/* The two devices opened by fwGsInitDevice, defined in fw_gsdev.c: the GS file
 * descriptor and VPU1 (libps2dev). fw_pk.c uses the VPU1 for DMA, gm_packet.cpp
 * copies it into its packet. */
extern int g_fd_gs;
extern struct ps2_vpu_struct *g_vpu1;

/* fw.c: the table of registered GS buffers and the environment stacks. */
typedef struct fwGsDrawPacket fwGsDrawPacket;
typedef struct fwGsClearPacket fwGsClearPacket;
typedef struct fwGsTexPacket fwGsTexPacket;
typedef struct fwGsTransPacket fwGsTransPacket;
typedef struct sceGsDispEnv sceGsDispEnv;
int fwGsInitBufEnv(void);
int fwGsReleaseBufEnv(void);
void fwGsPutDispEnv(int handle);
void fwGsPutCrtBlendEnv(int handle);
void fwGsPutDispEnvCB(int handle);
int _fwGsStoreImage(int handle, int buf);
void fwGsSetHalfOffset(int handle, int unused1, int unused2, int unused3);
int fwGsSetKickBaseDrawBuf(int handle);
int fwGsSetKickBaseDispBuf(int handle);
int fwGsGetKickBaseDrawBuf(void);
int fwGsGetKickBaseDispBuf(void);
int fwGsGetValueFRAME(void *out);
int fwGsGetValueZBUF(void *out);
int _getBufSize(int psm, short w, short h);
void fwGsSetTransData(int handle, int value);
void fwGsSetTransEnv(fwGsTransPacket *env, short tbp, short bw, short psm, short x, short y, short w, short h);
void fwGsSetCustomDrawEnv(int handle, int fbp, short psm, short w, short h, short ztest, short zpsm);
void fwGsSetDrawEnv(fwGsDrawPacket *env, int fbp, short psm, short w, short h, short ztest, short zpsm);
void fwGsSetCustomClearEnv(int handle, short unused, short w, short h, short ztest, short clear);
void fwGsSetClearEnv(fwGsClearPacket *env, int unused, short w, short h, short ztest, short clear);
void fwGsSetCustomTexEnv(int handle, int tbp, short psm, short w, short h, short wrap);
void fwGsSetTexEnv(fwGsTexPacket *env, int tbp, short psm, short w, short h, short wrap);
void fwGsSetCustomDispEnv(int handle, int fbp, short psm, int w, int h, int dx, int dy, int magh, int magv);
void fwGsSetDispEnv(sceGsDispEnv *env, int fbp, short psm, short w, short h);
int fwGsGetBufWidth(int bufId);
int fwGsGetBufHeight(int bufId);
int fwGsGetBufAddr(int bufId);
int fwGsSetBufEnv(int addr, short psm, short w, short h, short ztest, short zpsm, short clear);
void fwGsDelBufEnv(int handle);
void fwGsSetVisibleBufEnv(int handle);
int fwGsSetCrtBlendDisplay(int handle, int dx, int dy, int w, int h, int dw, int dh);
void fwGsSetCrtBlendBgColor(int handle, int color, int flag);
void fwGsSetCrtBlendAlpha(int handle, int alpha, int flag);
void fwGsSetCrtBlendBufEnv(int handle);
int fwGsPushTransEnv(int handle);
int fwGsPopTransEnv(void);
void fwGsResetTransStack(void);
int fwGsPushDrawEnv(int handle);
int fwGsPopDrawEnv(void);
void fwGsResetDrawStack(void);
int fwGsPushClearBuf(int handle);
int fwGsPopClearBuf(void);
void fwGsResetClearStack(void);
int fwGsPushTexEnv(int handle);
int fwGsPopTexEnv(void);
void fwGsResetTexStack(void);
void fwGsAttachZBuf(int handle, int zhandle);
void fwGsDumpBufEnv(int handle);
int fwGsGetBufSize(int handle);
void fwGsSetDthe(int handle, long long *tmpl);

#ifdef __cplusplus
}
#endif

#endif
