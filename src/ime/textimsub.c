/*
 * IME panel rendering: GS/GIF packets for panes, gradients and textured
 * sprites, the 16 keyboard textures and the TIM2 loader, and a service thread
 * that keeps a cache of 24 textured symbols.
 */

#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <string.h>

#include "textim_debug.h"
#include "textim_arg.h"
#include "textimsub.h"
#include "gfx/tim2.h"
#include "textim_glob.h"

/* The 16 texture files (relative to /opt0/bn/data/skb/tex/), loaded by
 * textimsub_init. */
static const char *g_texFiles[16] = {
	"skb/skbBG.tm2",
	"skb/skbBtn.tm2",
	"skb/skbKey.tm2",
	"skb/fepbar.tm2",
	"skb/fepbar2.tm2",
	"skb/skbMdbar_fixed.tm2",
	"skb/tField.tm2",
	"help/skbHItem.tm2",
	"help/skbHJp.tm2",
	"help/skbHEng.tm2",
	"help/skbHFch.tm2",
	"help/skbHSp.tm2",
	"help/skbHGmn.tm2",
	"help/skbHDch.tm2",
	"help/skbHIta.tm2",
	"help/skbHPtg.tm2",
};

/* TIM2 pixel type to GS format (see TIM2_FORMAT), indexed by type - 1. */
static TIM2_FORMAT g_tim2Fmt[6] = {
	{ 2, 2, 1 },
	{ 1, 3, 1 },
	{ 0, 4, 1 },
	{ 0x14, 1, 2 },
	{ 0x13, 1, 1 },
	{ -1, 0, 0 },
};

static int g_stateB = 0;
static int g_flushCount = 0;
static int g_texHandle = -1;             /* fd of the raw symbol file */
static volatile int g_threadStop = 1;        /* stops the service thread */
static volatile int g_texsymArg = 0;
static pthread_t g_threadId = -1;          /* service thread id (pthread_join) */

/* Mutex of the symbol cache. */
static pthread_mutex_t g_texsymMutex = PTHREAD_MUTEX_INITIALIZER;


static int g_resHandles[16];
static int g_resExtra[16];


static int g_texsymTex;


/*
 * GS/GIF packet building: g_flushCount counts queued qwords (flushed at
 * 512), g_packetCursor is the write cursor in bytes (64 per packet).
 */


/* Symbol cache, published as a block by the service thread. */
typedef struct {
	int key[24];
} TexsymKeys;
typedef struct {
	unsigned char pix[3344];
} TexsymPix;  /* 304x22 at 4 bpp */
typedef struct {
	TexsymPix pix[24];
} TexsymTab;
static TexsymKeys g_texsymKey;
static TexsymTab g_texsymData;

static int g_packetBufs[2]; /* indexed by a boolean */
static int g_packetCursor;          /* packet write cursor, in bytes */
static int g_packetStart;
static int g_symFileSize;                /* count read from the resource */
static unsigned char g_alphaPalette[16][4]; /* alpha LUT, 16 levels (R = G = B = 255, A = i * 8) */


static __inline__ float textimsub_scale_x(void)
{
	return TextIM_arg.scaleX;
}
static __inline__ float textimsub_scale_y(void)
{
	return TextIM_arg.scaleY;
}

static __inline__ int tim2_psm(int type);
static __inline__ int textimsub_load_tim2(const char *path);
static __inline__ void textimsub_flush_now(void);
static __inline__ void textimsub_reserve(unsigned long long **pp, int nq);
static __inline__ void textimsub_put(unsigned long long **pp, unsigned long long v);
static __inline__ void texsym_copy_page(TexsymPix *dst, void *map, unsigned int off);
static void *textimsub_thread(void *arg);

static __inline__ int tim2_psm(int type)
{
	return g_tim2Fmt[type - 1].psm;
}

/* Same as textimsub_tim2, inlined in textimsub_init. */
static __inline__ int textimsub_load_tim2(const char *path)
{
	int fd, size, r;
	void *map;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		goto err;
	size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	if (size <= 0)
		goto err;
	map = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == (void *)-1) {
		close(fd);
err:
		return -1;
	}
	r = textimsub_parsetim2(map);
	munmap(map, size);
	close(fd);
	return r;
}

