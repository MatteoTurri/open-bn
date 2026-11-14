/*
 * DNAS1 message layer, receiving side: reads the HTTP reply, checks its
 * header and parses the fields of each reply type (_sce_dnas1_recv_message).
 */

/* Locals that held secrets are overwritten with random bytes before return. */

#include <stdlib.h>
#include <string.h>
#include "dnas_err.h"
#include "dnas_prot.h"
#include "dnas1core.h"
#include "dnas1recv.h"
#include "dnas2m11_identity.h"
#include "scednas1_state.h"
#include "thread/scesema.h"
#include "dnas1msg.h"

static int dnas1_recv_header(unsigned char *buf, int *len);
static int dnas1_dispatch_message(int code, int len, void *ctx, int flags, int *st);
static int dnas1_check_reply(int code, int flags, unsigned int *lenout, unsigned char *p, int *okout);
static int dnas1_recv_exact(unsigned char *p, int want, int *st);
static int dnas1_emit_checked(unsigned char *buf, int want);
static int dnas1_emit_results(unsigned char *buf, int want);
static int dnas1_emit_down(unsigned char *buf, int want, void *ctx, int *err);
static int dnas1_emit_opt(unsigned char *buf, int want, void *ctx, int *err);
static int dnas1_emit_id(unsigned char *buf, int want, void *ctx, int *err);

static int dnas1_cmp3(const void *a, const void *b, unsigned int n)
{
	const unsigned char *p = (const unsigned char *)a;
	const unsigned char *q = (const unsigned char *)b;
	unsigned int i;

	for (i = 0; i < n; i++) {
		if (p[i] < q[i])
			return -1;
		if (p[i] > q[i])
			return 1;
	}
	return 0;
}

/* Receives and compares with `pat` byte by byte; returns the bytes consumed
 * or a negative error. If `have` is set the first byte is already in the
 * buffer and is compared without receiving it. */
