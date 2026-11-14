/*
 * The DNAS state shared by the DNAS1 client and the sceDNAS2* functions
 * (_sce_dnas1_common), and its accessors.
 */

#ifndef SCEDNAS1_STATE_H
#define SCEDNAS1_STATE_H

#include <pthread.h>

/* The action block of a request: what the next exchange with the server is for. */
typedef struct {
	unsigned int flags : 8;  /* bit 0: from bit 0 of DNAS2InitConf.flags */
	unsigned int category : 3;
	unsigned int purpose : 3; /* 1 unique ID, 2 install, 4 download */
	unsigned int unk_bit14 : 2;
	unsigned int : 16;
} sce_dnas1_action;

/* Progress of the request in flight (the first three words of DNAS2Status). */
typedef struct {
	/* 0x00 */ int code;
	/* 0x04 */ int sub_code;
	/* 0x08 */ int progress;
} sce_dnas1_status; /* size = 0xC */

/* Transport operations, filled by _sce_dnas2_init_netglue_table. */
typedef struct {
	/* 0x00 */ int (*ctx_new)(void);
	/* 0x04 */ int (*connect)(void);
	/* 0x08 */ int (*write)(unsigned char *buf, int len, int arg);
	/* 0x0C */ int (*read)(unsigned char *buf, int len);
	/* 0x10 */ int (*disconnect)(void);
	/* 0x14 */ int (*cleanup)(void);
	/* 0x18 */ int (*nop)(void);
	/* 0x1C */ int (*set_host)(void *host, int port);
	/* 0x20 */ int (*set_flag)(int v);
} sce_dnas1_netglue; /* size = 0x24 */

typedef struct {
	/* 0x00000 */ char server[512];  /* host name of the DNAS server */
	/* 0x00200 */ int core_started;
	/* 0x00204 */ char proxy_host[512];
	/* 0x00404 */ int proxy_enabled;
	/* 0x00408 */ short proxy_port;
	/* 0x0040A */ char pad_0040A[2];
	/* 0x0040C */ pthread_t thread;  /* the module thread */
	/* 0x00410 */ int in_connect;  /* the transport is waiting for connect() */
	/* 0x00414 */ int abort_sema;  /* signalled by sceDNAS2Abort */
	/* 0x00418 */ int unk_00418;
	/* 0x0041C */ unsigned char pass_phrase[8];
	/* 0x00424 */ unsigned char auth_data[0x10000];
	/* 0x10424 */ unsigned char auth_token[256]; /* derived from auth_data, sent with the request */
	/* 0x10524 */ unsigned char password[16];
	/* 0x10534 */ unsigned char session_id[32];
	/* 0x10554 */ unsigned char challenge[288];
	/* 0x10674 */ unsigned char auth_result; /* one-byte verdicts of the server */
	/* 0x10675 */ unsigned char reg_result;
	/* 0x10676 */ unsigned char inst_result;
	/* 0x10677 */ char pad_10677;
	/* 0x10678 */ union {
		int word;
		sce_dnas1_action bits;
	} action;
	/* 0x1067C */ unsigned char *msg_param; /* parameter of messages 2 and 12 */
	/* 0x10680 */ void *dl_req;  /* request of sceDNAS2AuthDataDownload */
	/* 0x10684 */ void *uid_buf;  /* buffer of sceDNAS2AuthGetUniqueID */
	/* 0x10688 */ unsigned char record_result;
	/* 0x10689 */ unsigned char down_result;
	/* 0x1068A */ unsigned char id_result;
	/* 0x1068B */ char pad_1068B;
	/* 0x1068C */ unsigned char *hdr_buf; /* HTTP header, 1024 bytes */
	/* 0x10690 */ int hdr_len;
	/* 0x10694 */ unsigned char *data_buf; /* HTTP body, 32819 bytes */
	/* 0x10698 */ int data_len;
	/* 0x1069C */ unsigned char *send_buf; /* header and body joined for the transport */
	/* 0x106A0 */ int send_len;
	/* 0x106A4 */ int state;  /* state of the core thread */
	/* 0x106A8 */ int mode;   /* bit 0: reason 96 already seen */
	/* 0x106AC */ int ready_sema;  /* signalled when the core thread is idle or failed */
	/* 0x106B0 */ int wakeup_sema;
	/* 0x106B4 */ int data_sema;
	/* 0x106B8 */ int lock_sema;  /* guards state, status and the counters */
	/* 0x106BC */ int exit_sema;
	/* 0x106C0 */ int io_sema;  /* released around every transport call */
	/* 0x106C4 */ int idle_sema;
	/* 0x106C8 */ int result;  /* exit code of the core thread */
	/* 0x106CC */ int msg_arg;  /* third argument of the download messages */
	/* 0x106D0 */ int idle_waiters;
	/* 0x106D4 */ int data_waiters;
	/* 0x106D8 */ char pad_106D8[4];
	/* 0x106DC */ int shutdown;
	/* 0x106E0 */ int abort;
	/* 0x106E4 */ char pad_106E4[4];
	/* 0x106E8 */ sce_dnas1_status status;
	/* 0x106F4 */ int write_arg;  /* third argument of the transport write */
	/* 0x106F8 */ sce_dnas1_netglue net;
	/* 0x1071C */ int server_kind;  /* > 0: the ts01 server instead of gate1 */
	/* 0x10720 */ int debug_error;  /* error code reported in the request path */
} sce_dnas1_state; /* size = 0x10724 */