/* Body of textimsub_flush, inlined in reserve (when full) and at the start
 * of tex_use/tex_dir/texsym_use. */
static __inline__ void textimsub_flush_now(void)
{
	int n = g_flushCount;
	if (n > 0)
		TextIM_arg.flush(g_packetStart, n);
	{
		int v = g_packetCursor;
		g_flushCount = 0;
		g_packetStart = v;
	}
}

static __inline__ void textimsub_reserve(unsigned long long **pp, int nq)
{
	if ((unsigned int)(g_flushCount + nq) >= 512)
		textimsub_flush_now();
	if (pp)
		*pp = (unsigned long long *)g_packetCursor;
	g_packetCursor += nq * 16;
	g_flushCount += nq;
}

/* Appends one 64-bit packet entry. */
static __inline__ void textimsub_put(unsigned long long **pp, unsigned long long v)
{
	unsigned long long *p = *pp;
	*p++ = v;
	*pp = p;
}

/* Copies one symbol from the mapped page into its cache slot. */
static __inline__ void texsym_copy_page(TexsymPix *dst, void *map, unsigned int off)
{
	*dst = *(TexsymPix *)((char *)map + (off & (getpagesize() - 1)));
}

int textimsub_parsetim2(void *buf)
{
	TIM2_FILE_HEAD *fh;
	TIM2_PIC_HEAD *p;
	unsigned char *pixel;
	unsigned char *clut;
	int fmt, clutFmt, type, w, h, r;

	fh = (TIM2_FILE_HEAD *)buf;
	if (fh->fileId != TIM2_MAGIC)
		goto err;
	if (fh->formatVersion >= 5U)
		goto err;
	if (fh->formatId >= 2U)
		goto err;
	p = (TIM2_PIC_HEAD *)((unsigned char *)buf + (fh->formatId == 1 ? 128 : 16));
	type = p->imageType;
	if (type >= 6)
		goto err;
	if (type == 0)
		goto err;

	fmt = tim2_psm(type);
	pixel = (unsigned char *)p + p->headerSize;
	w = p->width;
	h = p->height;
	if (type != 4 && type != 5) {
		clut = 0;
		clutFmt = -1;
	} else {
		clutFmt = tim2_psm(p->clutType);
		clut = pixel + p->imageSize;
	}
	r = TextIM_arg.setImage(w, h, fmt, clutFmt, pixel, clut);
	if (r < 0)
		return -1;
	return r;
err:
	return -1;
}

/* Service thread: keeps the cache of the 24 textured symbols starting at
 * g_texsymArg - 8 ready. Each round, under the mutex, loads one symbol (from
 * the previous cache if present, otherwise by mapping its page of the raw
 * file) and publishes the local cache as a block when it is full. Stops when
 * g_threadStop is 1. */
static void *textimsub_thread(void *arg)
{
	struct {
		TexsymKeys keys;
		TexsymTab tab;
	} c;
	int lastBase;
	int lastArg;
	int n;
	int sym;
	int found;
	int i;
	unsigned int off;
	int len;
	void *map;

	lastBase = -1;
	g_threadStop = 0;
	lastArg = -1;
	n = 0;
	while (!g_threadStop) {
		pthread_mutex_lock(&g_texsymMutex);
		if (lastBase == g_texsymArg && lastArg == g_texsymArg) {
			pthread_mutex_unlock(&g_texsymMutex);
			usleep(100000);
			continue;
		}
		if (lastBase != g_texsymArg) {
			lastBase = g_texsymArg;
			n = 0;
		}
		sym = g_texsymArg + n - 8;
		if ((unsigned int)sym > 480) {
			TexsymPix *tp;

			c.keys.key[n] = -1;
			tp = c.tab.pix;
			memset(tp + n, 0, sizeof(TexsymPix));
		} else {
			found = -1;
			for (i = 0; i < 24; i++)
				if (sym == g_texsymKey.key[i])
					found = i;
			if (found < 0) {
				len = getpagesize() * 3;
				off = sym * sizeof(TexsymPix);
				map = mmap(0, len, PROT_READ, MAP_PRIVATE, g_texHandle, off & -getpagesize());
				if (map != (void *)-1) {
					c.keys.key[n] = sym;
					texsym_copy_page(&c.tab.pix[n], map, off);
					munmap(map, getpagesize() * 2);
				}
			} else {
				c.keys.key[n] = g_texsymKey.key[found];
				c.tab.pix[n] = g_texsymData.pix[found];
			}
		}
		n++;
		if (n >= 24) {
			g_texsymKey = c.keys;
			g_texsymData = c.tab;
			lastArg = g_texsymArg;
		}
		pthread_mutex_unlock(&g_texsymMutex);
	}
}

