/*
 * sceDNAS2*: start of the DNAS2 module thread, sceDNAS2Init, the
 * sceDNAS2Auth* calls, Shutdown, Abort, GetStatus and the proxy settings.
 */

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "dnas_err.h"
#include "dnas1core.h"
#include "dnas2m11_identity.h"
#include "dnas_pwrap.h"
#include "scednas1_state.h"
#include "scednas2.h"

#include <string.h>
#include "dnas_prot.h"
#include "thread/scesema.h"

/* Download request: only the size is read. */
typedef struct {
	unsigned int size;
} Dnas2DlReq;

/* The DNAS state blob shared by the DNAS1 and DNAS2 code; the module-ready
   flag below is private to the sceDNAS2* functions of this file. */
sce_dnas1_state _sce_dnas1_common;
/* Library identification string. */
static char dnas2_libid[16] = "PsIIlibdnas2....";
static int g_dnas2Ready = 0;

/* Starts the module thread with default attributes (priority and stack size
 * are not applied), keeping its id in the common blob. */
static int DNAS2StartThread2(void *(*fn)(void *), int unused, int stackSize, int priority, void *arg)
{
	pthread_attr_t attr;
	int r;

	r = pthread_attr_init(&attr);
	if (r < 0) {
		perror("pthread_attr_init():");
		return r;
	}
	r = pthread_create(&_sce_dnas1_common.thread, &attr, fn, arg);
	if (r < 0) {
		perror("pthread_create:");
		goto destroy;
	}
	r = pthread_detach(_sce_dnas1_common.thread);
	if (r == 0)
		return 0;
	perror("pthread_detach:");
destroy:
	pthread_attr_destroy(&attr);
	return r;
}

static unsigned char dnas2_hdrbuf[1024];
static unsigned char dnas2_databuf[32820];
/* Header and data buffer back to back (1024 + 32820 bytes). */
static unsigned char dnas2_workbuf[1024 + 32820];

static inline void dnas2_bzero(void *buf, unsigned int n)
{
	unsigned char *p = (unsigned char *)buf;
	unsigned int i;

	for (i = 0; i < n; i++)
		p[i] = 0;
}

/*
 * DNAS2InitSub, the body of sceDNAS2Init: validate the initial error code
 * (-1055..0) and the ready flag, clears the common blob, stores pass phrase and
 * auth data, prepares the network context and the netglue table, creates the
 * eight core semaphores, chooses the server, installs the three message buffers,
 * set the state machine to state 0 and starts the core thread, waiting for its
 * result at state offset 0x106C8. On error the thread and the semaphores
 * already created are deleted.
 */
