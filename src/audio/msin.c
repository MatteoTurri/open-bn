/*
 * sceMSIn_*: MIDI input queues. Each queue entry is a byte buffer that
 * collects channel messages, SysEx (0xF0..0xF7) and 0xF9/0xFD messages.
 */

#include "audio/msin.h"

int sceMSIn_Init(sceCslCtx *ctx)
{
	sceCslBuffGrp *grp;
	sceCslBuffCtx *items;
	int i;

	if (ctx != 0 && (grp = ctx->buffGrp, ctx->buffGrpNum >= 2) && grp != 0
	    && (items = grp[1].buffCtx, grp[1].buffNum != 0) && items != 0) {
		for (i = 0; i < grp[1].buffNum; i++) {
			sceCslMidiStream *stream = items[i].buff;
			if (stream == 0)
				return -1;
			if (stream->buffsize < 8)
				return -1;
		}
		return 0;
	}
	return -1;
}

int sceMSIn_ATick(void *ctx)
{
	return 0;
}

int sceMSIn_Load(void *ctx)
{
	return 0;
}

static int msin_put_common(sceCslCtx *ctx, int port, unsigned char *src, unsigned int len)
{
	sceCslBuffGrp *grp;
	sceCslMidiStream *stream;
	unsigned char *dst;

	if (ctx == 0)
		return -1;
	grp = &ctx->buffGrp[1];
	if ((unsigned int)port >= (unsigned int)grp->buffNum)
		return -1;
	stream = grp->buffCtx[port].buff;
	if (stream->buffsize < stream->validsize + len + 8)
		return -1;
	dst = stream->data + stream->validsize;
	stream->validsize += len;
	while (--len != (unsigned int)-1)
		*dst++ = *src++;
	return 0;
}

int sceMSIn_PutMsg(sceCslCtx *ctx, int port, unsigned int msg)
{
	int len;

	switch (msg & 0xF0) {
	case 0xD0:
	case 0xC0:
	case 0x80:
		len = 2;
		break;
	case 0xB0:
	case 0x90:
	case 0xA0:
	case 0xE0:
		len = 3;
		break;
	default:
		return -1;
	}
	return msin_put_common(ctx, port, (unsigned char *)&msg, len);
}

int sceMSIn_PutExcMsg(sceCslCtx *ctx, int port, unsigned char *buf, unsigned int maxlen)
{
	unsigned char *cursor = buf;
	int count;

	if (*cursor++ != 0xF0)
		return -1;

	count = 2;
	for (;;) {
		if ((unsigned int)count > maxlen)
			break;
		if (*cursor++ == 0xF7)
			return msin_put_common(ctx, port, buf, count);
		count++;
	}
	return -1;
}

int sceMSIn_PutHsMsg(sceCslCtx *ctx, int port, unsigned char *buf)
{
	int len;

	switch (*buf) {
	case 0xF9:
		len = 5;
		break;
	case 0xFD:
		len = 7;
		break;
	default:
		return -1;
	}
	return msin_put_common(ctx, port, buf, len);
}