int textimsub_init(void)
{
	pthread_attr_t attr;
	char buf[528];
	int policy;
	struct sched_param param;
	int i, r, ok, pmax;
	int k;
	unsigned char *pal;
	unsigned char c;

	for (i = 0; i < 2; i++) {
		g_packetBufs[i] = TextIM_arg.mem.alloc(16, 0x8000);
		if (g_packetBufs[i] == 0) {
			if (i == 1)
				TextIM_arg.mem.free(g_packetBufs[0]);
			return 0;
		}
	}
	for (i = 0; i < 24; i++)
		g_texsymKey.key[i] = -1;

	for (i = 0; i < sizeof(g_texFiles) / sizeof(g_texFiles[0]); i++) {
		memcpy(buf, "/opt0/bn/data/skb/tex/", 23);
		strcat(buf, g_texFiles[i]);
		if (g_resHandles[i] == 0) {
			r = textimsub_load_tim2(buf);
			g_resExtra[i] = r;
			if (r < 0)
				return 0;
			g_resHandles[i] = 1;
		}
	}

	memcpy(buf, "/opt0/bn/data/skb/tex/", 23);
	strcat(buf, "skb/skbSym4.raw");
	/* Same as textimsub_raw_sym, with buf for the path. */
	if (g_texHandle >= 0)
		goto sym_pal;
	g_texHandle = open(buf, O_RDONLY);
	if (g_texHandle < 0) {
		ok = 0;
		goto sym_done;
	}
	g_symFileSize = lseek(g_texHandle, 0, SEEK_END);
	lseek(g_texHandle, 0, SEEK_SET);
	if (g_symFileSize <= 0) {
		close(g_texHandle);
		ok = 0;
		goto sym_done;
	}
sym_pal:
	k = 0;
	c = 255;
	pal = (unsigned char *)g_alphaPalette;
	for (; k < 16; k++) {
		pal[0] = c;
		pal[1] = c;
		pal[2] = c;
		pal[3] = (unsigned char)(k * 8);
		pal += 4;
	}
	ok = 1;
sym_done:
	if (ok == 0)
		return 0;

	g_stateB = 0;
	g_flushCount = 0;
	g_packetCursor = g_packetBufs[0];
	g_packetStart = g_packetBufs[0];
	pthread_mutex_init(&g_texsymMutex, 0);

	if (pthread_attr_init(&attr))
		return 0;
	pthread_attr_getschedpolicy(&attr, &policy);
	pthread_attr_getschedparam(&attr, &param);
	if (pthread_attr_setschedpolicy(&attr, 1) == 0) {
		pmax = sched_get_priority_max(1);
		if (pmax != -1) {
			param.sched_priority = pmax;
			if (pthread_attr_setschedparam(&attr, &param))
				return -1;
		}
		if (pthread_attr_setinheritsched(&attr, 1))
			return -1;
	}
	g_texsymTex = -1;
	g_texsymArg = 0;
	g_threadStop = 1;
	if (pthread_create(&g_threadId, &attr, textimsub_thread, 0))
		return -1;
	while (g_threadStop)
		usleep(30);
	return 1;
}

/* Full-screen black sprite (PRIM 6 = sprite, RGBAQ 0x80000000 = black with
 * alpha 0x80, two XYZ2). GIFtag REGLIST (FLG 1) NREG 4, REGS 0x5510 =
 * PRIM/RGBAQ/XYZ2/XYZ2. Coordinates are 12.4, packed into 64 bits: X in bits
 * 0-15, Y in bits 16-31. */
