/*
 * DNAS1 state accessors: getters and setters on the fields of
 * _sce_dnas1_common. The bodies run
 * inside self-modifying regions bounded by SCE_DNAS1_PROT_BEGIN/END. Each
 * accessor returns the number of bytes it copied.
 */

#include "dnas_err.h"
#include "scednas1_state.h"
#include "dnas_prot.h"

/* Writes the version halfwords 0x0200 and 0x0500. */
int _sce_dnas1_get_version(short *out)
{
	out[0] = 0x0200;
	out[1] = 0x0500;
	return 4;
}

/* --- password / pass phrase --------------------------------------------- */

/* Stores the 16-byte password. */
int _sce_dnas1_keep_password(unsigned char *p)
{
	unsigned int i;
	unsigned char *dst;

	SCE_DNAS1_PROT_BEGIN();
	dst = _sce_dnas1_common.password;
	for (i = 0; i < 16; i++)
		dst[i] = p[i];
	SCE_DNAS1_PROT_END();
	return 16;
}

/* Copies out the 8-byte pass phrase. */
int _sce_dnas1_get_pass_phrase(unsigned char *p)
{
	unsigned int i;
	unsigned char *src;

	SCE_DNAS1_PROT_BEGIN();
	src = _sce_dnas1_common.pass_phrase;
	for (i = 0; i < 8; i++)
		p[i] = src[i];
	SCE_DNAS1_PROT_END();
	return 8;
}

/* Stores the 8-byte pass phrase. */
int _sce_dnas1_set_pass_phrase(unsigned char *p)
{
	unsigned int i;
	unsigned char *dst = _sce_dnas1_common.pass_phrase;

	SCE_DNAS1_PROT_BEGIN();
	for (i = 0; i < 8; i++)
		dst[i] = p[i];
	SCE_DNAS1_PROT_END();
	return 8;
}

/* --- authentication data --- */

/* Stores the 64 KB authentication data. */
int _sce_dnas1_set_auth_data(unsigned char *p)
{
	unsigned int i;
	unsigned char *dst = _sce_dnas1_common.auth_data;

	SCE_DNAS1_PROT_BEGIN();
	for (i = 0; i < 0x10000; i++)
		dst[i] = p[i];
	SCE_DNAS1_PROT_END();
	return 0x10000;
}

/* Copies out the 32-byte session id. */
int _sce_dnas1_get_session_id(unsigned char *p)
{
	unsigned int i;
	unsigned char *src;

	SCE_DNAS1_PROT_BEGIN();
	src = _sce_dnas1_common.session_id;
	for (i = 0; i < 32; i++)
		p[i] = src[i];
	SCE_DNAS1_PROT_END();
	return 32;
}

/* Stores the 32-byte session id. */
int _sce_dnas1_set_session_id(unsigned char *p)
{
	unsigned int i;
	unsigned char *dst;

	SCE_DNAS1_PROT_BEGIN();
	dst = _sce_dnas1_common.session_id;
	for (i = 0; i < 32; i++)
		dst[i] = p[i];
	SCE_DNAS1_PROT_END();
	return 32;
}

/* --- one-byte verdicts --- */

/* Stores the one-byte authentication verdict. */
int _sce_dnas1_set_auth_result(unsigned char *p)
{
	SCE_DNAS1_PROT_BEGIN();
	_sce_dnas1_common.auth_result = *p;
	SCE_DNAS1_PROT_END();
	return 1;
}

/* Stores the 288-byte challenge. */
int _sce_dnas1_set_challenge(unsigned char *p)
{
	unsigned int i;
	unsigned char *dst;

	SCE_DNAS1_PROT_BEGIN();
	dst = _sce_dnas1_common.challenge;
	for (i = 0; i < 288; i++)
		dst[i] = p[i];
	SCE_DNAS1_PROT_END();
	return 288;
}

/* Stores the one-byte registration verdict. */
int _sce_dnas1_set_reg_result(unsigned char *p)
{
	SCE_DNAS1_PROT_BEGIN();
	_sce_dnas1_common.reg_result = *p;
	SCE_DNAS1_PROT_END();
	return 1;
}