static int DNAS2InitSub(DNAS2InitConf *conf, int priority, int stackSize, int server, int err, int reserved, int unused)
{
	SemaParam sp;
	char *gate;
	char *ts;
	unsigned char *p;
	unsigned int i;
	int rc;
	int r;

	if (err < -1055 || err > 0)
		return -9;
	if (g_dnas2Ready > 0)
		return -7;

	p = (unsigned char *)&_sce_dnas1_common;
	for (i = 0; i < sizeof(_sce_dnas1_common); i++)
		p[i] = 0;
	g_dnas2Ready = 1;

	_sce_dnas1_set_pass_phrase(conf->pass_phrase);
	_sce_dnas1_set_auth_data(conf->auth_data);
	_sce_dnas1_common.unk_00418 = reserved;
	r = __sce_dnas2_symbol11072(_sce_dnas1_common.auth_data);
	if (r < 0)
		return r;
	_sce_dnas1_common.action.word = (_sce_dnas1_common.action.word & ~1) | ((unsigned short)conf->flags & 1);
	_sce_dnas2_init_netglue_table((void **)&_sce_dnas1_common.net);

	dnas2_bzero(&sp, sizeof(sp));
	sp.initCount = 0;
	sp.maxCount = 1;
	rc = sce_sema_createsema(&sp);
	_sce_dnas1_common.ready_sema = rc;
	if (rc < 0)
		goto rc_fail;
	dnas2_bzero(&sp, sizeof(sp));
	sp.initCount = 0;
	sp.maxCount = 1;
	rc = sce_sema_createsema(&sp);
	_sce_dnas1_common.wakeup_sema = rc;
	if (rc < 0)
		goto rc_fail;
	dnas2_bzero(&sp, sizeof(sp));
	sp.initCount = 0;
	sp.maxCount = 1;
	rc = sce_sema_createsema(&sp);
	_sce_dnas1_common.data_sema = rc;
	if (rc < 0)
		goto rc_fail;
	dnas2_bzero(&sp, sizeof(sp));
	sp.initCount = 1;
	sp.maxCount = 1;
	rc = sce_sema_createsema(&sp);
	_sce_dnas1_common.lock_sema = rc;
	if (rc < 0)
		goto rc_fail;
	dnas2_bzero(&sp, sizeof(sp));
	sp.initCount = 0;
	sp.maxCount = 1;
	rc = sce_sema_createsema(&sp);
	_sce_dnas1_common.exit_sema = rc;
	if (rc < 0)
		goto rc_fail;
	dnas2_bzero(&sp, sizeof(sp));
	sp.initCount = 0;
	sp.maxCount = 1;
	rc = sce_sema_createsema(&sp);
	_sce_dnas1_common.io_sema = rc;
	if (rc < 0)
		goto rc_fail;
	dnas2_bzero(&sp, sizeof(sp));
	sp.initCount = 0;
	sp.maxCount = 1;
	rc = sce_sema_createsema(&sp);
	_sce_dnas1_common.idle_sema = rc;
	if (rc < 0)
		goto rc_fail;
	dnas2_bzero(&sp, sizeof(sp));
	sp.initCount = 0;
	sp.maxCount = 1;
	rc = sce_sema_createsema(&sp);
	_sce_dnas1_common.abort_sema = rc;
	if (rc < 0)
		goto rc_fail;

	if (server > 0)
		_sce_dnas1_common.server_kind = server;
	if (err != 0)
		_sce_dnas1_common.debug_error = err;
	_sce_dnas1_common.proxy_enabled = 0;

	SCE_DNAS1_PROT_BEGIN();
	gate = SCE_DNAS_STR("gate1.jp.dnas.playstation.org");
	ts = SCE_DNAS_STR("ts01.jp.dnas.playstation.org");
	if (_sce_dnas1_common.server_kind > 0)
		strncpy(_sce_dnas1_common.server, ts, 512);
	else
		strncpy(_sce_dnas1_common.server, gate, 512);
	SCE_DNAS1_PROT_END();

	_sce_dnas1_common.hdr_buf = dnas2_hdrbuf;
	_sce_dnas1_common.data_buf = dnas2_databuf;
	_sce_dnas1_common.send_buf = dnas2_workbuf;
	if (_sce_dnas1_set_state(0, -1, 0) < 0) {
		r = SCE_DNAS_ERR_IO;
		goto fail;
	}
	r = DNAS2StartThread2((void *(*)(void *))_sce_dnas1_core_loop, 0, stackSize, priority, 0);
	if (r < 0)
		goto fail;
	sce_sema_waitsema(_sce_dnas1_common.ready_sema);
	rc = _sce_dnas1_common.result;
	if (rc < 0)
		goto rc_fail;
	return 0;

rc_fail:
	r = rc;
fail:
	if (_sce_dnas1_common.thread != 0)
		pthread_cancel(_sce_dnas1_common.thread);
	if (_sce_dnas1_common.ready_sema > 0)
		sce_sema_deletesema(_sce_dnas1_common.ready_sema);
	if (_sce_dnas1_common.wakeup_sema > 0)
		sce_sema_deletesema(_sce_dnas1_common.wakeup_sema);
	if (_sce_dnas1_common.data_sema > 0)
		sce_sema_deletesema(_sce_dnas1_common.data_sema);
	if (_sce_dnas1_common.lock_sema > 0)
		sce_sema_deletesema(_sce_dnas1_common.lock_sema);
	if (_sce_dnas1_common.exit_sema > 0)
		sce_sema_deletesema(_sce_dnas1_common.exit_sema);
	if (_sce_dnas1_common.io_sema > 0)
		sce_sema_deletesema(_sce_dnas1_common.io_sema);
	if (_sce_dnas1_common.idle_sema > 0)
		sce_sema_deletesema(_sce_dnas1_common.idle_sema);
	if (_sce_dnas1_common.abort_sema > 0)
		sce_sema_deletesema(_sce_dnas1_common.abort_sema);
	return r;
}

