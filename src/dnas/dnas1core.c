/*
 * DNAS1 core: the client state machine (_sce_dnas1_core_loop), state
 * changes, and checks on the action block.
 */

#include <signal.h>
#include <stdlib.h>

#include "dnas_err.h"
#include "dnas1core.h"
#include "dnas1msg.h"
#include "dnas1recv.h"
#include "scednas1_state.h"
#include "thread/scesema.h"

/* State id: action code in bits 24 and up plus the three one-byte verdicts,
 * masked with 0x7F0003FF. */
static inline int dnas1_mkid(int k)
{
	return ((k << 24) | (_sce_dnas1_common.auth_result << 8) | (_sce_dnas1_common.reg_result << 4)
	           | (_sce_dnas1_common.inst_result << 2))
	    & 0x7F0003FF;
}
#define MKID(k) dnas1_mkid(k)

static void dnas1_sighandler(int sig)
{}

/* Clears the session id, takes idle_sema and puts the
 * state machine back in state 0 (action -1 if `abort`, else the current
 * state). */
static int dnas1_reset_state(int arg, int abort)
{
	unsigned char *p;
	unsigned int i;

	p = _sce_dnas1_common.session_id;
	for (i = 0; i < 32; i++)
		p[i] = 0;
	sce_sema_waitsema(_sce_dnas1_common.idle_sema);
	return _sce_dnas1_set_state(0, abort ? -1 : _sce_dnas1_common.state, arg);
}

static void dnas1_shutdown(void)
{
	dnas1_reset_state(0, 1);
	sce_sema_pollsema(_sce_dnas1_common.data_sema);
	sce_sema_waitsema(_sce_dnas1_common.lock_sema);
	_sce_dnas1_common.abort = 0;
	sce_sema_signalsema(_sce_dnas1_common.lock_sema);
}

/* Validates the current state against the requested transition
 * and, if it is allowed, stores the new state; returns -1 without changing
 * the state otherwise. */
int _sce_dnas1_set_state(int newstate, int action, int arg)
{
	sce_dnas1_status *st;
	int ret = 0;

	st = &_sce_dnas1_common.status;
	sce_sema_waitsema(_sce_dnas1_common.lock_sema);
	switch (newstate) {
	case 0:
		if (action < 0) {
			st->code = 2;
			st->sub_code = 0;
		} else if (action == 10 || action == 11) {
			if (action == 10)
				st->code = 6;
			else if (action == 11)
				st->code = 7;
			st->sub_code = arg;
		} else {
			int k;

			switch (action) {
			case 4:
				k = 1;
				st->code = 5;
				break;
			case 8:
				k = 2;
				st->code = 5;
				break;
			case 7:
				k = 3;
				st->code = 5;
				break;
			case 13:
				k = 4;
				st->code = 5;
				break;
			default:
				ret = -1;
				goto out;
			}
			st->sub_code = MKID(k);
		}
		st->progress = 0;
		break;
	case 1:
		if (_sce_dnas1_common.state != 0) {
			ret = -1;
			goto out;
		}
		break;
	case 2:
		if (_sce_dnas1_common.state != 1) {
			ret = -1;
			goto out;
		}
		break;
	case 3:
		if (_sce_dnas1_common.state != 2) {
			ret = -1;
			goto out;
		}
		st->progress = 2;
		break;
	case 4:
		if (_sce_dnas1_common.state != 3) {
			ret = -1;
			goto out;
		}
		break;
	case 5:
		switch (_sce_dnas1_common.state) {
		case 4:
			st->progress = 3;
			break;
		case 7:
			st->code = 8;
			st->progress = 7;
			st->sub_code = MKID(3);
			break;
		default:
			ret = -1;
			goto out;
		}
		break;
	case 7:
		if (_sce_dnas1_common.state != 5) {
			ret = -1;
			goto out;
		}
		st->progress = 5;
		break;
	case 12:
		if (_sce_dnas1_common.state != 4) {
			ret = -1;
			goto out;
		}
		st->progress = 3;
		break;
	case 13:
		if (_sce_dnas1_common.state != 12) {
			ret = -1;
			goto out;
		}
		st->progress = 8;
		break;
	case 6:
		if (_sce_dnas1_common.state != 4) {
			ret = -1;
			goto out;
		}
		st->progress = 3;
		break;
	case 8:
		if (_sce_dnas1_common.state != 6) {
			ret = -1;
			goto out;
		}
		st->progress = 4;
		break;
	}
	_sce_dnas1_common.state = newstate;
out:
	sce_sema_signalsema(_sce_dnas1_common.lock_sema);
	return ret;
}