/* Stores the one-byte install verdict. */
int _sce_dnas1_set_inst_result(unsigned char *p)
{
	SCE_DNAS1_PROT_BEGIN();
	_sce_dnas1_common.inst_result = *p;
	SCE_DNAS1_PROT_END();
	return 1;
}

/* Stores the one-byte download verdict. */
int _sce_dnas1_set_down_result(unsigned char *p)
{
	SCE_DNAS1_PROT_BEGIN();
	_sce_dnas1_common.down_result = *p;
	SCE_DNAS1_PROT_END();
	return 1;
}

/* --- action field / slot / category --- */

/* Copies out the 2-byte action field. */
int _sce_dnas1_get_action_field(unsigned char *p)
{
	int i;
	unsigned char *src;

	SCE_DNAS1_PROT_BEGIN();
	src = (unsigned char *)&_sce_dnas1_common.action;
	for (i = 0; i < 2; i++)
		p[i] = src[i];
	SCE_DNAS1_PROT_END();
	return 2;
}

/* Reads the big-endian download ID slot through dl_req. */
int _sce_dnas1_get_down_IDslot(unsigned short *p)
{
	unsigned short v;

	SCE_DNAS1_PROT_BEGIN();
	v = *(unsigned short *)_sce_dnas1_common.dl_req;
	*p = (unsigned short)((v >> 8) | ((v & 0xff) << 8));
	SCE_DNAS1_PROT_END();
	return 2;
}

/* --- date of the last access (6 bytes at +4 of the block passed in) --- */

/* Copies out the 6-byte last access date of `blk`. */
int _sce_dnas1_get_lastdate(unsigned char *p, unsigned char *blk)
{
	int i;
	unsigned char *src;

	SCE_DNAS1_PROT_BEGIN();
	src = blk + 4;
	for (i = 0; i < 6; i++)
		p[i] = src[i];
	SCE_DNAS1_PROT_END();
	return 6;
}

/* Stores the 6-byte last access date into `blk`. */
int _sce_dnas1_set_lastdate(unsigned char *p, unsigned char *blk)
{
	int i;
	unsigned char *dst;

	SCE_DNAS1_PROT_BEGIN();
	dst = blk + 4;
	for (i = 0; i < 6; i++)
		dst[i] = p[i];
	SCE_DNAS1_PROT_END();
	return 6;
}

/* Stores the big-endian size at `p` in *io->downdata_size; SCE_DNAS_ERR_BAD_RESPONSE if it is 0
 * or above 32 KB. */
int _sce_dnas1_set_downdata_size(unsigned char *p, sce_dnas1_io *io)
{
	unsigned char buf[4];
	unsigned int i, k;
	unsigned int n, v;
	int ret;
	unsigned char *b1;
	unsigned int max = 0x8000;

	SCE_DNAS1_PROT_BEGIN();
	ret = 4;
	b1 = buf;
	for (i = 0; i < 4; i++)
		b1[i] = p[i];
	v = *(unsigned int *)buf;
	n = (v >> 24) | ((v >> 8) & 0xff00) | ((v & 0xff00) << 8) | (v << 24);
	*(unsigned int *)buf = n;
	if (n == 0 || n > max) {
		ret = SCE_DNAS_ERR_BAD_RESPONSE;
	} else {
		unsigned char *q = (unsigned char *)io->downdata_size;
		for (k = 0; k < 4; k++)
			q[k] = buf[k];
	}
	SCE_DNAS1_PROT_END();
	return ret;
}

/* Copies *io->downdata_size bytes into io->downdata. */
int _sce_dnas1_set_downdata(unsigned char *p, sce_dnas1_io *io)
{
	int i;
	int n;
	unsigned char *q;

	SCE_DNAS1_PROT_BEGIN();
	i = 0;
	n = *io->downdata_size;
	q = io->downdata;
	if (n > 0) {
		do {
			q[i] = p[i];
			i++;
		} while (i < n);
	}
	SCE_DNAS1_PROT_END();
	return n;
}

/* Reads the big-endian ID request category through uid_buf. */
int _sce_dnas1_get_idreq_category(unsigned int *p)
{
	unsigned int v;

	SCE_DNAS1_PROT_BEGIN();
	v = *(unsigned int *)_sce_dnas1_common.uid_buf;
	*p = (v >> 24) | ((v >> 8) & 0xff00) | ((v & 0xff00) << 8) | (v << 24);
	SCE_DNAS1_PROT_END();
	return 4;
}