/* Forwards to DNAS2InitSub, with zeros as 6th and 7th arguments. */
int sceDNAS2Init(DNAS2InitConf *conf, int priority, int stackSize, int server, int err)
{
	return DNAS2InitSub(conf, priority, stackSize, server, err, 0, 0);
}

int sceDNAS2AuthInstall(void)
{
	sce_dnas1_state *ctx = &_sce_dnas1_common;
	sce_dnas1_status *req = &_sce_dnas1_common.status;
	int r;

	if (g_dnas2Ready == 0)
		return -8;

	r = __sce_dnas2_symbol11071();
	if (r > 0) {
		r = __sce_dnas2_symbol11066();
		if (r < 0)
			return r;
		if (r == 0)
			return -404;
	} else if (r < 0) {
		return r;
	}

	if (_sce_dnas1_core_in_sleep() == 0)
		return -3;

	ctx->action.bits.purpose = 2;
	ctx->action.bits.category = 0;

	r = __sce_dnas2_symbol11071();
	if (r > 0) {
		if (_sce_dnas1_common.mode & 1)
			ctx->action.bits.unk_bit14 = 0;
		else
			ctx->action.bits.unk_bit14 = 2;
	} else {
		if (r != 0)
			return r;
		ctx->action.bits.unk_bit14 = 0;
	}

	ctx->msg_arg = 1;
	sce_sema_waitsema(_sce_dnas1_common.lock_sema);
	req->code = 3;
	req->progress = 1;
	sce_sema_signalsema(_sce_dnas1_common.lock_sema);
	_sce_dnas1_core_wakeup();
	return 0;
}

int sceDNAS2AuthDataDownload(Dnas2DlReq *p)
{
	sce_dnas1_state *ctx = &_sce_dnas1_common;
	sce_dnas1_status *req = &_sce_dnas1_common.status;
	int r;

	if (g_dnas2Ready == 0)
		return -8;

	r = __sce_dnas2_symbol11071();
	if (r > 0) {
		r = __sce_dnas2_symbol11066();
		if (r < 0)
			return r;
		if (r == 0)
			return -404;
	} else if (r < 0) {
		return r;
	}

	if (_sce_dnas1_core_in_sleep() == 0)
		return -3;

	if (p->size > 0xffff)
		return -4;

	ctx->dl_req = p;
	ctx->action.bits.purpose = 4;
	ctx->action.bits.category = 2;

	r = __sce_dnas2_symbol11071();
	if (r > 0) {
		if (_sce_dnas1_common.mode & 1)
			ctx->action.bits.unk_bit14 = 0;
		else
			ctx->action.bits.unk_bit14 = 2;
	} else {
		if (r != 0)
			return r;
		ctx->action.bits.unk_bit14 = 0;
	}

	ctx->msg_arg = 1;
	sce_sema_waitsema(_sce_dnas1_common.lock_sema);
	req->code = 4;
	req->progress = 1;
	sce_sema_signalsema(_sce_dnas1_common.lock_sema);
	_sce_dnas1_core_wakeup();
	_sce_dnas1_incr_data_wait();
	return 0;
}

int sceDNAS2AuthDataDownload2(Dnas2DlReq *p, int flag)
{
	sce_dnas1_state *ctx = &_sce_dnas1_common;
	sce_dnas1_status *req = &_sce_dnas1_common.status;
	int r;

	if (g_dnas2Ready == 0)
		return -8;

	r = __sce_dnas2_symbol11071();
	if (r > 0) {
		r = __sce_dnas2_symbol11066();
		if (r < 0)
			return r;
		if (r == 0)
			return -404;
	} else if (r < 0) {
		return r;
	}

	if (_sce_dnas1_core_in_sleep() > 0) {
		if (p->size > 0xffff)
			return -4;

		ctx->dl_req = p;
		ctx->action.bits.purpose = 4;
		ctx->action.bits.category = 2;

		r = __sce_dnas2_symbol11071();
		if (r > 0) {
			if (_sce_dnas1_common.mode & 1)
				ctx->action.bits.unk_bit14 = 0;
			else
				ctx->action.bits.unk_bit14 = 2;
		} else {
			if (r != 0)
				return r;
			ctx->action.bits.unk_bit14 = 0;
		}

		ctx->msg_arg = flag;
		sce_sema_waitsema(_sce_dnas1_common.lock_sema);
		req->code = 4;
		req->progress = 1;
		sce_sema_signalsema(_sce_dnas1_common.lock_sema);
		_sce_dnas1_core_wakeup();
		_sce_dnas1_incr_data_wait();
	} else if (_sce_dnas1_core_in_data() > 0) {
		if (p->size > 0xffff)
			return -4;

		ctx->dl_req = p;
		ctx->action.bits.category = 2;
		ctx->msg_arg = flag;
		sce_sema_waitsema(_sce_dnas1_common.lock_sema);
		req->code = 4;
		req->progress = 3;
		sce_sema_signalsema(_sce_dnas1_common.lock_sema);
		_sce_dnas1_incr_data_wait();
	} else {
		return -3;
	}
	return 0;
}