/* 1 if the core thread is waiting in state 0 (idle) for a wakeup. */
int _sce_dnas1_core_in_sleep(void)
{
	return _sce_dnas1_common.idle_waiters > 0;
}

/* 1 if the core thread is waiting for the next data step. */
int _sce_dnas1_core_in_data(void)
{
	return _sce_dnas1_common.data_waiters > 0;
}

/* Wakes the idle core thread. */
int _sce_dnas1_core_wakeup(void)
{
	sce_sema_signalsema(_sce_dnas1_common.wakeup_sema);
	return 0;
}

/* Lets the core thread waiting for data go on. */
int _sce_dnas1_incr_data_wait(void)
{
	sce_sema_signalsema(_sce_dnas1_common.data_sema);
	return 0;
}

/* Compares the "purpose" field (bits 11..13 of the first halfword of the
 * action block) with `purpose`, then overwrites the first two bytes of the
 * local copy with random bytes. */
int _sce_dnas1_check_action_purpose(int purpose)
{
	unsigned char buf[16];
	unsigned char *p;
	unsigned int i;
	int ret;
	int t;

	p = buf;
	_sce_dnas1_get_action_field(p);

	t = ((*(unsigned short *)p >> 11) & 7) ^ purpose;
	ret = t == 0;

	for (i = 0; i < 2; i++)
		p[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);

	return ret;
}

/* Same as _sce_dnas1_check_action_purpose on the "category" field (bits 8..10). */
static int dnas1_check_action_category(int category)
{
	unsigned char buf[16];
	unsigned char *p;
	unsigned int i;
	int ret;
	int t;

	p = buf;
	_sce_dnas1_get_action_field(p);

	t = ((*(unsigned short *)p >> 8) & 7) ^ category;
	ret = t == 0;

	for (i = 0; i < 2; i++)
		p[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);

	return ret;
}

/* 1 if a shutdown was requested. */
static int dnas1_shutdown_called(void)
{
	return _sce_dnas1_common.shutdown > 0;
}

/* 1 if an abort was requested. */
int _sce_dnas1_abort_called(void)
{
	return _sce_dnas1_common.abort > 0;
}

/* Body of the core thread: installs the signal handler, runs the three setup
 * hooks, then drives the state machine (send and receive of each message
 * type) until it finishes, aborts or fails; the exit code goes to result. */
