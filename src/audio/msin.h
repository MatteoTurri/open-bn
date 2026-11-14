/*
 * sceMSIn_*: MIDI input module of the CSL. The records are csl.h/cslmidi.h ones;
 * no CSL header is in docker/kit: names from the SDK, layouts from the uses.
 */
#ifndef MSIN_H
#define MSIN_H

/* One buffer of a group, and its semaphore. */
typedef struct {
	/* 0x00 */ int sema;
	/* 0x04 */ void *buff;
} sceCslBuffCtx; /* size = 0x8 */

typedef struct {
	/* 0x00 */ int buffNum;
	/* 0x04 */ sceCslBuffCtx *buffCtx;
} sceCslBuffGrp; /* size = 0x8 */

/* Module context: buffGrp[0] = input groups, buffGrp[1] = output groups. */
typedef struct {
	/* 0x00 */ int buffGrpNum;
	/* 0x04 */ sceCslBuffGrp *buffGrp;
	/* 0x08 */ void *conf;
	/* 0x0C */ void *callBack;
	/* 0x10 */ char **extmod;
} sceCslCtx; /* size = 0x14 */

/* MIDI stream buffer: `buffsize` = total size in bytes including this
   header, `validsize` = bytes of messages that follow it. */
typedef struct {
	/* 0x00 */ unsigned int buffsize;
	/* 0x04 */ unsigned int validsize;
	/* 0x08 */ unsigned char data[0];
} sceCslMidiStream; /* size = 0x8 */

int sceMSIn_Init(sceCslCtx *ctx);
int sceMSIn_ATick(void *ctx);
int sceMSIn_Load(void *ctx);
int sceMSIn_PutMsg(sceCslCtx *ctx, int port, unsigned int msg);
int sceMSIn_PutExcMsg(sceCslCtx *ctx, int port, unsigned char *buf, unsigned int maxlen);
int sceMSIn_PutHsMsg(sceCslCtx *ctx, int port, unsigned char *buf);

#endif
