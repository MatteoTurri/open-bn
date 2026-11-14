/*
 * sceDNAS2*: start of the DNAS2 module thread, sceDNAS2Init, the sceDNAS2Auth* calls,
 * Shutdown, Abort, GetStatus and the proxy settings.
 */

#ifndef SCEDNAS2_H
#define SCEDNAS2_H

/* First argument of sceDNAS2Init: the pass phrase and the authentication data. */
typedef struct {
	/* 0x00 */ int flags; /* bit 0: copied to bit 0 of state offset 0x10678 */
	/* 0x04 */ char *pass_phrase;
	/* 0x08 */ int pass_phrase_len;
	/* 0x0C */ void *auth_data;
	/* 0x10 */ int auth_data_len;
} DNAS2InitConf; /* size = 0x14 */

/* State filled by sceDNAS2GetStatus. */
typedef struct {
	/* 0x00 */ int code;
	/* 0x04 */ int sub_code;
	/* 0x08 */ int progress;
	/* 0x0C */ int reserved; /* left as the caller set it */
} DNAS2Status; /* size = 0x10 */

#ifdef __cplusplus
extern "C" {
#endif

int sceDNAS2Abort(void);
int sceDNAS2AuthInstall(void);
int sceDNAS2GetStatus(DNAS2Status *out);
int sceDNAS2Init(DNAS2InitConf *conf, int priority, int stackSize, int server, int err);
int sceDNAS2Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