static int dnas1_recv_match(char *buf, const char *pat, int have, int *cnt)
{
	char *p;
	unsigned char *np;
	int ret;
	unsigned int i;
	int n;

	SCE_DNAS1_PROT_BEGIN();
	p = buf;
	n = 1;
	if (have) {
		if (*p != *pat) {
			ret = p - buf;
			goto out;
		}
		p++;
		pat++;
	}
	do {
		ret = SCE_DNAS_ERR_BAD_RESPONSE;
		if (*cnt + 1 > 1024)
			goto out;
		sce_sema_signalsema(_sce_dnas1_common.io_sema);
		ret = _sce_dnas1_common.net.read((unsigned char *)p, n);
		sce_sema_pollsema(_sce_dnas1_common.io_sema);
		if (ret < 0)
			goto out;
		if (ret != n) {
			ret = SCE_DNAS_ERR_IO;
			goto out;
		}
		*cnt += ret;
		if (*p != *pat) {
			ret = p - buf;
			goto out;
		}
		pat += ret;
		p += ret;
	} while (*pat != 0);
	ret = p - buf;
out:
	np = (unsigned char *)&p;
	for (i = 0; i < 4; i++)
		np[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS1_PROT_END();
	return ret;
}

/* Receives one byte at a time until one is NOT in `set`, and returns the
 * pointer it reached (as an int). The caller's counter is capped at 1024. */
static int dnas1_recv_until(char *buf, const char *set, int *cnt)
{
	char *p;
	unsigned char *np;
	int ret;
	int j;
	unsigned int i;

	SCE_DNAS1_PROT_BEGIN();
	p = buf;
	for (;;) {
		if (*cnt + 1 > 1024) {
			ret = SCE_DNAS_ERR_BAD_RESPONSE;
			goto out;
		}
		sce_sema_signalsema(_sce_dnas1_common.io_sema);
		ret = _sce_dnas1_common.net.read((unsigned char *)p, 1);
		sce_sema_pollsema(_sce_dnas1_common.io_sema);
		if (ret < 0)
			goto out;
		(*cnt)++;
		for (j = 0; set[j] != 0; j++)
			if (*p == set[j])
				break;
		if (set[j] == 0) {
			ret = (int)p;
			goto out;
		}
		p++;
	}
out:
	np = (unsigned char *)&p;
	for (i = 0; i < 4; i++)
		np[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS1_PROT_END();
	return ret;
}

/* Receives the HTTP header of the reply: the status line
 * "HTTP/1.0 200 OK\r\n", then skips to "Content-Length: ", converts its
 * digits into *len (at most 32819, the size of the data buffer) and skips to
 * the empty line. Returns the bytes consumed. */
static int dnas1_recv_header(unsigned char *buf, int *len)
{
	char tmp[256];
	int cnt;
	int n;
	int r;
	int have;
	char *p;
	unsigned int i;
	unsigned int j;
	char *s;
	char *eol;

	cnt = 0;
	SCE_DNAS1_PROT_BEGIN();
	strncpy(tmp, SCE_DNAS_STR("HTTP/1.0 200 OK\r\n"), 256);
	s = tmp;
	n = strlen(s);
	SCE_DNAS1_PROT_END();
	r = dnas1_recv_match((char *)buf, s, 0, &cnt);
	if (r < 0)
		return r;
	if (r == 0)
		return SCE_DNAS_ERR_BAD_RESPONSE;
	if (r < n)
		return SCE_DNAS_ERR_BAD_RESPONSE;
	p = (char *)buf + r;
	SCE_DNAS1_PROT_BEGIN();
	strncpy(tmp, SCE_DNAS_STR("Content-Length: "), 256);
	s = tmp;
	n = strlen(s);
	SCE_DNAS1_PROT_END();
	have = 0;
	for (;;) {
		r = dnas1_recv_match(p, s, have, &cnt);
		if (r < 0)
			return r;
		if (r == n)
			break;
		if (r == 0) {
			have = 0;
			p++;
		} else {
			have = 1;
			p += r;
		}
	}
	p += r;
	s = (char *)dnas1_recv_until(p, "0123456789", &cnt);
	*len = strtol(p, 0, 10);
	if (*len > 32819)
		return SCE_DNAS_ERR_BAD_RESPONSE;
	p = s;
	SCE_DNAS1_PROT_BEGIN();
	eol = SCE_DNAS_STR("\r\n");
	strncpy(tmp, eol, 256);
	strncat(tmp, eol, strlen(eol));
	s = tmp;
	n = strlen(s);
	SCE_DNAS1_PROT_END();
	have = 1;
	for (;;) {
		r = dnas1_recv_match(p, s, have, &cnt);
		if (r < 0)
			return r;
		if (r == n)
			break;
		if (r == 0) {
			have = 0;
			p++;
		} else {
			have = 1;
			p += r;
		}
	}
	p += r;
	for (i = 0; i < 256; i++)
		tmp[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	for (j = 0; j < 4; j++)
		tmp[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	return p - (char *)buf;
}

/* Compares 2 bytes with the action block. */
static int dnas1_check_tag2(void *p)
{
	int r;

	SCE_DNAS1_PROT_BEGIN();
	r = (dnas1_cmp3(p, (unsigned char *)&_sce_dnas1_common.action, 2) == 0) ? 0 : -1;
	SCE_DNAS1_PROT_END();
	return r;
}

/* Checks that the header at p+2 has type 0x400 and only bit 0 set in the
 * flags, then compares its code (bits 8..11) and flag (bit 7) with the
 * expected ones. */
static int dnas1_verify_tag(int code, int flags, unsigned char *p)
{
	struct dnas1_msg_bits tmp;
	unsigned char *w;
	unsigned int i;
	int r;

	p += 2;
	SCE_DNAS1_PROT_BEGIN();
	if (((struct dnas1_msg_bits *)p)->code == 4) {
		r = 0;
	} else if ((flags & -2) != 0) {
		r = -1;
	} else {
		w = (unsigned char *)&tmp;
		for (i = 0; i < 2; i++)
			w[i] = 0;
		tmp.code = code & 0xf;
		tmp.flag = flags & 1;
		r = (dnas1_cmp3(p, &tmp, 2) != 0) ? -1 : 1;
	}
	SCE_DNAS1_PROT_END();
	return r;
}

/* Compares 32 bytes from p+4 with the session id. */
static int dnas1_check_tag32(void *p)
{
	char *q = (char *)p + 4;
	int r;

	SCE_DNAS1_PROT_BEGIN();
	r = (dnas1_cmp3(q, _sce_dnas1_common.session_id, 32) == 0) ? 0 : -1;
	SCE_DNAS1_PROT_END();
	return r;
}

/* Checks the reply: the 2-byte tag, the type and flag tag and, unless the
 * code is 5, the 32-byte signature; for code 5 it stores the session id
 * instead. Returns the length the header announces in *lenout, and 40. */
static int dnas1_check_reply(int code, int flags, unsigned int *lenout, unsigned char *p, int *okout)
{
	int r;
	unsigned int v;

	if (dnas1_check_tag2(p) < 0)
		return SCE_DNAS_ERR_BAD_RESPONSE;
	r = dnas1_verify_tag(code, flags, p);
	if (r < 0)
		return SCE_DNAS_ERR_BAD_RESPONSE;
	if (r == 0)
		*okout = 1;
	if (code == 5) {
		_sce_dnas1_set_session_id(p + 4);
	} else if (dnas1_check_tag32(p) < 0) {
		return SCE_DNAS_ERR_BAD_RESPONSE;
	}
	v = ((struct dnas1_msg_hdr *)p)->len;
	*lenout = (v >> 24) | ((v >> 8) & 0xff00) | ((v & 0xff00) << 8) | (v << 24);
	return 40;
}

/* Reads the auth result (unless bit 0 of mode is set) and the
 * challenge, and checks the total length. */
static int dnas1_emit_checked(unsigned char *buf, int want)
{
	int r;
	int len;
	unsigned char *p;

	p = buf;
	r = __sce_dnas2_symbol11071();
	if (r < 0)
		return r;
	if (r != 0) {
		if ((_sce_dnas1_common.mode & 1) == 0)
			p = buf + _sce_dnas1_set_auth_result(buf);
	}
	p += _sce_dnas1_set_challenge(p);
	len = p - buf;
	if (want != len)
		return SCE_DNAS_ERR_BAD_RESPONSE;
	return len;
}

/* Reads the reg and inst result fields and the 128-byte signature, checks
 * the length, then (if debug_error is not negative) has DNAS2 check the
 * block, and clears bit 0 of mode. */
static int dnas1_emit_results(unsigned char *buf, int want)
{
	unsigned char *p;
	unsigned char *q;
	int r;
	int len;

	p = buf;
	p += _sce_dnas1_set_reg_result(p);
	p += _sce_dnas1_set_inst_result(p);
	q = p;
	p = p + 128;
	len = p - buf;
	if (want != len)
		return SCE_DNAS_ERR_BAD_RESPONSE;
	if (_sce_dnas1_common.debug_error >= 0) {
		r = __sce_dnas2_symbol11071();
		if (r < 0)
			return r;
		if (r != 0) {
			r = __sce_dnas2_symbol11079(_sce_dnas1_common.password);
			if (r < 0)
				return r;
		} else {
			r = __sce_dnas2_symbol11078(q, _sce_dnas1_common.password);
			if (r < 0)
				return r;
		}
		p += r;  /* redundant as written */
	}
	if ((_sce_dnas1_common.mode & 1) != 0)
		_sce_dnas1_common.mode &= -2;
	return len;
}

/* Reads the "down" fields (date, size and data). If down_result is
 * set nothing is read and *err gets the corresponding SCE_DNAS_ERR_DOWN_* code. */
static int dnas1_emit_down(unsigned char *buf, int want, void *ctx, int *err)
{
	unsigned char *p;
	sce_dnas1_state *st;
	int r;
	int used;

	p = buf + _sce_dnas1_set_down_result(buf);
	if (_sce_dnas1_common.down_result != 0) {
		SCE_DNAS1_PROT_BEGIN();
		st = &_sce_dnas1_common;
		switch (st->down_result) {
		case 1:
			*err = SCE_DNAS_ERR_DOWN_NOT_EXIST;
			break;
		case 2:
			*err = SCE_DNAS_ERR_DOWN_BEFORE_SERVICE;
			break;
		case 3:
			*err = SCE_DNAS_ERR_DOWN_SERVICE_FINISHED;
			break;
		case 4:
			*err = SCE_DNAS_ERR_DOWN_NOT_MODIFIED;
			break;
		}
		SCE_DNAS1_PROT_END();
	} else {
		p += _sce_dnas1_set_lastdate(p, ctx);
		r = _sce_dnas1_set_downdata_size(p, ctx);
		if (r < 0)
			return r;
		p += r;
		p += _sce_dnas1_set_downdata(p, ctx);
	}
	used = p - buf;
	if (want != used)
		return SCE_DNAS_ERR_BAD_RESPONSE;
	return used;
}

/* Reads the "id" fields. If id_result is set nothing is read and
 * *err gets the corresponding SCE_DNAS_ERR_ID_* result code. */
static int dnas1_emit_id(unsigned char *buf, int want, void *ctx, int *err)
{
	unsigned char *p;
	int r;
	int used;

	p = buf + _sce_dnas1_set_id_result(buf);
	if (_sce_dnas1_common.id_result != 0) {
		SCE_DNAS1_PROT_BEGIN();
		switch (_sce_dnas1_common.id_result) {
		case 2:
			*err = SCE_DNAS_ERR_ID_NOT_REGISTERED;
			break;
		case 3:
			*err = SCE_DNAS_ERR_ID_INVALID_CATEGORY;
			break;
		case 4:
			*err = SCE_DNAS_ERR_ID_NOT_JOIN_IN_THIS_CATEGORY;
			break;
		}
		SCE_DNAS1_PROT_END();
	} else {
		r = _sce_dnas1_set_id_size(p, ctx);
		if (r < 0)
			return r;
		p += r;
		p += _sce_dnas1_set_id(p, ctx);
	}
	used = p - buf;
	if (want != used)
		return SCE_DNAS_ERR_BAD_RESPONSE;
	return used;
}

/* Reads the header and, if record_result is set, goes through an empty
 * protected region; then checks the length. */
static int dnas1_emit_opt(unsigned char *buf, int want, void *ctx, int *err)
{
	unsigned char *p;
	int r;

	(void)ctx;
	(void)err;
	p = buf + _sce_dnas1_set_record_result(buf);
	if (_sce_dnas1_common.record_result != 0) {
		SCE_DNAS1_PROT_BEGIN();
		SCE_DNAS1_PROT_END();
	}
	r = p - buf;
	if (want != r)
		r = SCE_DNAS_ERR_BAD_RESPONSE;
	return r;
}

/* Reads the NG-reason field and checks that exactly `want` bytes were consumed (else SCE_DNAS_ERR_BAD_RESPONSE). */
static int dnas1_recv_exact(unsigned char *p, int want, int *st)
{
	unsigned char *cur = p;
	int n;

	n = _sce_dnas1_set_ng_reason(cur, st);
	if (n < 0)
		return n;
	cur += n;
	return (want == cur - p) ? cur - p : SCE_DNAS_ERR_BAD_RESPONSE;
}

/* Dispatches the reply: checks the header (dnas1_check_reply) and the
 * announced length, then either enters the protected region (which reports
 * debug_error if it is one of the five known ones) and
 * receives the rest, or switches on `code`: 5, 6, 7, 12 and 13 have a
 * handler, anything else is SCE_DNAS_ERR_BAD_RESPONSE. */
static int dnas1_dispatch_message(int code, int len, void *ctx, int flags, int *st)
{
	unsigned int need;
	int more;
	unsigned char *p;
	int r;
	int n;

	need = 0;
	more = 0;
	p = _sce_dnas1_common.data_buf;
	r = dnas1_check_reply(code, flags, &need, p, &more);
	if (r < 0)
		return r;
	len -= r;
	p += r;
	if (len != need)
		return SCE_DNAS_ERR_BAD_RESPONSE;
	if (more > 0) {
		SCE_DNAS1_PROT_BEGIN();
		switch (_sce_dnas1_common.debug_error) {
		case SCE_DNAS_ERR_ID_FAILED:
			return SCE_DNAS_ERR_ID_FAILED;
		case SCE_DNAS_ERR_ID_BLOCK:
			return SCE_DNAS_ERR_ID_BLOCK;
		case SCE_DNAS_ERR_ID_MEDIUM:
			return SCE_DNAS_ERR_ID_MEDIUM;
		case SCE_DNAS_ERR_IO:
			return SCE_DNAS_ERR_IO;
		case SCE_DNAS_ERR_BAD_RESPONSE:
			return SCE_DNAS_ERR_BAD_RESPONSE;
		}
		SCE_DNAS1_PROT_END();
		n = dnas1_recv_exact(p, len, st);
done:
		if (n < 0)
			return n;
		p += n;
	} else {
		switch (code) {
		case 5:
			n = dnas1_emit_checked(p, len);
			goto done;
		case 6:
			n = dnas1_emit_results(p, len);
			goto done;
		case 7:
			n = dnas1_emit_down(p, len, ctx, st);
			goto done;
		case 13:
			n = dnas1_emit_id(p, len, ctx, st);
			goto done;
		case 12:
			n = dnas1_emit_opt(p, len, ctx, st);
			goto done;
		default:
			return SCE_DNAS_ERR_BAD_RESPONSE;
		}
	}
	return p - _sce_dnas1_common.data_buf;
}

/* Receives the reply to message `code`: reads header and body, dispatches the body
 * and disconnects; returns the total length or the first error (status of the reply included). */
int _sce_dnas1_recv_message(int code, void *ctx, int flags)
{
	int len;
	int status;
	int ret;
	unsigned char *hp;
	unsigned char *dp;
	int i;
	unsigned int j;
	unsigned char *ep;
	unsigned int k;

	status = 0;
	ret = dnas1_recv_header(_sce_dnas1_common.hdr_buf, &len);
	if (ret < 0)
		goto err;
	_sce_dnas1_common.hdr_len = ret;
	if (_sce_dnas1_abort_called() > 0) {
		ret = SCE_DNAS_ERR_ABORTED;
		goto err;
	}
	hp = _sce_dnas1_common.hdr_buf;
	for (i = 0; i < 1024; i++)
		hp[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	sce_sema_signalsema(_sce_dnas1_common.io_sema);
	ret = _sce_dnas1_common.net.read(_sce_dnas1_common.data_buf, len);
	sce_sema_pollsema(_sce_dnas1_common.io_sema);
	if (ret < 0)
		goto err;
	if (ret != len) {
		ret = SCE_DNAS_ERR_IO;
		goto err;
	}
	_sce_dnas1_common.data_len = ret;
	ret = dnas1_dispatch_message(code, ret, ctx, flags, &status);
	if (ret < 0)
		goto err;
	dp = _sce_dnas1_common.data_buf;
	for (j = 0; j < 32819; j++)
		dp[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	sce_sema_signalsema(_sce_dnas1_common.io_sema);
	ret = _sce_dnas1_common.net.disconnect();
	sce_sema_pollsema(_sce_dnas1_common.io_sema);
	if (ret < 0)
		return ret;
	if (_sce_dnas1_abort_called() > 0)
		return SCE_DNAS_ERR_ABORTED;
	if (status != 0)
		return status;
	return _sce_dnas1_common.hdr_len + _sce_dnas1_common.data_len;

err:
	ep = _sce_dnas1_common.data_buf;
	for (k = 0; k < 32819; k++)
		ep[k] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	sce_sema_signalsema(_sce_dnas1_common.io_sema);
	_sce_dnas1_common.net.disconnect();
	sce_sema_pollsema(_sce_dnas1_common.io_sema);
	return ret;
}