void textimsub_black(void)
{
	unsigned long long *p;
	int x0, y0, x1, y1;

	x0 = (2048 - (TextIM_arg.screen.maxX >> 1)) << 4;
	y0 = (2048 - ((TextIM_arg.screen.maxY - 120) >> (skb_interlace + 1))) << 4;
	textimsub_reserve(&p, 3);
	*p++ = 0x4400000000008001ULL;
	*p++ = 0x5510;
	*p++ = 6;
	*p++ = 0x80000000;
	*p++ = (unsigned long long)x0 | ((unsigned long long)y0 << 16);
	x1 = x0 + (TextIM_arg.screen.maxX << 4);
	y1 = y0 + (((TextIM_arg.screen.maxY - 120) << 4) >> skb_interlace);
	*p++ = (unsigned long long)x1 | ((unsigned long long)y1 << 16);
}

/* Filled rectangle (sprite with ABE: PRIM 0x46) of colour `color` in the
 * logical frame. Absolute coordinates start from the screen centre (2048)
 * minus half the usable area, in 12.4. */
void textimsub_pane(unsigned int color, int x, int y, int w, int h)
{
	unsigned long long *p;
	int x0, y0;

	x0 = (2048 - (TextIM_arg.screen.maxX >> 1)) << 4;
	y0 = (2048 - (TextIM_arg.screen.maxY >> (skb_interlace + 1))) << 4;
	textimsub_reserve(&p, 3);
	*p++ = 0x4400000000008001ULL;
	*p++ = 0x5510;
	*p++ = 70;
	*p++ = color;
	*p++ = (unsigned long long)(x0 + (x << 4)) | ((unsigned long long)(y0 + (y << 4)) << 16);
	*p++ = (unsigned long long)(x0 + ((x + w) << 4))
	    | ((unsigned long long)(y0 + ((y + h) << (skb_interlace + 4))) << 16);
}

/* Like textimsub_pane with coordinates already in 12.4 (sixteenths): no
 * `<< 4` on the offsets, and the interlace correction of the second vertex is
 * a right shift by skb_interlace instead of a left shift by skb_interlace+4. */
void textimsub_pane16(unsigned int color, int x, int y, int w, int h)
{
	unsigned long long *p;
	int x0, y0;

	x0 = (2048 - (TextIM_arg.screen.maxX >> 1)) << 4;
	y0 = (2048 - (TextIM_arg.screen.maxY >> (skb_interlace + 1))) << 4;
	textimsub_reserve(&p, 3);
	*p++ = 0x4400000000008001ULL;
	*p++ = 0x5510;
	*p++ = 70;
	*p++ = color;
	*p++ = (unsigned long long)(x0 + x) | ((unsigned long long)(y0 + y) << 16);
	*p++ = (unsigned long long)(x0 + (x + w)) | ((unsigned long long)(y0 + ((y + h) >> skb_interlace)) << 16);
}

/* Vertical gradient rectangle: PRIM 0x4C (triangle strip, Gouraud, ABE),
 * GIFtag REGLIST NREG 7 with REGS 0x05515510 =
 * PRIM,RGBAQ,XYZ2,XYZ2,RGBAQ,XYZ2,XYZ2: two strips, colour c0 at the top and
 * c1 at the bottom. Five qwords, the last entry a zero NOP. */
void textimsub_grad(unsigned int c0, unsigned int c1, int x, int y, int w, int h)
{
	unsigned long long *p;
	int x0, y0;

	x0 = (2048 - (TextIM_arg.screen.maxX >> 1)) << 4;
	y0 = (2048 - (TextIM_arg.screen.maxY >> (skb_interlace + 1))) << 4;
	textimsub_reserve(&p, 5);
	*p++ = 0x7400000000008001ULL;
	*p++ = 0x05515510;
	*p++ = 76;
	*p++ = c0;
	*p++ = (unsigned long long)(x0 + (x << 4)) | ((unsigned long long)(y0 + (y << 4)) << 16);
	*p++ = (unsigned long long)(x0 + ((x + w) << 4)) | ((unsigned long long)(y0 + (y << 4)) << 16);
	*p++ = c1;
	*p++ =
	    (unsigned long long)(x0 + (x << 4)) | ((unsigned long long)(y0 + ((y + h) << (skb_interlace + 4))) << 16);
	*p++ = (unsigned long long)(x0 + ((x + w) << 4))
	    | ((unsigned long long)(y0 + ((y + h) << (skb_interlace + 4))) << 16);
	*p++ = 0;
}

