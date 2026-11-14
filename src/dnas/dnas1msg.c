/*
 * DNAS1 message layer, sending side: builds the HTTP request of each
 * message type and sends it (_sce_dnas1_send_message).
 */

/* Locals that held secrets are overwritten with random bytes before return. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dnas_err.h"
#include "dnas_prot.h"
#include "dnas1core.h"
#include "dnas2m11_identity.h"
#include "scednas1_state.h"
#include "thread/scesema.h"
#include "dnas1msg.h"

struct dnas1_msg_hdr;
static int dnas1_msg_send3(int code, int flag, void *ctx);
static int dnas1_emit_auth(unsigned char *buf);
static int dnas1_send_request(unsigned char *p);
static int dnas1_record_wrap(unsigned char *buf, void *msg);
static int dnas1_drain_wrap(unsigned char *buf, void *msg);
static int dnas1_emit_message(unsigned char *buf, void *msg);
static void dnas1_hdr_set_len(struct dnas1_msg_hdr *h, unsigned int v);
static int dnas1_finish_message(int type, int len, unsigned char *buf);

static int dnas1_build_message(int type, unsigned char *msg, int flag);

static void dnas1_msg_emit3(int code, int flag, unsigned char *p);

/* Writes into `buf` the path of the HTTP request to the DNAS server and
 * returns its length. If a negative debug_error is recorded the path is
 * the debug one that names it (debug_fermat/...); otherwise it is the gateway
 * path for the message type and the action purpose. Codes -2xx and -7xx count
 * only if the action block enables them; other codes below -805
 * (except -896) go to unknown_ng/ with the code in hex. */
