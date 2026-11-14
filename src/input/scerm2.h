/*
 * sceRm2*: "Remote 2" API of the PS2 SDK, the IR remote control (scerm2.c).
 */

#ifndef SCERM2_H
#define SCERM2_H

/* IR capabilities returned by sceRm2GetIRFeature (only flags is written). */
typedef struct sceRm2IRFeature {
	/* 0x00 */ unsigned char flags;
	/* 0x01 */ unsigned char pad[15];
} sceRm2IRFeature; /* size = 0x10 */

#ifdef __cplusplus
extern "C" {
#endif

extern int sceRm2Lib_debug;

int sceRm2Init(int mode);
int sceRm2End(void);
int sceRm2PortOpen(void *port);
int sceRm2PortClose(void);
int sceRm2Read(unsigned short *deviceId, unsigned char *index);
int sceRm2GetState(void);
int sceRm2GetModVersion(void);
void sceRm2CodeToStr(unsigned short code, signed char idx, char *buf);
int sceRm2SetReqState(int state);
int sceRm2GetReqState(void);
void sceRm2StateIntToStr(int state, char *buf);
void sceRm2ReqIntToStr(int state, char *buf);
int sceRm2GetIRFeature(sceRm2IRFeature *feature);
int sceRm2IgnoreSIRCS(char flag);

#ifdef __cplusplus
}
#endif

#endif