/* Textured sprite: PRIM 0x1D6 (sprite + TME + ABE + AA1 + FST), GIFtag
 * REGLIST NREG 6, REGS = PRIM,RGBAQ,UV,XYZ2,UV,XYZ2. UVs are
 * zero-extended u16, XYZ sign-extended ints. */
void textimsub_tex_dir(unsigned int color, TexSprite *dst, const TexUv *uv, const TexSprite *src)
{
	unsigned long long *p;
	int x0, y0;

	x0 = (2048 - (TextIM_arg.screen.maxX >> 1)) << 4;
	y0 = (2048 - (TextIM_arg.screen.maxY >> (skb_interlace + 1))) << 4;
	textimsub_reserve(&p, 4);
	*p++ = 0x6400000000008001ULL;
	*p++ = 0x00535310;
	*p++ = 470;
	*p++ = color;
	*p++ = (unsigned long long)uv->u | ((unsigned long long)uv->v << 16);
	*p++ = (unsigned long long)(x0 + dst->x) | ((unsigned long long)(y0 + (dst->y >> skb_interlace)) << 16);
	*p++ = (unsigned long long)(unsigned int)(uv->u + uv->w)
	    | ((unsigned long long)(unsigned int)(uv->v + uv->h) << 16);
	*p++ = (unsigned long long)(x0 + dst->x + dst->w)
	    | ((unsigned long long)(y0 + ((dst->y + dst->h) >> skb_interlace)) << 16);
}

/* Hit test of the pointer against the scaled rectangle `src`; returns 1 if
 * inside. */
int textimsub_mousecheck_opt(int baseX, int baseY, const TexSprite *src)
{
	TexSprite s;
	float v, rx, ry;
	int i, mx, my;

	s = *src;
	v = (float)baseX + (float)src->x * textimsub_scale_x();
	i = (int)(v + 0.5f);
	rx = v - (float)i;
	s.x = i;
	v = (float)baseY + (float)src->y * textimsub_scale_y();
	i = (int)(v + 0.5f);
	ry = v - (float)i;
	s.y = i;
	v = (float)s.w * textimsub_scale_x() + rx;
	s.w = (unsigned short)(int)(v + 0.5f);
	v = (float)s.h * textimsub_scale_y() + ry;
	s.h = (unsigned short)(int)(v + 0.5f);
	mx = skb_mouse_x();
	my = skb_mouse_y();
	return mx >= s.x && mx < s.x + s.w && my >= s.y && my < s.y + s.h;
}

/* Installs texture idx as the current source. PACKED packet (FLG 0) NREG 3,
 * REGS 0xE6E: TEXFLUSH, TEX0_1 (64-bit value computed by fn36 into a local
 * buffer) and TEX1_1. The flush first is unconditional: the texture register
 * cannot change in the middle of the queued packets. */
void textimsub_tex_use(int idx)
{
	unsigned long long tex0;
	unsigned long long *p;

	textimsub_flush_now();
	TextIM_arg.texUse(g_resExtra[idx], &tex0);
	textimsub_reserve(&p, 4);
	*p++ = 0x3000000000008001ULL;
	*p++ = 3694;
	*p++ = 0;
	*p++ = 63;
	*p++ = tex0;
	*p++ = 6;
	*p++ = 0;
	*p++ = 20;
}

/* Looks up symbol sym in the table, registers its texture and installs it as
 * the current source (same packet as textimsub_tex_use). Returns 1 if it drew,
 * 0 if the mutex is busy or the symbol is missing. */