static int dnas1_build_uri(int type, char *buf)
{
	char *uri;
	int ng;
	unsigned char *np;
	unsigned int i;

	ng = 0;
	SCE_DNAS1_PROT_BEGIN();
	switch (_sce_dnas1_common.debug_error) {
	case SCE_DNAS_ERR_BUSY:
		uri = SCE_DNAS_STR("debug_fermat/ng/busy");
		goto copy;
	case SCE_DNAS_ERR_BEFORE_SERVICE:
		uri = SCE_DNAS_STR("debug_fermat/ng/before_dnas_service");
		goto copy;
	case SCE_DNAS_ERR_AFTER_SERVICE:
		uri = SCE_DNAS_STR("debug_fermat/ng/after_dnas_service");
		goto copy;
	case SCE_DNAS_ERR_SERVICE_FINISHED:
		uri = SCE_DNAS_STR("debug_fermat/ng/dnas_service_finished");
		goto copy;
	case SCE_DNAS_ERR_TIMEOUT:
		uri = SCE_DNAS_STR("debug_fermat/ng/timeout");
		goto copy;
	case SCE_DNAS_ERR_EXTERNAL:
		uri = SCE_DNAS_STR("debug_fermat/ng/external_error");
		goto copy;
	case SCE_DNAS_ERR_DIFFERENT_HDD:
		uri = SCE_DNAS_STR("debug_fermat/ng/different_hdd");
		goto copy;
	case SCE_DNAS_ERR_ALL_OK:
		uri = SCE_DNAS_STR("debug_fermat/ng/all_ok");
		goto copy;
	case SCE_DNAS_ERR_BAD_RESPONSE:
	case SCE_DNAS_ERR_IO:
		uri = SCE_DNAS_STR("debug_fermat/ng/busy");
		goto copy;
	case SCE_DNAS_ERR_DOWN_NOT_EXIST:
	case SCE_DNAS_ERR_DOWN_BEFORE_SERVICE:
	case SCE_DNAS_ERR_DOWN_SERVICE_FINISHED:
	case SCE_DNAS_ERR_DOWN_NOT_MODIFIED:
		if ((_sce_dnas1_common.action.word & 0x3f00) != 0x2200)
			break;
		switch (_sce_dnas1_common.debug_error) {
		case SCE_DNAS_ERR_DOWN_NOT_EXIST:
			uri = SCE_DNAS_STR("debug_fermat/down_result/not_exist");
			goto copy;
		case SCE_DNAS_ERR_DOWN_BEFORE_SERVICE:
			uri = SCE_DNAS_STR("debug_fermat/down_result/before_service");
			goto copy;
		case SCE_DNAS_ERR_DOWN_SERVICE_FINISHED:
			uri = SCE_DNAS_STR("debug_fermat/down_result/service_finished");
			goto copy;
		case SCE_DNAS_ERR_DOWN_NOT_MODIFIED:
			uri = SCE_DNAS_STR("debug_fermat/down_result/not_modified");
			goto copy;
		}
		/* fall through */
	case SCE_DNAS_ERR_ID_NOT_REGISTERED:
	case SCE_DNAS_ERR_ID_INVALID_CATEGORY:
	case SCE_DNAS_ERR_ID_NOT_JOIN_IN_THIS_CATEGORY:
		if ((_sce_dnas1_common.action.word & 0x3800) != 0x800)
			break;
		switch (_sce_dnas1_common.debug_error) {
		case SCE_DNAS_ERR_ID_NOT_REGISTERED:
			uri = SCE_DNAS_STR("debug_fermat/id_result/not_registered");
			goto copy;
		case SCE_DNAS_ERR_ID_INVALID_CATEGORY:
			uri = SCE_DNAS_STR("debug_fermat/id_result/invalid_category");
			goto copy;
		case SCE_DNAS_ERR_ID_NOT_JOIN_IN_THIS_CATEGORY:
			uri = SCE_DNAS_STR("debug_fermat/id_result/not_join_in_this_category");
			goto copy;
		}
		/* fall through */
	default:
		if (_sce_dnas1_common.debug_error > -806 || _sce_dnas1_common.debug_error == -896) {
			_sce_dnas1_common.debug_error = 0;
			break;
		}
		uri = SCE_DNAS_STR("debug_fermat/ng/unknown_ng/");
		ng = 1;
		goto copy;
	case 0:
		break;
	}
	if (type == 0) {
		if (_sce_dnas1_check_action_purpose(4) == 0)
			uri = SCE_DNAS_STR("gai-gw/v2.5_i-connect");
		else
			uri = SCE_DNAS_STR("gai-gw/v2.5_d-connect");
	} else
		uri = SCE_DNAS_STR("gai-gw/v2.5_others");
copy:
	strcpy(buf, uri);
	if (ng > 0)
		sprintf(buf + strlen(buf), "%02x", -800 - _sce_dnas1_common.debug_error);
	SCE_DNAS1_PROT_END();
	np = (unsigned char *)&uri;
	for (i = 0; i < 4; i++)
		np[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	np = (unsigned char *)&ng;
	for (i = 0; i < 4; i++)
		np[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	return strlen(buf);
}

/* Writes into `buf` the HTTP request line and headers: "POST /" + the path
 * from dnas1_build_uri + " HTTP/1.0", User-Agent, Content-Type and
 * Content-Length (`len`). Returns the length written. */
static int dnas1_finish_message(int type, int len, unsigned char *buf)
{
	char num[16];
	int n;
	char *s;
	char *p;
	unsigned char *na;
	unsigned int ja;
	unsigned char *nb;
	unsigned int jb;
	unsigned char *nc;
	unsigned int jc;

	SCE_DNAS1_PROT_BEGIN();
	s = SCE_DNAS_STR("POST /");
	strcpy((char *)buf, s);
	p = (char *)buf + strlen(s);
	SCE_DNAS1_PROT_END();
	n = dnas1_build_uri(type, p);
	if (n < 0)
		return n;
	p += n;
	SCE_DNAS1_PROT_BEGIN();
	s = SCE_DNAS_STR(" HTTP/1.0\r\n");
	strcpy(p, s);
	p += strlen(s);
	s = SCE_DNAS_STR("User-Agent: open sesame asdfjkl\r\n");
	strcpy(p, s);
	p += strlen(s);
	s = SCE_DNAS_STR("Content-Type: image/gif\r\n");
	strcpy(p, s);
	p += strlen(s);
	s = SCE_DNAS_STR("Content-Length: ");
	strcpy(p, s);
	p += strlen(s);
	s = SCE_DNAS_STR("\r\n");
	sprintf(num, "%d%s", len, s);
	strcpy(p, num);
	n = strlen(num);
	p += n;
	strcpy(p, s);
	p += strlen(s);
	SCE_DNAS1_PROT_END();
	na = (unsigned char *)&n;
	for (ja = 0; ja < 4; ja++)
		na[ja] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	nb = (unsigned char *)num;
	for (jb = 0; jb < 16; jb++)
		nb[jb] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	nc = (unsigned char *)&s;
	for (jc = 0; jc < 4; jc++)
		nc[jc] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	return p - (char *)buf;
}

/* Clears the two header bytes at p+2, then sets the flag bit (if `flag`)
 * and the low nibble of `code` in bits 8..11. */
static void dnas1_msg_emit3(int code, int flag, unsigned char *p)
{
	struct dnas1_msg_bits *h = (struct dnas1_msg_bits *)(p + 2);
	unsigned int i;

	SCE_DNAS1_PROT_BEGIN();
	for (i = 0; i < 2; i++)
		((unsigned char *)h)[i] = 0;
	if (flag != 0)
		h->flag = 1;
	h->code = code & 0xf;
	SCE_DNAS1_PROT_END();
}

static int dnas1_msg_send3(int code, int flag, void *ctx)
{
	_sce_dnas1_get_action_field(ctx);
	dnas1_msg_emit3((int)code, (int)flag, (unsigned char *)ctx);
	_sce_dnas1_get_session_id((char *)ctx + 4);
	return 40;
}

/* Emits version and pass phrase, the 256 bytes from 11070, and, if 11071
 * agrees and bit 0 of mode is clear, the fields from 11067 and
 * 11068. If 11071 fails the result is reported with
 * _sce_dnas1_set_auth_result. Returns the bytes emitted. */
static int dnas1_emit_auth(unsigned char *buf)
{
	unsigned char tmp[16];
	unsigned char *p;
	unsigned char *q;
	unsigned char *q2;
	unsigned char *w;
	unsigned int i;
	int r;

	p = buf + _sce_dnas1_get_version((short *)buf);
	p += _sce_dnas1_get_pass_phrase(p);
	q = _sce_dnas1_common.auth_data;
	q2 = _sce_dnas1_common.auth_token;
	r = __sce_dnas2_symbol11070(q, q2);
	if (r < 0)
		return r;
	w = _sce_dnas1_common.auth_token;
	for (i = 0; i < 256; i++)
		p[i] = w[i];
	p += r;
	r = __sce_dnas2_symbol11071();
	if (r < 0) {
		tmp[0] = 0;
		return _sce_dnas1_set_auth_result(tmp);
	}
	if (r <= 0)
		return p - buf;
	if ((_sce_dnas1_common.mode & 1) != 0)
		return p - buf;
	r = __sce_dnas2_symbol11067(p);
	if (r < 0)
		return r;
	p += r;
	r = __sce_dnas2_symbol11068(p);
	if (r < 0)
		return r;
	p += r;
	return p - buf;
}

/* Packs the request from auth_data, auth_token, challenge and password
 * with __sce_dnas2_symbol11069 at the message cursor `p`; on success the cursor advances by 144 bytes. */
static int dnas1_send_request(unsigned char *p)
{
	unsigned char *cur = p;
	unsigned char *b = _sce_dnas1_common.auth_data;
	int r;

	r = __sce_dnas2_symbol11069(b, _sce_dnas1_common.auth_token, _sce_dnas1_common.challenge, cur,
	    _sce_dnas1_common.password);
	if (r < 0)
		return r;
	cur += 144;
	return cur - p;
}

/* Writes header and body into `buf` and returns the total bytes written. */
static int dnas1_emit_message(unsigned char *buf, void *msg)
{
	unsigned char *p = buf;

	p += _sce_dnas1_get_down_IDslot((unsigned short *)buf);
	p += _sce_dnas1_get_lastdate(p, msg);
	return p - buf;
}

/* Body of case 8 of dnas1_build_message. */
static int dnas1_record_wrap(unsigned char *buf, void *msg)
{
	return _sce_dnas1_get_idreq_category((unsigned int *)buf);
}

/* Body of case 2 of dnas1_build_message. */
static int dnas1_drain_wrap(unsigned char *buf, void *msg)
{
	return _sce_dnas1_get_record_data(buf, msg);
}

/* Stores the message length, big-endian. */
static void dnas1_hdr_set_len(struct dnas1_msg_hdr *h, unsigned int v)
{
	h->len = (v >> 24) | ((v >> 8) & 0xff00) | ((v & 0xff00) << 8) | (v << 24);
}

/* Builds message `type` in data_buf: the common header,
 * then the body for types 0..3 and 8 (anything else is error SCE_DNAS_ERR_IO), then
 * the length in the header and the HTTP request (dnas1_finish_message). */
static int dnas1_build_message(int type, unsigned char *msg, int flag)
{
	int n;
	int b;
	int len;
	unsigned char *p;
	int ret;
	unsigned char *na;
	unsigned int ja;
	unsigned char *nb;
	unsigned int jb;
	unsigned char *nc;
	unsigned int jc;
	unsigned char *nd;
	unsigned int jd;

	b = 0;
	len = 0;
	p = _sce_dnas1_common.data_buf;
	n = dnas1_msg_send3(type, flag, p);
	p += n;
	len += n;

	switch (type) {
	case 0:
		b = dnas1_emit_auth(p);
		if (b < 0) {
			ret = b;
			goto noise;
		}
		p += b;
		len += b;
		break;
	case 1:
		b = dnas1_send_request(p);
		if (b < 0) {
			ret = b;
			goto noise;
		}
		p += b;
		len += b;
		break;
	case 3:
		b = dnas1_emit_message(p, msg);
		if (b < 0) {
			ret = b;
			goto noise;
		}
		p += b;
		len += b;
		break;
	case 8:
		b = dnas1_record_wrap(p, msg);
		if (b < 0) {
			ret = b;
			goto noise;
		}
		p += b;
		len += b;
		break;
	case 2:
		b = dnas1_drain_wrap(p, msg);
		if (b < 0) {
			ret = b;
			goto noise;
		}
		p += b;
		len += b;
		break;
	default:
		ret = SCE_DNAS_ERR_IO;
		goto noise;
	}

	p = _sce_dnas1_common.data_buf;
	dnas1_hdr_set_len((struct dnas1_msg_hdr *)p, b);
	_sce_dnas1_common.data_len = n + b;
	b = dnas1_finish_message(type, len, _sce_dnas1_common.hdr_buf);
	if (b < 0) {
		ret = b;
		goto noise;
	}
	_sce_dnas1_common.hdr_len = b;
	ret = b + len;

noise:
	na = (unsigned char *)&n;
	for (ja = 0; ja < 4; ja++)
		na[ja] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	nb = (unsigned char *)&b;
	for (jb = 0; jb < 4; jb++)
		nb[jb] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	nc = (unsigned char *)&len;
	for (jc = 0; jc < 4; jc++)
		nc[jc] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	nd = (unsigned char *)&p;
	for (jd = 0; jd < 4; jd++)
		nd[jd] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	return ret;
}

/* Builds message `type`, connects, sends header and body through the transport and
 * returns the bytes written; the buffers are wiped, and on error the connection is closed. */
int _sce_dnas1_send_message(int type, unsigned char *msg, int flag)
{
	int ret;
	int i;
	int i2;
	unsigned char *src;
	unsigned char *dst;
	unsigned char *p;
	unsigned char *q;
	unsigned char *na;
	int ja;
	unsigned char *nb;
	unsigned int jb;
	unsigned char *nc;
	unsigned int jc;
	unsigned char *nd;
	int jd;
	unsigned char *ne;
	unsigned int je;
	unsigned char *nf;
	unsigned int jf;

	ret = dnas1_build_message(type, msg, flag);
	if (ret < 0)
		return ret;
	if (_sce_dnas1_abort_called() > 0)
		return SCE_DNAS_ERR_ABORTED;
	sce_sema_signalsema(_sce_dnas1_common.io_sema);
	ret = _sce_dnas1_common.net.connect();
	sce_sema_pollsema(_sce_dnas1_common.io_sema);
	if (ret < 0)
		goto err;

	dst = _sce_dnas1_common.send_buf;
	src = _sce_dnas1_common.hdr_buf;
	for (i = 0; i < _sce_dnas1_common.hdr_len; i++)
		dst[i] = src[i];

	q = _sce_dnas1_common.data_buf;
	p = _sce_dnas1_common.send_buf + _sce_dnas1_common.hdr_len;
	_sce_dnas1_common.send_len = _sce_dnas1_common.hdr_len;
	for (i2 = 0; i2 < _sce_dnas1_common.data_len; i2++)
		p[i2] = q[i2];
	_sce_dnas1_common.send_len = _sce_dnas1_common.send_len + _sce_dnas1_common.data_len;

	na = _sce_dnas1_common.hdr_buf;
	for (ja = 0; ja < 1024; ja++)
		na[ja] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	nb = _sce_dnas1_common.data_buf;
	for (jb = 0; jb < 32819; jb++)
		nb[jb] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);

	sce_sema_signalsema(_sce_dnas1_common.io_sema);
	ret = _sce_dnas1_common.net.write(_sce_dnas1_common.send_buf, _sce_dnas1_common.send_len,
	    _sce_dnas1_common.write_arg);
	sce_sema_pollsema(_sce_dnas1_common.io_sema);
	if (ret < 0)
		goto err;
	if (ret != _sce_dnas1_common.send_len) {
		ret = SCE_DNAS_ERR_IO;
		goto err;
	}
	nc = _sce_dnas1_common.send_buf;
	for (jc = 0; jc < 33843; jc++)
		nc[jc] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	return ret;

err:
	nd = _sce_dnas1_common.hdr_buf;
	for (jd = 0; jd < 1024; jd++)
		nd[jd] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	ne = _sce_dnas1_common.data_buf;
	for (je = 0; je < 32819; je++)
		ne[je] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	nf = _sce_dnas1_common.send_buf;
	for (jf = 0; jf < 33843; jf++)
		nf[jf] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	sce_sema_signalsema(_sce_dnas1_common.io_sema);
	_sce_dnas1_common.net.disconnect();
	sce_sema_pollsema(_sce_dnas1_common.io_sema);
	return ret;
}