extern sce_dnas1_state _sce_dnas1_common;

/* Second argument of set_id, set_id_size, set_downdata, set_downdata_size. */
typedef struct sce_dnas1_io {
	/* 0x00 */ void *unk_00;
	/* 0x04 */ unsigned char *id;  /* ID buffer */
	/* 0x08 */ int *id_size;  /* ID size */
	/* 0x0C */ unsigned char *downdata;  /* downloaded data buffer */
	/* 0x10 */ int *downdata_size;  /* downloaded data size */
} sce_dnas1_io;

int _sce_dnas1_get_action_field(unsigned char *p);
int _sce_dnas1_get_down_IDslot(unsigned short *p);
int _sce_dnas1_get_idreq_category(unsigned int *p);
int _sce_dnas1_get_lastdate(unsigned char *p, unsigned char *blk);
int _sce_dnas1_get_pass_phrase(unsigned char *p);
int _sce_dnas1_get_record_data(unsigned char *p, unsigned char *src);
int _sce_dnas1_get_session_id(unsigned char *p);
int _sce_dnas1_get_version(short *out);
int _sce_dnas1_set_auth_data(unsigned char *p);
int _sce_dnas1_set_auth_result(unsigned char *p);
int _sce_dnas1_set_challenge(unsigned char *p);
int _sce_dnas1_set_down_result(unsigned char *p);
int _sce_dnas1_set_downdata(unsigned char *p, sce_dnas1_io *io);
int _sce_dnas1_set_downdata_size(unsigned char *p, sce_dnas1_io *io);
int _sce_dnas1_set_id(unsigned char *p, sce_dnas1_io *io);
int _sce_dnas1_set_id_result(unsigned char *p);
int _sce_dnas1_set_id_size(unsigned char *p, sce_dnas1_io *io);
int _sce_dnas1_set_inst_result(unsigned char *p);
int _sce_dnas1_set_lastdate(unsigned char *p, unsigned char *blk);
int _sce_dnas1_set_ng_reason(unsigned char *p, int *out);
int _sce_dnas1_set_pass_phrase(unsigned char *p);
int _sce_dnas1_set_record_result(unsigned char *p);
int _sce_dnas1_set_reg_result(unsigned char *p);
int _sce_dnas1_set_session_id(unsigned char *p);

#endif