int textimsub_texsym_use(int sym, int base)
{
	unsigned long long tex0;
	unsigned long long *p;
	int i;

	if (pthread_mutex_trylock(&g_texsymMutex)) {
		usleep(5);
		if (pthread_mutex_trylock(&g_texsymMutex))
			return 0;
	}
	g_texsymArg = base;
	if (g_texsymTex >= 0)
		TextIM_arg.texRelease(g_texsymTex);
	g_texsymTex = -1;
	for (i = 0; i < 24; i++) {
		if (g_texsymKey.key[i] == sym)
			g_texsymTex = TextIM_arg.setImage(304, 22, 20, 0, g_texsymData.pix[i].pix, g_alphaPalette);
	}
	if (g_texsymTex < 0) {
		pthread_mutex_unlock(&g_texsymMutex);
		return 0;
	}
	textimsub_flush_now();
	TextIM_arg.texUse(g_texsymTex, &tex0);
	textimsub_reserve(&p, 4);
	*p++ = 0x3000000000008001ULL;
	*p++ = 3694;
	*p++ = 0;
	*p++ = 63;
	*p++ = tex0;
	*p++ = 6;
	*p++ = 0;
	*p++ = 20;
	pthread_mutex_unlock(&g_texsymMutex);
	return 1;
}

void textimsub_exec_1(void)
{
	unsigned long long *p;

	textimsub_reserve(&p, 4);
	/* GIFtag NLOOP 1 EOP 1 NREG 3 REGS 0xEE8, then three A+D entries */
	textimsub_put(&p, 0x3000000000008001ULL);
	textimsub_put(&p, 3816);
	textimsub_put(&p, 0);
	textimsub_put(&p, 8);
	textimsub_put(&p, 68);
	textimsub_put(&p, 66);
	textimsub_put(&p, 0x3000d);
	textimsub_put(&p, 71);
}

void textimsub_shutdown(void)
{
	void *ret;
	int i;

	g_threadStop = 1;
	pthread_join(g_threadId, &ret);

	g_threadId = -1;
	pthread_mutex_lock(&g_texsymMutex);

	if (g_texsymTex >= 0) {
		TextIM_arg.texRelease(g_texsymTex);
		g_texsymTex = -1;
	}

	for (i = 0; i < 2; i++) {
		TextIM_arg.mem.free(g_packetBufs[i]);
		g_packetBufs[i] = 0;
	}

	if (g_texHandle >= 0) {
		close(g_texHandle);
		g_texHandle = -1;
	}
	pthread_mutex_unlock(&g_texsymMutex);
}

void textimsub_shutdown_noreuse(void)
{
	unsigned int i;

	pthread_mutex_lock(&g_texsymMutex);
	for (i = 0; i < 16; i++) {
		if (g_resHandles[i] != 0 && g_resExtra[i] >= 0)
			TextIM_arg.texRelease(g_resExtra[i]);
		g_resExtra[i] = -1;
		g_resHandles[i] = 0;
	}
	pthread_mutex_unlock(&g_texsymMutex);
}

void textimsub_tex_opt(unsigned int color, int baseX, int baseY, const TexSprite *src, const TexUv *uv)
{
	float v, rx, ry, sw, sh;
	int i, iw, ih;
	TexSprite buf;

	buf = *src;
	v = (float)baseX + (float)buf.x * TextIM_arg.scaleX;
	i = (int)(v + 0.5f);
	rx = v - (float)i;
	buf.x = i << 4;

	v = (float)baseY + (float)buf.y * TextIM_arg.scaleY;
	i = (int)(v + 0.5f);
	ry = v - (float)i;
	buf.y = i << 4;

	sw = (float)buf.w * TextIM_arg.scaleX;
	v = sw + rx;
	iw = (int)(v + 0.5f);

	sh = (float)buf.h * TextIM_arg.scaleY;
	v = sh + ry;
	ih = (int)(v + 0.5f);

	buf.w = (unsigned short)(iw << 4);
	buf.h = (unsigned short)(ih << 4);

	textimsub_tex_dir(color, &buf, uv, src);
}

void textimsub_calc_opt(const TexSprite *src, int *outX, int *outY, int *outX2, int *outY2)
{
	float v, rx, ry, sw, sh;
	int ix, iy, iw, ih;

	v = (float)src->x * TextIM_arg.scaleX;
	ix = (int)(v + 0.5f);
	*outX = ix;
	rx = v - (float)ix;

	v = (float)src->y * TextIM_arg.scaleY;
	iy = (int)(v + 0.5f);
	*outY = iy;

	sw = (float)src->w * TextIM_arg.scaleX;
	ry = v - (float)iy;
	v = sw + rx;
	iw = (int)(v + 0.5f);
	*outX2 = *outX + iw;

	sh = (float)src->h * TextIM_arg.scaleY;
	v = sh + ry;
	ih = (int)(v + 0.5f);
	*outY2 = *outY + ih;
}