int _sce_dnas1_core_loop(void)
{
	struct sigaction act;
	struct sigaction oldact;
	int ret;
	int rc;
	unsigned char *p;
	unsigned int i;

	ret = 0;
	p = (unsigned char *)&act;
	for (i = 0; i < sizeof(act); i++)
		p[i] = 0;
	act.sa_handler = dnas1_sighandler;
	sigaction(SIGUSR2, &act, &oldact);

	rc = _sce_dnas1_common.net.ctx_new();
	if (rc < 0) {
		_sce_dnas1_common.result = rc;
		sce_sema_signalsema(_sce_dnas1_common.ready_sema);
		return rc;
	}
	rc = _sce_dnas1_common.net.set_host(_sce_dnas1_common.server, 443);
	if (rc < 0) {
		_sce_dnas1_common.result = rc;
		sce_sema_signalsema(_sce_dnas1_common.ready_sema);
		return rc;
	}
	rc = _sce_dnas1_common.net.set_flag(1);
	if (rc < 0) {
		_sce_dnas1_common.result = rc;
		sce_sema_signalsema(_sce_dnas1_common.ready_sema);
		return rc;
	}
	_sce_dnas1_common.core_started = 1;

	for (;;) {
		switch (_sce_dnas1_common.state) {
		case 0:
			sce_sema_waitsema(_sce_dnas1_common.lock_sema);
			_sce_dnas1_common.idle_waiters++;
			sce_sema_signalsema(_sce_dnas1_common.lock_sema);
			sce_sema_signalsema(_sce_dnas1_common.idle_sema);
			sce_sema_signalsema(_sce_dnas1_common.ready_sema);
			sce_sema_waitsema(_sce_dnas1_common.wakeup_sema);
			sce_sema_waitsema(_sce_dnas1_common.lock_sema);
			_sce_dnas1_common.idle_waiters--;
			sce_sema_signalsema(_sce_dnas1_common.lock_sema);
			if (dnas1_shutdown_called() > 0)
				goto finish;
			if (_sce_dnas1_abort_called() > 0)
				goto onabort;
			if (_sce_dnas1_set_state(1, 0, 0) < 0)
				goto onerr107;
			break;
		case 1:
			ret = _sce_dnas1_send_message(0, 0, 0);
			if (ret == SCE_DNAS_ERR_ABORTED)
				goto onabort;
			if (ret < 0)
				goto onerr;
			if (_sce_dnas1_set_state(2, 0, 0) < 0)
				goto onerr107;
			break;
		case 2:
			ret = _sce_dnas1_recv_message(5, 0, 0);
			if (ret == SCE_DNAS_ERR_ABORTED)
				goto onabort;
			if (ret == SCE_DNAS_ERR_ALL_OK) {
				if (_sce_dnas1_set_state(0, 4, 0) < 0)
					goto onerr107;
				_sce_dnas1_common.inst_result = 1;
				_sce_dnas1_common.auth_result = 1;
				_sce_dnas1_common.reg_result = 2;
				_sce_dnas1_common.status.sub_code = 0x01000124;
				break;
			}
			if (ret == SCE_DNAS_ERR_DIFFERENT_HDD) {
				_sce_dnas1_common.state = 0;
				_sce_dnas1_common.action.word &= 0xffff3fff;
				if (_sce_dnas1_set_state(1, 0, 0) < 0)
					goto onerr107;
				break;
			}
			if (ret < 0)
				goto onerr;
			if (_sce_dnas1_set_state(3, 0, 0) < 0)
				goto onerr107;
			break;
		case 3:
			if (_sce_dnas1_check_action_purpose(2) == 0 && _sce_dnas1_check_action_purpose(3) == 0)
				ret = _sce_dnas1_send_message(1, 0, 0);
			else
				ret = _sce_dnas1_send_message(1, 0, _sce_dnas1_common.msg_arg);
			if (ret == SCE_DNAS_ERR_ABORTED)
				goto onabort;
			if (ret < 0)
				goto onerr;
			if (_sce_dnas1_set_state(4, 0, 0) < 0)
				goto onerr107;
			break;
		case 4:
			if (_sce_dnas1_check_action_purpose(2) == 0 && _sce_dnas1_check_action_purpose(3) == 0)
				ret = _sce_dnas1_recv_message(6, 0, 0);
			else
				ret = _sce_dnas1_recv_message(6, 0, 1);
			if (ret == SCE_DNAS_ERR_ABORTED)
				goto onabort;
			if (ret < 0)
				goto onerr;
			if (_sce_dnas1_check_action_purpose(2) > 0 || _sce_dnas1_check_action_purpose(3) > 0) {
				dnas1_reset_state(0, 0);
				break;
			}
			if (_sce_dnas1_check_action_purpose(4) > 0) {
				if (_sce_dnas1_set_state(5, 0, 0) < 0)
					goto onerr107;
				break;
			}
			if (_sce_dnas1_check_action_purpose(1) > 0) {
				if (_sce_dnas1_set_state(12, 0, 0) < 0)
					goto onerr107;
				break;
			}
			if (_sce_dnas1_check_action_purpose(5) <= 0)
				break;
			if (_sce_dnas1_set_state(6, 0, 0) < 0)
				goto onerr107;
			break;
		case 5:
			sce_sema_waitsema(_sce_dnas1_common.lock_sema);
			_sce_dnas1_common.data_waiters++;
			sce_sema_signalsema(_sce_dnas1_common.lock_sema);
			sce_sema_waitsema(_sce_dnas1_common.data_sema);
			sce_sema_waitsema(_sce_dnas1_common.lock_sema);
			_sce_dnas1_common.data_waiters--;
			sce_sema_signalsema(_sce_dnas1_common.lock_sema);
			if (dnas1_shutdown_called() > 0)
				goto finish;
			if (_sce_dnas1_abort_called() > 0)
				goto onabort;
			if (dnas1_check_action_category(2) == 0)
				break;
			ret = _sce_dnas1_send_message(3, _sce_dnas1_common.dl_req, _sce_dnas1_common.msg_arg);
			if (ret == SCE_DNAS_ERR_ABORTED)
				goto onabort;
			if (ret < 0)
				goto onerr;
			if (_sce_dnas1_set_state(7, 0, 0) < 0)
				goto onerr107;
			break;
		case 7:
			ret = _sce_dnas1_recv_message(7, _sce_dnas1_common.dl_req, _sce_dnas1_common.msg_arg);
			if (ret == SCE_DNAS_ERR_ABORTED)
				goto onabort;
			if (ret < 0)
				goto onerr;
			if (_sce_dnas1_common.msg_arg > 0) {
				dnas1_reset_state(0, 0);
				break;
			}
			if (_sce_dnas1_set_state(5, 0, 0) < 0)
				goto onerr107;
			break;
		case 12:
			ret = _sce_dnas1_send_message(8, _sce_dnas1_common.uid_buf, 1);
			if (ret == SCE_DNAS_ERR_ABORTED)
				goto onabort;
			if (ret < 0)
				goto onerr;
			if (_sce_dnas1_set_state(13, 0, 0) < 0)
				goto onerr107;
			break;
		case 13:
			ret = _sce_dnas1_recv_message(13, _sce_dnas1_common.uid_buf, 1);
			if (ret == SCE_DNAS_ERR_ABORTED)
				goto onabort;
			if (ret < 0)
				goto onerr;
			dnas1_reset_state(0, 0);
			break;
		case 6:
			ret = _sce_dnas1_send_message(2, _sce_dnas1_common.msg_param, 1);
			if (ret == SCE_DNAS_ERR_ABORTED)
				goto onabort;
			if (ret < 0)
				goto onerr;
			if (_sce_dnas1_set_state(8, 0, 0) < 0)
				goto onerr107;
			break;
		case 8:
			ret = _sce_dnas1_recv_message(12, _sce_dnas1_common.msg_param, 1);
			if (ret == SCE_DNAS_ERR_ABORTED) {
				dnas1_shutdown();
				break;
			}
			if (ret < 0)
				goto onerr;
			dnas1_reset_state(0, 0);
			break;
		case 9:
			break;
		case 10:
		case 11:
			dnas1_reset_state(ret, 0);
			break;
		}
		continue;
onabort:
		dnas1_shutdown();
		continue;
onerr107:
		ret = SCE_DNAS_ERR_IO;
onerr:
		if (ret >= -699 && ret <= -600) /* an error of the network layer */
			_sce_dnas1_set_state(11, 0, 0);
		else if (ret < -99)
			_sce_dnas1_set_state(10, 0, 0);
		else
			_sce_dnas1_set_state(11, 0, 0);
		continue;
	}
finish:
	rc = _sce_dnas1_common.net.cleanup();
	sce_sema_signalsema(_sce_dnas1_common.exit_sema);
	sigaction(SIGUSR2, &oldact, 0);
	return rc;
}