/* --- record ------------------------------------------------------------- */

/* Copies a 128-byte record from `src`. */
int _sce_dnas1_get_record_data(unsigned char *p, unsigned char *src)
{
	unsigned int i;

	SCE_DNAS1_PROT_BEGIN();
	for (i = 0; i < 128; i++)
		p[i] = src[i];
	SCE_DNAS1_PROT_END();
	return 128;
}

/* Stores the one-byte ID verdict. */
int _sce_dnas1_set_id_result(unsigned char *p)
{
	SCE_DNAS1_PROT_BEGIN();
	_sce_dnas1_common.id_result = *p;
	SCE_DNAS1_PROT_END();
	return 1;
}

/* Stores the one-byte record verdict. */
int _sce_dnas1_set_record_result(unsigned char *p)
{
	SCE_DNAS1_PROT_BEGIN();
	_sce_dnas1_common.record_result = *p;
	SCE_DNAS1_PROT_END();
	return 1;
}

/* --- ID / downdata sizes and buffers --- */

/* Stores the big-endian size at `p` in *io->id_size; SCE_DNAS_ERR_BAD_RESPONSE if it is 0 or
 * above 32 KB. */
int _sce_dnas1_set_id_size(unsigned char *p, sce_dnas1_io *io)
{
	unsigned char buf[4];
	unsigned int i, k;
	unsigned int n, v;
	int ret;
	unsigned char *b1;
	unsigned int max = 0x8000;

	SCE_DNAS1_PROT_BEGIN();
	ret = 4;
	b1 = buf;
	for (i = 0; i < 4; i++)
		b1[i] = p[i];
	v = *(unsigned int *)buf;
	n = (v >> 24) | ((v >> 8) & 0xff00) | ((v & 0xff00) << 8) | (v << 24);
	*(unsigned int *)buf = n;
	if (n == 0 || n > max) {
		ret = SCE_DNAS_ERR_BAD_RESPONSE;
	} else {
		unsigned char *q = (unsigned char *)io->id_size;
		for (k = 0; k < 4; k++)
			q[k] = buf[k];
	}
	SCE_DNAS1_PROT_END();
	return ret;
}

/* Copies *io->id_size bytes into io->id. */
int _sce_dnas1_set_id(unsigned char *p, sce_dnas1_io *io)
{
	int i;
	int n;
	unsigned char *q;

	SCE_DNAS1_PROT_BEGIN();
	i = 0;
	n = *io->id_size;
	q = io->id;
	if (n > 0) {
		do {
			q[i] = p[i];
			i++;
		} while (i < n);
	}
	SCE_DNAS1_PROT_END();
	return n;
}

/* --- error code translation --- */

/* Translates a one-byte server reason code into an error code in *out. */
int _sce_dnas1_set_ng_reason(unsigned char *p, int *out)
{
	unsigned char buf[4];
	unsigned int i;
	unsigned char *b1;

	SCE_DNAS1_PROT_BEGIN();
	b1 = buf;
	for (i = 0; i < 1; i++)
		b1[i] = p[i];
	switch (buf[0]) {
	case 0:
		*out = SCE_DNAS_ERR_EXTERNAL;
		break;
	case 1:
		*out = SCE_DNAS_ERR_BUSY;
		break;
	case 2:
		*out = SCE_DNAS_ERR_BEFORE_SERVICE;
		break;
	case 3:
		*out = SCE_DNAS_ERR_AFTER_SERVICE;
		break;
	case 4:
		*out = SCE_DNAS_ERR_SERVICE_FINISHED;
		break;
	case 5:
		*out = SCE_DNAS_ERR_TIMEOUT;
		break;
	case 9:
		*out = SCE_DNAS_ERR_ALL_OK;
		break;
	case 96:
		if (!(_sce_dnas1_common.mode & 1)) {
			*out = SCE_DNAS_ERR_DIFFERENT_HDD;
			_sce_dnas1_common.mode |= 1;
			break;
		}
		/* fall through */
	default:
		*out = SCE_DNAS_ERR_NG_BASE - buf[0];
		break;
	}
	SCE_DNAS1_PROT_END();
	return 1;
}