int sceDNAS2AuthGetUniqueID(void *p)
{
	sce_dnas1_state *ctx = &_sce_dnas1_common;
	sce_dnas1_status *req = &_sce_dnas1_common.status;
	int r;

	if (g_dnas2Ready == 0)
		return -8;

	r = __sce_dnas2_symbol11071();
	if (r > 0) {
		r = __sce_dnas2_symbol11066();
		if (r < 0)
			return r;
		if (r == 0)
			return -404;
	} else if (r < 0) {
		return r;
	}

	if (_sce_dnas1_core_in_sleep() == 0)
		return -3;

	ctx->uid_buf = p;
	ctx->action.bits.purpose = 1;
	ctx->action.bits.category = 0;

	r = __sce_dnas2_symbol11071();
	if (r > 0) {
		if (_sce_dnas1_common.mode & 1)
			ctx->action.bits.unk_bit14 = 0;
		else
			ctx->action.bits.unk_bit14 = 2;
	} else {
		if (r != 0)
			return r;
		ctx->action.bits.unk_bit14 = 0;
	}

	ctx->msg_arg = 1;
	sce_sema_waitsema(_sce_dnas1_common.lock_sema);
	req->code = 9;
	req->progress = 1;
	sce_sema_signalsema(_sce_dnas1_common.lock_sema);
	_sce_dnas1_core_wakeup();
	_sce_dnas1_incr_data_wait();
	return 0;
}

/*
 * Closes the DNAS2 session: wakes the core, cancels its thread, deletes the eight
 * semaphores, then overwrites the whole context with random bytes before
 * clearing the ready flag.
 */
int sceDNAS2Shutdown(void)
{
	unsigned char *p;
	unsigned int i;
	int r;

	if (g_dnas2Ready == 0)
		return 0;

	if (_sce_dnas1_common.shutdown > 0)
		return 0;

	sce_sema_waitsema(_sce_dnas1_common.idle_sema);
	sce_sema_signalsema(_sce_dnas1_common.idle_sema);

	if (_sce_dnas1_core_in_sleep() == 0)
		return -3;

	_sce_dnas1_common.shutdown = 1;

	if (_sce_dnas1_core_in_sleep() > 0)
		_sce_dnas1_core_wakeup();
	else if (_sce_dnas1_core_in_data() > 0)
		_sce_dnas1_incr_data_wait();

	sce_sema_waitsema(_sce_dnas1_common.exit_sema);
	if (_sce_dnas1_common.thread != 0)
		pthread_cancel(_sce_dnas1_common.thread);
	__sce_dnas2_symbol11080();

	r = sce_sema_deletesema(_sce_dnas1_common.ready_sema);
	if (r != _sce_dnas1_common.ready_sema)
		return r;
	r = sce_sema_deletesema(_sce_dnas1_common.wakeup_sema);
	if (r != _sce_dnas1_common.wakeup_sema)
		return r;
	r = sce_sema_deletesema(_sce_dnas1_common.data_sema);
	if (r != _sce_dnas1_common.data_sema)
		return r;
	r = sce_sema_deletesema(_sce_dnas1_common.lock_sema);
	if (r != _sce_dnas1_common.lock_sema)
		return r;
	r = sce_sema_deletesema(_sce_dnas1_common.exit_sema);
	if (r != _sce_dnas1_common.exit_sema)
		return r;
	r = sce_sema_deletesema(_sce_dnas1_common.io_sema);
	if (r != _sce_dnas1_common.io_sema)
		return r;
	r = sce_sema_deletesema(_sce_dnas1_common.idle_sema);
	if (r != _sce_dnas1_common.idle_sema)
		return r;
	r = sce_sema_deletesema(_sce_dnas1_common.abort_sema);
	if (r != _sce_dnas1_common.abort_sema)
		return r;

	p = (unsigned char *)&_sce_dnas1_common;
	for (i = 0; i < sizeof(_sce_dnas1_common); i++)
		p[i] = (unsigned char)(rand() / 2147483648.0 * 256.0);

	g_dnas2Ready = 0;
	return 0;
}