/* Like mousecheck_opt but with the point given by the caller; also stores in
 * *outDist the squared distance from the centre of the rectangle. */
int textimsub_poscheck_opt(int baseX, int baseY, const TexSprite *src, int px, int py, int *outDist)
{
	TexSprite s;
	float v, rx, ry;
	int i, x, dx, dy;

	s = *src;
	v = (float)baseX + (float)src->x * TextIM_arg.scaleX;
	i = (int)(v + 0.5f);
	x = i;
	rx = v - (float)x;
	s.x = x;
	v = (float)baseY + (float)src->y * TextIM_arg.scaleY;
	i = (int)(v + 0.5f);
	ry = v - (float)i;
	s.y = i;
	v = (float)s.w * TextIM_arg.scaleX + rx;
	s.w = (unsigned short)(int)(v + 0.5f);
	v = (float)s.h * TextIM_arg.scaleY + ry;
	s.h = (unsigned short)(int)(v + 0.5f);
	dy = s.y + (s.h >> 1) - py;
	dx = s.x + (s.w >> 1) - px;
	*outDist = dx * dx + dy * dy;
	if (px >= s.x && px < s.x + s.w && py >= s.y && py < s.y + s.h)
		return 1;
	return 0;
}

void textimsub_flush(void)
{
	int v;
	int n = g_flushCount;
	if (n > 0) {
		TextIM_arg.flush(g_packetStart, n);
	}
	v = g_packetCursor;
	g_flushCount = 0;
	g_packetStart = v;
}

void textimsub_exec_2(void)
{
	int old, b, val;
	int n = g_flushCount;
	if (n > 0)
		TextIM_arg.flush(g_packetStart, n);

	old = g_stateB;
	g_flushCount = 0;
	b = (old == 0);
	val = g_packetBufs[b];
	g_stateB = b;
	g_packetCursor = val;
	g_packetStart = val;
}

void textimsub_sound(int id)
{
	textim_SendBackSoundMessage(id);
}

void textimsub_settim2(TIM2_PIC_HEAD *p)
{
	unsigned char *pixel;
	int fmt, clutFmt;
	unsigned char *clut;
	int w, h;

	pixel = (unsigned char *)p + p->headerSize;
	fmt = g_tim2Fmt[p->imageType - 1].psm;
	w = p->width;
	h = p->height;
	if (p->imageType != 4 && p->imageType != 5) {
		clut = 0;
		clutFmt = -1;
	} else {
		clutFmt = g_tim2Fmt[p->clutType - 1].psm;
		clut = pixel + p->imageSize;
	}
	TextIM_arg.setImage(w, h, fmt, clutFmt, pixel, clut);
}

/* Loads a TIM2 file by mapping it into memory and passes it to parsetim2. */
int textimsub_tim2(const char *path)
{
	int fd, size, r;
	void *map;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		goto err;
	size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	if (size <= 0)
		goto err;
	map = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == (void *)-1)
		goto err_close;
	r = textimsub_parsetim2(map);
	munmap(map, size);
	close(fd);
	return r;
err_close:
	close(fd);
err:
	return -1;
}

int textimsub_raw_sym(const char *path)
{
	int h, cnt, i;
	unsigned char *p;
	unsigned char c;

	h = g_texHandle;
	if (h < 0) {
		h = open(path, O_RDONLY);
		g_texHandle = h;
		if (h < 0)
			return 0;
		cnt = lseek(h, 0, SEEK_END);
		g_symFileSize = cnt;
		lseek(g_texHandle, 0, SEEK_SET);
		if (g_symFileSize <= 0) {
			close(g_texHandle);
			return 0;
		}
	}

	i = 0;
	c = 255;
	p = (unsigned char *)g_alphaPalette;
	for (; i < 16; i++) {
		p[0] = c;
		p[1] = c;
		p[2] = c;
		p[3] = (unsigned char)(i * 8);
		p += 4;
	}
	return 1;
}