/*
 * If the core is sleeping or in data transfer, checks that both semaphores
 * still have waiters and raises abort; otherwise raises abort and wakes
 * the waiter (signalsema) if pollsema succeeds, or sends signal 17 to the
 * core thread.
 */
int sceDNAS2Abort(void)
{
	SemaParam st;
	int r;

	if (g_dnas2Ready == 0)
		return -8;

	if (_sce_dnas1_core_in_sleep() <= 0 && _sce_dnas1_core_in_data() <= 0)
		goto notSleeping;

	r = ReferSemaStatus(_sce_dnas1_common.wakeup_sema, &st);
	if (r < 0)
		return r;
	if (st.currentCount == 0)
		return -6;
	r = ReferSemaStatus(_sce_dnas1_common.data_sema, &st);
	if (r < 0)
		return r;
	if (st.currentCount == 0)
		return -6;
	sce_sema_waitsema(_sce_dnas1_common.lock_sema);
	_sce_dnas1_common.abort = 1;
	sce_sema_signalsema(_sce_dnas1_common.lock_sema);
	return 0;

notSleeping:
	sce_sema_waitsema(_sce_dnas1_common.lock_sema);
	_sce_dnas1_common.abort = 1;
	sce_sema_signalsema(_sce_dnas1_common.lock_sema);
	if (sce_sema_pollsema(_sce_dnas1_common.io_sema) < 0)
		return 0;
	if (_sce_dnas1_common.in_connect <= 0) {
		pthread_kill(_sce_dnas1_common.thread, SIGUSR2);
		return 0;
	}
	sce_sema_signalsema(_sce_dnas1_common.abort_sema);
	return 0;
}

/* Copies the status of the request in flight to *out, leaving out->reserved as it was. */
int sceDNAS2GetStatus(DNAS2Status *out)
{
	unsigned char *dst;
	const unsigned char *src;
	int saved;
	unsigned int i;

	dst = (unsigned char *)out;
	saved = out->reserved;

	if (!g_dnas2Ready)
		return -8;

	src = (unsigned char *)&_sce_dnas1_common.status;
	for (i = 0; i < 16; i++)
		dst[i] = src[i];

	out->reserved = saved;

	return 0;
}

/* Copy the proxy host name into the common block (512 bytes at +516). */
#define DNAS2_PROXY_SETHOST(h) \
    do { strncpy(_sce_dnas1_common.proxy_host, (h), 512); } while (0)

int sceDNAS2SetProxy(int enabled, char *hostIn, unsigned short port)
{
	if (!g_dnas2Ready)
		return -8;

	if (enabled == 1) {
		DNAS2_PROXY_SETHOST(hostIn);
		_sce_dnas1_common.proxy_port = port;

		if (_sce_dnas1_common.proxy_host[511] != 0)
			return -5;

		_sce_dnas1_common.proxy_enabled = enabled;
	} else {
		_sce_dnas1_common.proxy_enabled = 0;
	}

	return 0;
}

int sceDNAS2GetProxy(int *enabledOut, char *hostOut, int maxLen, short *portOut)
{
	int val;

	if (!g_dnas2Ready)
		return -8;

	val = _sce_dnas1_common.proxy_enabled;
	if (val != 0) {
		*enabledOut = val;
		strncpy(hostOut, _sce_dnas1_common.proxy_host, maxLen);
		*portOut = _sce_dnas1_common.proxy_port;
	} else {
		*enabledOut = 0;
	}

	return 0;
}
