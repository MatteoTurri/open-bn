/*
 * gm_graph methods: the inline state setters, flush_state() and its writers
 * (GS registers, VU1 vectors, matrices, layers, textures) and the draw calls.
 */

#pragma implementation
#include <iostream.h>
#include <stdio.h>
#include <string.h>
#include "vec.h"
#include "gm_graph.h"
#include "tm.h"

#include "gm_texture.h"
#include "vecmath.h"
#include "tm2.h"
#include "tim2.h"
#include "bmp.h"


/* Identity matrix: the fallback of GetMatrix() for an invalid mode and the
   matrix sent to VU address 96. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3a(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */
/* Vector of a light that is off (write_matrices). */
static vec4 s_lightDefaultVec(0.0f, 0.0f, 0.0f, 0.0f);

/* VU1 microcode of the two built-in vertex shaders loaded by Init (VIF
   packets: NOP + MPG, then the code). */
static unsigned char gmVu1Prog0[0x3BD0] __attribute__((aligned(16))) = {
#include "../../assets/gm/vu1_prog0.inc"
};
static unsigned char gmVu1Prog1[0x3FD0] __attribute__((aligned(16))) = {
#include "../../assets/gm/vu1_prog1.inc"
};

/* Blend coefficients, indexed by the low nibble of fBlendSrc. */
/* clang-format off */
static signed char s_blendCoef[] = {
    10, 68, 72, 66, 74, -119, 70, 73, -127, 65,
};
/* clang-format on */

/* GS alpha test and depth test encodings, indexed by the compare function;
   [7] of the depth table is the default when depth test is off. */
/* clang-format off */
static signed char s_gsAlphaFuncTbl[8] = {
    0x01, 0x05, 0x09, 0x07, 0x0d, 0x0f, 0x0b, 0x03,  /* ATE | ATST<<1 */
};

static signed char s_gsDepthFuncTbl[8] = {
    0x01, 0x07, 0x05, 0x05, 0x07, 0x07, 0x05, 0x03,  /* ZTE | ZTST<<1 */
};
/* clang-format on */

/* Vertex stride table (gmVertexStride). */
static unsigned char gmStrideTbl[16] = { 0, 12, 12, 24, 4, 16, 16, 28, 4, 16, 16, 28, 8, 20, 20, 32 };

/* Per primitive kind (GS PRIM, 0 = POINT .. 6 = SPRITE) tables of DrawArrays
   and DrawParticle, used to build the GIF tag (fDrawTagHi) and the chunk
   overlap (fDrawChunkOverlap). */
static signed char gmKindPrimVerts[8] = { 0, 2, 0, 3, 0, -1, 2,
	0 }; /* DrawArrays: vertices per primitive of a list kind, -1 = fan, 0 = strip */
static signed char gmKindTagBits[8] = { 0, 1, 1, 6, 14, 5, 1, 0 }; /* ORed into the tag */
static signed char gmKindOverlap[8] = { 0, 0, 1, 2, 2, 2, 0, 0 }; /* vertices repeated across a chunk boundary */
static signed char gmParticlePrim[8] = { 6, 4, 4, 0, 0, 0, 4, 0 }; /* DrawParticle, shifted left by 15 into the tag */

/* Per-layer BlendFunc and ColorGen defaults of reset_state. */
static int gmResetBlend[4] = { GM_BLEND_ALPHA, GM_BLEND_ADD, GM_BLEND_OFF, GM_BLEND_OFF };
static int gmResetColorGen[4] = { 0, 1, 2, 2 };

/* Constants of the VU1 microprogram, written as words (GIF tags and register
   values mixed with floats): 1/3, 1/6, 2/3 (VU 7) and the two cubic Bezier
   basis matrices (VU 12 and 16). */
struct gmVuVec {
	unsigned int w[4];
};
struct gmVuMat {
	gmVuVec m0, m1, m2, m3;
};

static gmVuVec s_vuConst2 = { { 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff } };
static gmVuVec s_vuConst3 = { { 0x00008000, 0x3002c000, 0x00000412, 0x00000000 } };
static gmVuVec s_vuConst128 = { { 0x00008006, 0x10000000, 0x0000000e, 0x00000000 } };
static gmVuMat s_vuMat8 = {
	{ 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{ 0x00000000, 0x00000000, 0x00000001, 0x45600008 },
	{ 0x45600008, 0x00000000, 0x0000ffff, 0x45600008 },
	{ 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
};
static gmVuVec s_vuConst7 = { { 0x3eaaaaab, 0x3e2aaaab, 0x3f2aaaab, 0x3e2aaaab } };
/* clang-format off */
static gmVuMat s_vuMatPair[2] = {   /* VU 12 and 16 */
{
	{ 0xbf800000, 0x40400000, 0xc0400000, 0x3f800000 },
	{ 0x40400000, 0xc0c00000, 0x40400000, 0x00000000 },
	{ 0xc0400000, 0x40400000, 0x00000000, 0x00000000 },
	{ 0x3f800000, 0x00000000, 0x00000000, 0x00000000 }
},
{
	{ 0xc0400000, 0x41100000, 0xc1100000, 0x40400000 },
	{ 0x40c00000, 0xc1400000, 0x40c00000, 0x00000000 },
	{ 0xc03ffffc, 0x40400004, 0x358637bd, 0x358637bd },
	{ 0x3f800000, 0x00000000, 0x00000000, 0x00000000 }
},
};
/* clang-format on */

class img_file;
static img_file *gmCreateImageFile(const char *path);
static int gmReadImageFile(const char *path, int **buf, int *size);
static int gmLoadArchiveEntry(const char *path, const char *name, void **out, int *size);

/* Clear(mask) builds a GS packet by hand (GIF tag 0x50000007 + A+D
   registers) that draws a full-screen sprite with FRAME/TEST/RGBAQ/XYZ. GS
   registers are 64-bit (data, address) pairs. mask: bit 0 = leave the color
   buffer alone, bit 1 = TEST value, bit 2 = full color mask. */
struct gmGsReg {
	long long data;
	long long addr;
};

/* VIF UNPACK code: VU address (FLG = 0x8000, relative to TOPS), NUM in bits
   16..23, command (0x6X000000, USN and mask included) in the high bits. */
#define VIF_UNPACK(addr, num, cmd) ((addr) | 0x8000 | ((num) << 16) | (cmd))

/* Image loader factory: reads the file into memory and builds the loader from
   its first bytes ('TIM2' -> tm2_file, 'BM' in the low 16 bits -> bmp_file),
   which then loads the buffer [buf, buf + size / 4). */
/* Loads a file into a new buffer; returns its address and length, 0 on
   failure. */
static int gmReadImageFile(const char *path, int **buf, int *size);


/* Archive member extractor: opens `path`, reads the 52-byte header and the
   name table (entry `nameIndex`, right after the directory), then scans the
   `count` entries for `name`; when found, allocates the data rounded up to a
   multiple of 4, clears the last word and reads it. Returns true if a buffer
   was produced. */

struct gmArcHeader {
	/* 0x00 */ char pad_00[32];
	/* 0x20 */ int dirOffset;
	/* 0x24 */ char pad_24[10];
	/* 0x2E */ short entSize;
	/* 0x30 */ short count;
	/* 0x32 */ short nameIndex;
}; /* size = 0x34 */

struct gmArcEntry {
	/* 0x00 */ int nameOffset;
	/* 0x04 */ char pad_04[12];
	/* 0x10 */ int dataOffset;
	/* 0x14 */ int dataSize;
	/* 0x18 */ char pad_18[16];
}; /* size = 0x28 */

gm_graph::gm_graph()
{
	fState = 0;
	fFenceLo = 0x8000;
	fFenceHi = 1024;
	fPacketFailA = 0;
	fFenceRaw = 0;
	fPacketFailB = 0;
	fResHandle0 = 0;
	fResHandle1 = 0;
	fResHandle2 = 0;
}

inline gm_graph::~gm_graph()
{
	Term();
}

inline int gm_graph::Init(int dbufSize, int heapSize)
{
	if (fState)
		Term();
	fState |= 1;
	fPacket.init(dbufSize, heapSize);
	fDisp.init(0);
	fResHandle0 = CreateVertexShader((const int *)gmVu1Prog0, sizeof(gmVu1Prog0));
	fResHandle1 = CreateVertexShader((const int *)gmVu1Prog1, sizeof(gmVu1Prog1));
	reset_state();
	return 1;
}

inline void gm_graph::Term()
{
	if (!fState)
		return;
	fState = 0;
	DeleteVertexShader(fResHandle0);
	DeleteVertexShader(fResHandle1);
	fResHandle0 = 0;
	fResHandle1 = 0;
	fResHandle2 = 0;
	fDisp.term();
	fPacket.term();
}

inline void gm_graph::SetTextureManager(int v)
{
	gm_texture::g_texture_manager = v;
}

inline int gm_graph::GetTextureManager()
{
	return gm_texture::g_texture_manager;
}

inline void gm_graph::SetPacketFence(int size, int minFree)
{
	int v = (1023 < size) ? size : 1024;
	fFenceLo = v + 0x7C00;
	fFenceHi = v;
	fFenceRaw = minFree;
}

inline int gm_graph::GetPacketFailCount()
{
	int a = fPacketFailA;
	fPacketFailA = 0;
	int b = fPacketFailB;
	fPacketFailB = 0;
	return a + b;
}

inline void gm_graph::SetPixelOffset(float x, float y)
{
	fPixelOffXRaw = x;
	fPixelOffYRaw = y;
	fPixelOffXAdj = (float)fDisp.get_offset_x() + x - 0.5625f;
	fPixelOffYAdj = (float)fDisp.get_offset_y() + y - 0.5625f;
	fFlagsC |= 8;
}

inline void gm_graph::GetPixelOffset(float &x, float &y)
{
	x = fPixelOffXRaw;
	y = fPixelOffYRaw;
}

inline void gm_graph::Start()
{
	if (fState & 2)
		return;
	gm_window *d = &fDisp;
	fState |= 2;
	d->update();
	fPixelOffXAdj = (float)d->m_offsetX1 + fPixelOffXRaw - 0.5625f;
	float y = (float)d->m_offsetY1 + fPixelOffYRaw;
	fFlagsA = -1;
	fFlagsB = -1;
	y = y - 0.5625f;
	fFlagsC = -1;
	fLayerActiveMask = -1;
	fPixelOffYAdj = y;
	for (int i = 0; i < 4; i++) {
		SetLayer(i);
		SetTexture(0);
	}
	SetLayer(0);
	flush_state();
}

inline void gm_graph::Finish()
{
	if (fState & 2) {
		fState &= ~2;
		fFlagsA |= 1;
		flush_state();
		fPacket.send();
	}
}

inline void gm_graph::Flush()
{
	flush_state();
}

inline void gm_graph::SwapBuffers()
{
	if (fState & 2) {
		fState &= ~2;
		fFlagsA |= 1;
		Flush();
		fPacket.send();
	}
	fPacket.flip();
}

inline void gm_graph::DrawText(int, int, int, const char *)
{}

/* Stride of a vertex format; used by GetStride and by SetArrays for both the
   vertex and the weight format. */
static inline int gmVertexStride(int format)
{
	unsigned short stride = gmStrideTbl[format & 0xF];
	stride += (unsigned)(format & 0xF0) >> 1;
	stride += (unsigned)(format & 0xF00) >> 6;
	if (format & GM_VERTEX_16BIT)
		stride >>= 1;
	return stride;
}

template <class T> static inline T *gmVecAt(const vector<T *> &v, int i)
{
	return i < (int)v.size() ? v[i] : 0;
}

inline void gm_graph::ClearColor(const vec4 &c)
{
	fClearColor = c;
}

inline void gm_graph::ClearColor(float r, float g, float b, float a)
{
	const vec4 &c = vec4(r, g, b, a);
	fClearColor = c;
}

inline void gm_graph::ClearDepth(float v)
{
	fClearDepthVal = v;
}

inline void gm_graph::ClearStencil(int v)
{
	fClearStencilVal = v;
	fClearColor.w = v ? 1.0f : 0.0f;
}

inline void gm_graph::MatrixMode(int mode)
{
	if ((unsigned)mode < 8) {
		fCurMatrixMode = mode;
		fCurMatrixPtr = &fMatrixStack[mode];
	}
}

static inline mat4 gmRotXMat4(float a)
{
	float c = cosf(a);
	float s = sinf(a);
	return mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, s, 0.0f, 0.0f, -s, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

static inline mat4 gmRotYMat4(float a)
{
	float c = cosf(a);
	float s = sinf(a);
	return mat4(c, 0.0f, -s, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, s, 0.0f, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

static inline mat4 gmRotZMat4(float a)
{
	float c = cosf(a);
	float s = sinf(a);
	return mat4(c, s, 0.0f, 0.0f, -s, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

inline void gm_graph::PushMatrix()
{
	mat4_stack *e = fCurMatrixPtr;
	int i = 0;  /* redundant as written */
	do {
		int newDepth = e->fDepth + 1;
		if (e->fMaxDepth < newDepth)
			break;
		mat4 *b = e->fBase;
		b[newDepth] = b[e->fDepth];
		vec4 *d = e->fScales;
		d[newDepth] = d[e->fDepth];
		e->fDepth = newDepth;
	} while (--i >= 0);
}

inline void gm_graph::PushMatrix(int mode)
{
	if ((unsigned)mode < 8) {
		fCurMatrixMode = mode;
		fCurMatrixPtr = &fMatrixStack[mode];
	}
	mat4_stack *e = fCurMatrixPtr;
	int i = 0;  /* redundant as written */
	do {
		int newDepth = e->fDepth + 1;
		if (e->fMaxDepth < newDepth)
			break;
		mat4 *b = e->fBase;
		b[newDepth] = b[e->fDepth];
		vec4 *d = e->fScales;
		d[newDepth] = d[e->fDepth];
		e->fDepth = newDepth;
	} while (--i >= 0);
}

inline void gm_graph::PushMatrix(int mode, const mat4 &m)
{
	if ((unsigned)mode < 8) {
		fCurMatrixMode = mode;
		fCurMatrixPtr = &fMatrixStack[mode];
	}
	mat4_stack *e = fCurMatrixPtr;
	int i = 0;  /* redundant as written */
	do {
		int newDepth = e->fDepth + 1;
		if (e->fMaxDepth < newDepth)
			break;
		mat4 *b = e->fBase;
		b[newDepth] = b[e->fDepth];
		vec4 *d = e->fScales;
		d[newDepth] = d[e->fDepth];
		e->fDepth = newDepth;
	} while (--i >= 0);
	LoadMatrix(m);
}

inline void gm_graph::PopMatrix()
{
	mat4_stack *e = fCurMatrixPtr;
	int d = e->fDepth - 1;
	e->fDerivedValid = 0;
	if (d < 0)
		e->fDepth = 0;
	else
		e->fDepth = d;
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::PopMatrix(int mode)
{
	if ((unsigned)mode < 8) {
		fCurMatrixMode = mode;
		fCurMatrixPtr = &fMatrixStack[mode];
	}
	mat4_stack *e = fCurMatrixPtr;
	int d = e->fDepth - 1;
	e->fDerivedValid = 0;
	if (d < 0)
		e->fDepth = 0;
	else
		e->fDepth = d;
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::PushIdentity()
{
	mat4_stack *e = fCurMatrixPtr;
	int i = 0;  /* redundant as written */
	do {
		int newDepth = e->fDepth + 1;
		if (e->fMaxDepth < newDepth)
			break;
		mat4 *b = e->fBase;
		b[newDepth] = b[e->fDepth];
		vec4 *d = e->fScales;
		d[newDepth] = d[e->fDepth];
		e->fDepth = newDepth;
	} while (--i >= 0);
	LoadIdentity();
}

inline void gm_graph::PushIdentity(int mode)
{
	if ((unsigned)mode < 8) {
		fCurMatrixMode = mode;
		fCurMatrixPtr = &fMatrixStack[mode];
	}
	mat4_stack *e = fCurMatrixPtr;
	int i = 0;  /* redundant as written */
	do {
		int newDepth = e->fDepth + 1;
		if (e->fMaxDepth < newDepth)
			break;
		mat4 *b = e->fBase;
		b[newDepth] = b[e->fDepth];
		vec4 *d = e->fScales;
		d[newDepth] = d[e->fDepth];
		e->fDepth = newDepth;
	} while (--i >= 0);
	LoadIdentity();
}

inline mat4 *gm_graph::GetMatrix()
{
	mat4_stack *e = fCurMatrixPtr;
	return e->fBase + e->fDepth;
}

inline mat4 *gm_graph::GetMatrix(int mode)
{
	if ((unsigned)mode >= 8)
		return &s_unitMatrix;
	int off = mode * (int)sizeof(mat4_stack);
	mat4_stack *e = (mat4_stack *)((char *)this + off + 0x2A0);  /* &fMatrixStack[mode] */
	return e->fBase + e->fDepth;
}

inline void gm_graph::LoadIdentity()
{
	mat4_stack *e = fCurMatrixPtr;
	mat4 *m = (mat4 *)(e->fDepth * 64 + (int)e->fBase);
	m->m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	m->m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	m->m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	m->m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	(e->fScales)[e->fDepth].Set(1.0f, 1.0f, 1.0f, 1.0f);
	(e->fScales)[e->fDepth].w = 0.0f;
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::LoadIdentity(int mode)
{
	if ((unsigned)mode < 8) {
		fCurMatrixMode = mode;
		fCurMatrixPtr = &fMatrixStack[mode];
		mat4_stack *e = fCurMatrixPtr;
		mat4 *m = (mat4 *)(e->fDepth * 64 + (int)e->fBase);
		m->m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
		m->m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
		m->m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
		m->m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
		(e->fScales)[e->fDepth].Set(1.0f, 1.0f, 1.0f, 1.0f);
		(e->fScales)[e->fDepth].w = 0.0f;
		fFlagsC |= 1 << mode;
	}
}

inline void gm_graph::LoadMatrix(const mat4 &m)
{
	(fCurMatrixPtr)->load_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::LoadMatrix(int mode, const mat4 &m)
{
	/* An invalid mode loads nothing and leaves fFlagsC alone. */
	if ((unsigned)mode < 8) {
		fCurMatrixMode = mode;
		fCurMatrixPtr = &fMatrixStack[mode];
		(fCurMatrixPtr)->load_matrix(m);
		fFlagsC |= 1 << mode;
	}
}

inline void gm_graph::MultMatrix(const mat4 &m)
{
	(fCurMatrixPtr)->mult_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::MultMatrix(int mode, const mat4 &m)
{
	if ((unsigned)mode < 8) {
		fCurMatrixMode = mode;
		fCurMatrixPtr = &fMatrixStack[mode];
	}
	(fCurMatrixPtr)->mult_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

/* v.w is ignored. */
inline void gm_graph::Translate(const vec4 &v)
{
	mat4 m = mat4_translate(v.x, v.y, v.z);
	fCurMatrixPtr->mult_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::Translate(float x, float y, float z)
{
	float m[16];
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[11] = 0.0f;
	m[12] = x;
	m[13] = y;
	m[14] = z;
	m[0] = 1.0f;
	m[5] = 1.0f;
	m[10] = 1.0f;
	m[15] = 1.0f;
	(fCurMatrixPtr)->mult_matrix(*(const mat4 *)m);
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::RotateX(float angle)
{
	float c = cosf(angle);
	float s = sinf(angle);
	float m[16];
	m[6] = s;
	s = -s;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[5] = c;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[10] = c;
	m[11] = 0.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[0] = 1.0f;
	m[15] = 1.0f;
	m[9] = s;
	(fCurMatrixPtr)->mult_matrix(*(const mat4 *)m);
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::RotateY(float angle)
{
	float c = cosf(angle);
	float s = sinf(angle);
	float m[16];
	m[8] = s;
	s = -s;
	m[0] = c;
	m[1] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[9] = 0.0f;
	m[10] = c;
	m[11] = 0.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[5] = 1.0f;
	m[15] = 1.0f;
	m[2] = s;
	(fCurMatrixPtr)->mult_matrix(*(const mat4 *)m);
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::RotateZ(float angle)
{
	float c = cosf(angle);
	float s = sinf(angle);
	float m[16];
	m[1] = s;
	s = -s;
	m[0] = c;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[5] = c;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[11] = 0.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[10] = 1.0f;
	m[15] = 1.0f;
	m[4] = s;
	(fCurMatrixPtr)->mult_matrix(*(const mat4 *)m);
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::Scale(const vec4 &v)
{
	mat4 m = mat4_scale(v.x, v.y, v.z);
	fCurMatrixPtr->mult_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::Scale(float x, float y, float z)
{
	float m[16];
	m[0] = x;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[5] = y;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[10] = z;
	m[11] = 0.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = 0.0f;
	m[15] = 1.0f;
	(fCurMatrixPtr)->mult_matrix(*(const mat4 *)m);
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::Perspective(float fovy, float aspect, float near, float far)
{
	float cot = 1.0f / tanf(fovy * 0.5f);
	float diff = near - far;
	float a = (far + near) / diff;
	float b = (2.0f * far * near) / diff;
	float m[16];
	m[0] = cot / aspect;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[5] = cot;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[10] = a;
	m[11] = -1.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = b;
	m[15] = 0.0f;
	(fCurMatrixPtr)->mult_matrix(*(const mat4 *)m);
	fFlagsC |= 1 << fCurMatrixMode;
}

/* Same matrix as glFrustum. */
inline void gm_graph::Frustum(float left, float right, float bottom, float top, float near, float far)
{
	float rl = 1.0f / (right - left);
	float tb = 1.0f / (top - bottom);
	float fn = 1.0f / (far - near);
	float m[16];
	m[0] = 2.0f * near * rl;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[5] = 2.0f * near * tb;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = (right + left) * rl;
	m[9] = (top + bottom) * tb;
	m[10] = -(far + near) * fn;
	m[11] = -1.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = -2.0f * far * near * fn;
	m[15] = 0.0f;
	(fCurMatrixPtr)->mult_matrix(*(const mat4 *)m);
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::Ortho(float left, float right, float bottom, float top, float near, float far)
{
	float rl = 1.0f / (right - left);
	float tb = 1.0f / (top - bottom);
	float fn = 1.0f / (far - near);
	float m[16];
	m[0] = 2.0f * rl;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[5] = 2.0f * tb;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[10] = -2.0f * fn;
	m[11] = 0.0f;
	m[12] = -(right + left) * rl;
	m[13] = -(top + bottom) * tb;
	m[14] = -(far + near) * fn;
	m[15] = 1.0f;
	(fCurMatrixPtr)->mult_matrix(*(const mat4 *)m);
	fFlagsC |= 1 << fCurMatrixMode;
}

inline void gm_graph::Viewport(float x, float y, float w, float h)
{
	Viewport(vec4(x, y, w, h));
}

/* Viewport over the color buffer, or over the window when there is none
   (fColorBufferId -1 or 0). */
inline void gm_graph::Viewport()
{
	int w, h;
	int id = fColorBufferId;
	if ((unsigned)(id + 1) < 2) {
		const gm_window &d = fDisp;
		w = d.m_width1;
		h = d.m_height1;
	} else {
		w = tmGetWidth(id);
		h = tmGetHeight(id);
	}
	Viewport(vec4(0.0f, 0.0f, (float)w, (float)h));
}

/* write_vectors writes the state VECTORS of the VU1 microprogram as unpack
   records of 32 bytes (vec4) or 80 bytes (mat4): two header words (command,
   VU address) and the data at +16. */
static inline int *gmPutV4(int *dst, int cmd, int addr, const vec4 &v)
{
	dst[0] = cmd;
	dst[1] = addr;
	vec4 *d = (vec4 *)(dst + 4);
	*d = v;
	return dst + 8;
}

static inline int *gmPutM4(int *dst, int cmd, int addr, const mat4 &m)
{
	dst[0] = cmd;
	dst[1] = addr;
	mat4 *d = (mat4 *)(dst + 4);
	*d = m;
	return (int *)(d + 1);
}

/* write_matrices writes the VU1 MATRIX records, driven by fFlagsC:
     bits 0..2   fMatrixStack[0..2] at VU addresses 64/68/72; bit 1 adds a
                 vec4 (address 42) with -T * R, the inverse translation;
     bit 3       two vec4 (addresses 40 and 41) with the projection, fog and
                 pixel offset coefficients taken from fMatrixStack[3];
     bits 4..7   fMatrixStack[4..7] at addresses 137, 153, 169, 185;
     bits 28..31 the four lights: a vec4 (80 + i) and a mat4 (192 + 4 * k)
                 each, updating the transposed direction cache, which goes
                 to VU 76 at the end.
   Ends with the termination word 3 if any of bits 0..3 or 28..31 was set. */
static inline const mat4 *gmTop(const mat4_stack *e)
{
	return e->fBase + e->fDepth;
}

static inline vec4 gmNorm(const vec4 &v)
{
	float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	if (len == 0.0f)
		return v;
	len = 1.0 / len;
	return vec4(v.x * len, v.y * len, v.z * len, v.w);
}

/* Rotation about an axis through a quaternion:
     m = quat_to_mat(normalize(axis) * sin(angle / 2), cos(angle / 2)) */
static inline mat4 gmAxisAngleMat4(float angle, const vec4 &axis)
{
	float h = angle * 0.5f;
	vec4 q = gmNorm(axis) * sinf(h);
	float qx = q.x;
	float qy = q.y;
	float qz = q.z;
	float c = cosf(h);
	float x2 = qx + qx;
	float y2 = qy + qy;
	float z2 = qz + qz;
	float w2 = c + c;
	return mat4(1.0f - y2 * qy - z2 * qz, x2 * qy + w2 * qz, z2 * qx - w2 * qy, 0.0f, x2 * qy - w2 * qz,
	    1.0f - x2 * qx - z2 * qz, y2 * qz + w2 * qx, 0.0f, z2 * qx + w2 * qy, y2 * qz - w2 * qx,
	    1.0f - x2 * qx - y2 * qy, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

static inline mat4 gmAxisAngleMat4(float angle, float x, float y, float z)
{
	vec4 axis(x, y, z, 1.0f);
	return gmAxisAngleMat4(angle, axis);
}

/* Same body as mat4_look_at() of vecmath.cpp, expanded inline by both LookAt
   overloads. */
static inline mat4 gmLookAt(const vec4 &eye, const vec4 &center, const vec4 &up)
{
	vec4 f = gmNorm(center - eye);
	vec4 u = gmNorm(up);
	vec4 s = gmNorm(cross(f, u));
	u = cross(s, f);
	return mat4(s.x, u.x, -f.x, 0.0f, s.y, u.y, -f.y, 0.0f, s.z, u.z, -f.z, 0.0f,
	    -(eye.x * s.x + eye.y * s.y + eye.z * s.z), -(eye.x * u.x + eye.y * u.y + eye.z * u.z),
	    eye.x * f.x + eye.y * f.y + eye.z * f.z, 1.0f);
}

/* Scalar overload: builds the three vectors and calls the vec4 version. */
static inline mat4 gmLookAt(float ex, float ey, float ez, float cx, float cy, float cz, float ux, float uy, float uz)
{
	vec4 e(ex, ey, ez, 1.0f);
	vec4 c(cx, cy, cz, 1.0f);
	vec4 u(ux, uy, uz, 1.0f);
	return gmLookAt(e, c, u);
}

void gm_graph::Clear(int mask)
{
	flush_state();

	gm_packet *pk = &fPacket;
	char *limit = (char *)((int)pk->fTagCursor & ~0xFFF);
	char *cursor = pk->fCursor;
	if (limit - cursor < fFenceLo) {
		fPacketFailA++;
		return;
	}

	int *hdr = (int *)cursor;
	hdr[2] = 0x11000000;
	hdr[0] = 0;
	hdr[1] = 0;
	hdr[3] = 0x50000007;
	hdr[4] = 0x8001;
	int *p = hdr + 4;
	p[1] = 0x60034000;
	p[2] = 0x00441eee;
	p[3] = 0;

	int noColor = (mask ^ 1) & 1;
	gm_window *dp = &fDisp;
	p = hdr + 8;
	p[2] = 127;
	if (noColor != 0) {
		int target;
		int id = fColorBufferId;
		if ((unsigned)(id + 1) < 2)
			(dp)->get_gs_frame(&target);
		else {
			((gm_texture *)id)->send();
			tmGetGsFRAME(id, (unsigned int *)&target);
		}
		int m = 0x80000000;
		if ((mask & 4) == 0)
			m = 0;
		p[0] = target;
		p[1] = ~m;
		p[2] = 76;
		fFlagsA |= 2;
	}

	gm_packet *packet = &fPacket;
	p += 4;
	gmGsReg *g = (gmGsReg *)p;
	g[0].data = 0;
	g[0].addr = 27;
	g[1].data = ((long long)(((unsigned)mask >> 1) & 1) << 16) | 0x20000;
	g[1].addr = 71;

	p += 8;
	p[0] = (int)(fClearColor.x * 255.0f);
	p[1] = (int)(fClearColor.y * 255.0f);
	p[2] = (int)(fClearColor.z * 255.0f);
	p[3] = (int)(fClearColor.w * 128.0f);

	float x = (float)dp->m_offsetX1;
	float y = (float)dp->m_offsetY1;
	int depth = (int)(((float)((1 << dp->m_zbufAlign) - 1) * (1.0f - fClearDepthVal)) * 16.0f);
	x = x + fScissor.x;
	y = y + fScissor.y;

	p += 4;
	p[3] = 0;
	p[0] = (int)(x * 16.0f);
	p[1] = (int)(y * 16.0f);
	p[2] = depth;

	p += 4;
	p[0] = (int)((x + fScissor.z) * 16.0f);
	char *tail = (char *)(p + 4);
	int rem = (int)tail & 0xc;
	p[1] = (int)((y + fScissor.w) * 16.0f);
	p[2] = depth;
	p[3] = 0;

	if (rem != 0) {
		while (rem < 16) {
			*(int *)tail = 0;
			tail += 4;
			rem += 4;
		}
	}
	int size = tail - packet->fCursor;
	packet->fCursor = tail;
	fPacket.put_packet((int *)cursor, size);
	fFlagsA |= 0x1c00000;
}

void gm_graph::Rotate(float angle, const vec4 &axis)
{
	mat4 m = gmAxisAngleMat4(angle, axis);
	(fCurMatrixPtr)->mult_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

void gm_graph::Rotate(float angle, float x, float y, float z)
{
	mat4 m = gmAxisAngleMat4(angle, x, y, z);
	(fCurMatrixPtr)->mult_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

void gm_graph::RotateYXZ(const vec4 &v)
{
	mat4 m = gmRotYMat4(v.y) * gmRotXMat4(v.x) * gmRotZMat4(v.z);
	(fCurMatrixPtr)->mult_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

void gm_graph::RotateYXZ(float x, float y, float z)
{
	mat4 m = gmRotYMat4(y) * gmRotXMat4(x) * gmRotZMat4(z);
	(fCurMatrixPtr)->mult_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

void gm_graph::RotateZYX(const vec4 &v)
{
	mat4 m = gmRotZMat4(v.z) * gmRotYMat4(v.y) * gmRotXMat4(v.x);
	(fCurMatrixPtr)->mult_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

void gm_graph::RotateZYX(float x, float y, float z)
{
	mat4 m = gmRotZMat4(z) * gmRotYMat4(y) * gmRotXMat4(x);
	(fCurMatrixPtr)->mult_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

void gm_graph::LookAt(const vec4 &eye, const vec4 &center, const vec4 &up)
{
	mat4 m = gmLookAt(eye, center, up);
	(fCurMatrixPtr)->mult_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

void gm_graph::LookAt(float ex, float ey, float ez, float cx, float cy, float cz, float ux, float uy, float uz)
{
	mat4 m = gmLookAt(ex, ey, ez, cx, cy, cz, ux, uy, uz);
	(fCurMatrixPtr)->mult_matrix(m);
	fFlagsC |= 1 << fCurMatrixMode;
}

/* fDepthRange[0..3] caches the viewport (x, y, w, h) and [4]/[5] hold near/far
   of DepthRange. Loads the DepthRange matrix into matrix mode 3, then sets
   the scissor to the viewport. */
void gm_graph::Viewport(const vec4 &v)
{
	float *d = fDepthRange;
	*(vec4 *)d = v;
	float x = d[0];
	float y = d[1];
	float w = d[2];
	float h = d[3];
	float sz = (d[5] - d[4]) * 0.5f;
	float tz = (d[5] + d[4]) * 0.5f;
	float sx = w * 0.5f;
	float sy = h * -0.5f;
	float ox = x + sx;
	float oy = y - sy;
	float m[16];
	m[0] = sx;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[5] = sy;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[10] = sz;
	m[11] = 0.0f;
	m[12] = ox;
	m[13] = oy;
	m[14] = tz;
	m[15] = 1.0f;
	fCurMatrixMode = 3;
	fCurMatrixPtr = &fMatrixStack[3];
	(fCurMatrixPtr)->load_matrix(m);
	fFlagsC |= 1 << 3;
	Scissor(v);
}

inline void gm_graph::Scissor(const vec4 &c)
{
	fScissor = c;
	fFlagsA |= 4;
}

inline void gm_graph::Scissor(float x, float y, float w, float h)
{
	const vec4 &c = vec4(x, y, w, h);
	fFlagsA |= 4;
	fScissor = c;
}

inline void gm_graph::DepthRange(float n, float f)
{
	float *d = fDepthRange;
	d[4] = n;
	d[5] = f;
	float sz = (d[5] - d[4]) * 0.5f;
	float tz = (d[5] + d[4]) * 0.5f;
	float sx = d[2] * 0.5f;
	float sy = d[3] * -0.5f;
	float m[16];
	m[0] = sx;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[5] = sy;
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = 0.0f;
	m[9] = 0.0f;
	m[10] = sz;
	m[11] = 0.0f;
	m[12] = d[0] + sx;
	m[13] = d[1] - sy;
	m[14] = tz;
	m[15] = 1.0f;
	(&fMatrixStack[3])->load_matrix(m);
	fFlagsC |= 8;
}

inline void gm_graph::Fog(int mode, const vec4 &color, float near, float far)
{
	FogMode(mode);
	FogColor(color);
	FogRange(near, far);
}

inline void gm_graph::FogMode(int mode)
{
	fFogMode = mode;
}

inline void gm_graph::FogColor(const vec4 &c)
{
	unsigned newMask = fLayerActiveMask | (fLayerEnableMask & 0x00F00000);
	float rf = c.x * 255.0f;
	float gf = c.y * 255.0f;
	float bf = c.z * 255.0f;
	fLayerActiveMask = newMask;
	int ri = (int)rf;
	int gi = (int)gf;
	int bi = (int)bf;
	fFogColorPacked = (ri & 0xFF) | ((gi & 0xFF) << 8) | ((bi & 0xFF) << 16);
}

inline void gm_graph::FogColor(float r, float g, float b, float a)
{
	vec4 c(r, g, b, a);
	unsigned newMask = fLayerActiveMask | (fLayerEnableMask & 0x00F00000);
	float rf = c.x * 255.0f;
	float gf = g * 255.0f;
	float bf = b * 255.0f;
	fLayerActiveMask = newMask;
	int ri = (int)rf;
	int gi = (int)gf;
	int bi = (int)bf;
	fFogColorPacked = (ri & 0xFF) | ((gi & 0xFF) << 8) | ((bi & 0xFF) << 16);
}

inline void gm_graph::FogRange(float n, float f)
{
	fFogNear = n;
	fFogFar = f;
	fFlagsC |= 8;
}

inline void gm_graph::Ambient(const vec4 &c)
{
	fAmbient = c;
	fFlagsB |= 0x40000;
}

inline void gm_graph::Ambient(float r, float g, float b, float a)
{
	const vec4 &c = vec4(r, g, b, a);
	fFlagsB |= 0x40000;
	fAmbient = c;
}

inline void gm_graph::LightRange(int idx, float range, float constant, float linear, float quadratic)
{
	if ((unsigned)idx < 4) {
		gm_light *l = &fLights[idx];
		float *p = &l->fParams.m1.x;
		p[0] = range * range;
		p[1] = constant;
		p[2] = linear;
		p[3] = quadratic;
		fFlagsC |= 0x10000000 << idx;
	}
}

inline void gm_graph::LightCone(int idx, float innerAngle, float ratio)
{
	if ((unsigned)idx < 4) {
		if (innerAngle < 0.01f)
			innerAngle = 0.01f;
		if (innerAngle > 3.14f)
			innerAngle = 3.14f;
		innerAngle = cosf(innerAngle);
		ratio = ratio / 3.0f;
		if (ratio < 1.0f)
			ratio = 1.0f;
		gm_light *l = &fLights[idx];
		l->fParams.m0.x = -ratio / (1.0f - innerAngle);
		l->fParams.m0.y = (-ratio * innerAngle) / (1.0f - innerAngle) + (1.0f - ratio);
		fFlagsC |= 0x10000000 << idx;
	}
}

inline void gm_graph::EnableLight(int idx, bool on)
{
	if ((unsigned)idx < 4) {
		int bit = GM_ENABLE_LIGHT0 << idx;
		int flags = on ? (fFlagsE | bit) : (fFlagsE & ~bit);
		SetEnableBits(flags);
	}
}

inline void gm_graph::DisableLight(int idx, bool clearFlag)
{
	if ((unsigned)idx < 4) {
		int bit = GM_ENABLE_LIGHT0 << idx;
		bool notClear = !clearFlag;
		int flags = notClear ? (fFlagsE | bit) : (fFlagsE & ~bit);
		SetEnableBits(flags);
	}
}

inline void gm_graph::Color(const vec4 &c)
{
	fColor = c;
	fFlagsB |= 0x80000;
}

inline void gm_graph::Color(float r, float g, float b, float a)
{
	const vec4 &c = vec4(r, g, b, a);
	fFlagsB |= 0x80000;
	fColor = c;
}

inline void gm_graph::Diffuse(const vec4 &c)
{
	fDiffuse = c;
	fFlagsB |= 0x100000;
}

inline void gm_graph::Diffuse(float r, float g, float b, float a)
{
	const vec4 &c = vec4(r, g, b, a);
	fFlagsB |= 0x100000;
	fDiffuse = c;
}

inline void gm_graph::Specular(const vec4 &c, float exp)
{
	exp = exp / 6.0f;
	if (exp < 1.0f)
		exp = 1.0f;
	fSpecular = c;
	fSpecularExtraA = exp;
	fSpecularExtraB = 1.0f - exp;
	fFlagsB |= 0x200000;
	int mask = 0;
	if (c.w != 0.0f) {
		if (c.x + c.y + c.z != 0.0f)
			mask = 15;
	}
	if (fSpecularMask != mask) {
		fSpecularMask = mask;
		fFlagsB |= 0x80000000;
	}
}

inline void gm_graph::Specular(float r, float g, float b, float a, float exp)
{
	const vec4 &c = vec4(r, g, b, a);
	exp = exp / 6.0f;
	if (exp < 1.0f)
		exp = 1.0f;
	fSpecularExtraA = exp;
	fSpecular = c;
	fSpecularExtraB = 1.0f - exp;
	fFlagsB |= 0x200000;
	int mask = 0;
	if (a != 0.0f) {
		if (c.x + g + b != 0.0f)
			mask = 15;
	}
	if (fSpecularMask != mask) {
		fSpecularMask = mask;
		fFlagsB |= 0x80000000;
	}
}

inline void gm_graph::Emission(const vec4 &c)
{
	fEmission = c;
	fFlagsB |= 0x400000;
}

inline void gm_graph::Emission(float r, float g, float b, float a)
{
	const vec4 &c = vec4(r, g, b, a);
	fFlagsB |= 0x400000;
	fEmission = c;
}

inline void gm_graph::TexFactor(const vec4 &c, float bias, float bias2)
{
	float negBias2 = -bias2;
	fTexFactor = c;
	fTexFactorBias = bias;
	fTexFactorBias2 = negBias2;
	fFlagsB |= 0x800000;
}

inline void gm_graph::TexFactor(float r, float g, float b, float a, float bias, float bias2)
{
	const vec4 &c = vec4(r, g, b, a);
	fTexFactorBias = bias;
	fFlagsB |= 0x800000;
	fTexFactor = c;
	fTexFactorBias2 = -bias2;
}

inline int gm_graph::CreateTexture(const char *name)
{
	int *data;
	int size;
	if (!gmReadImageFile(name, &data, &size))
		return 0;
	int handle = CreateTexture(data, size);
	if (data)
		delete[] data;
	return handle;
}

void gm_graph::Light(int idx, int type, const vec4 &a, const vec4 &b)
{
	if ((unsigned)idx < 4) {
		gm_light *L = &fLights[idx];
		L->fType = type;
		L->fColor = a;
		fFlagsC |= 0x10000000 << idx;
		if (type == GM_LIGHT_DIRECTIONAL) {
			L->fParams.m3 = gmNorm(b);
			fPointLightMask &= ~(0x10000000 << idx);
		} else {
			L->fParams.m2 = b;
			fPointLightMask |= 0x10000000 << idx;
		}
		int m = fPointLightMask & fFlagsE;
		int cur = fVshFlags;
		int prev = cur;
		if (m)
			cur |= 8;
		else
			cur &= ~8;
		if (prev != cur) {
			fVshFlags = cur;
			fFlagsB |= 0xdf;
		}
	}
}

void gm_graph::Light(int idx, int type, const vec4 &a, const vec4 &b, const vec4 &c)
{
	if ((unsigned)idx < 4) {
		gm_light *L = &fLights[idx];
		L->fType = type;
		L->fColor = a;
		L->fParams.m2 = b;
		L->fParams.m3 = gmNorm(c);
		unsigned bit = 0x10000000 << idx;
		fFlagsC |= bit;
		if (type == GM_LIGHT_DIRECTIONAL)
			fPointLightMask &= ~bit;
		else
			fPointLightMask |= bit;
		int m = fPointLightMask & fFlagsE;
		int cur = fVshFlags;
		int prev = cur;
		if (m)
			cur |= 8;
		else
			cur &= ~8;
		if (prev != cur) {
			fVshFlags = cur;
			fFlagsB |= 0xdf;
		}
	}
}

/* High word (bits 32-63) of the GIFtag of a primitive list: PRE (bit 46),
   PRIM (47-57) and NREG (60-63) as in SCE_GIF_SET_TAG, and the primitive
   kind in bits 36-38, which the GIF ignores; the caller ORs gmKindTagBits
   into bits 32-35. All callers pass PRE = 1, NREG = 3. */
static inline int gm_giftag_hi(int pre, int prim, int nreg, int kind)
{
	return (prim << 15) | ((kind << 4) | (pre << 14) | (nreg << 28));
}

/* gmKindPrimVerts[kind]: 0 plain, negative triangle fan, positive rederives
   stride and chunk limit (one primitive per stride). */
inline void gm_draw_arrays(gm_graph *g, int kind, int start, int stride, int chunkLimit, const short *indices)
{
	g->flush_state();
	int tag = gm_giftag_hi(1, kind, 3, kind);
	g->fDrawMode = 0;
	int tagBits = gmKindTagBits[kind];
	g->fDrawChunkCap = (g->fArrayFlag > 1) ? 18 : 36;
	g->fDrawTagHi = tag | tagBits;
	g->fDrawChunkOverlap = gmKindOverlap[kind];
	int primVerts = gmKindPrimVerts[kind];
	if (primVerts != 0) {
		if (primVerts >= 0) {
			int q = stride / primVerts;
			stride = primVerts;
			chunkLimit *= q;
		} else {
			g->draw_trifan(start, stride, chunkLimit, indices);
			return;
		}
	}
	if (!indices)
		g->send_arrays_linear(kind, start, stride, chunkLimit);
	else
		g->send_arrays_indexed(kind, indices + start, stride, chunkLimit);
}

/* Detects TIM2 or BMP from the magic and creates the texture from the first
   image and the first CLUT of the first picture. */
int gm_graph::CreateTexture(const void *data, int size)
{
	img_file *ld;
	if (*(const int *)data == TIM2_MAGIC) {
		ld = new tm2_file;
		ld->load((int *)data, (int *)((const char *)data + (size / 4) * 4));
	} else if ((*(const int *)data & 0xffff) == 0x4d42) {
		ld = new bmp_file;
		ld->load((int *)data, (int *)((const char *)data + (size / 4) * 4));
	} else
		return 0;

	int id = 0;

	img_pict *set = gmVecAt(ld->m_picts, 0);
	if (set) {
		img_image *img = gmVecAt(set->m_images, 0);
		img_image *clut = gmVecAt(set->m_cluts, 0);
		if (img) {
			if (!clut) {
				const void *d = img->m_buffer;
				int fmt = img->get_gs_format();
				int w = img->m_width;
				int h = img->m_height;
				id = CreateTexture(d, fmt, w, h, 0, 0, 0);
			} else {
				const void *d = img->m_buffer;
				int fmt = img->get_gs_format();
				int w = img->m_width;
				int h = img->m_height;
				const void *cd = clut->m_buffer;
				int cfmt = clut->get_gs_format();
				id = CreateTexture(d, fmt, w, h, cd, cfmt, 0);
			}
		}
	}
	delete ld;
	return id;
}

/* Loads the first image and the first CLUT of the first picture of an image
   file into texture `id`. */
int gm_graph::LoadTexture(int id, const char *name)
{
	if (id == 0)
		return 1;
	img_file *ld = gmCreateImageFile(name);
	if (!ld)
		return 0;
	int ok = 1;
	img_pict *set = gmVecAt(ld->m_picts, 0);
	if (set) {
		img_image *img = gmVecAt(set->m_images, 0);
		img_image *clut = gmVecAt(set->m_cluts, 0);
		if (img) {
			if (!LoadTextureImage(id, img->m_buffer))
				ok = 0;
		}
		if (clut) {
			/* NOTE: bug: the CLUT is loaded from the image data, not from clut->m_buffer. */
			if (!LoadTextureClut(id, img->m_buffer))
				ok = 0;
		}
	}
	delete ld;
	return ok;
}

inline int gm_graph::CreateTexture(const void *data, int a, int b, int c, const void *clut, int d, int e)
{
	return gm_texture::new_texture((void *)data, a, b, c, (void *)clut, d, e);
}

inline int gm_graph::LoadTextureImage(int id, const void *data)
{
	return ((gm_texture *)id)->load_image((void *)data);
}

inline int gm_graph::LoadTextureClut(int id, const void *data)
{
	return ((gm_texture *)id)->load_clut((void *)data);
}

inline void gm_graph::DeleteTexture(int id)
{
	delete (gm_texture *)id;
}

inline void gm_graph::DuplicateTexture(int id)
{
	((gm_texture *)id)->duplicate();
}

inline int gm_graph::CreateColorBuffer(int a, int b, int c)
{
	return gm_texture::new_color_buffer(a, b, c);
}

inline int gm_graph::CreateDepthBuffer(int a, int b, int c)
{
	return gm_texture::new_depth_buffer(a, b, c);
}

inline void gm_graph::DeleteColorBuffer(int id)
{
	delete (gm_texture *)id;
}

inline void gm_graph::DeleteDepthBuffer(int id)
{
	delete (gm_texture *)id;
}

inline void gm_graph::SetColorBuffer(int id)
{
	fColorBufferId = id;
	fFlagsA |= 2;
}

inline void gm_graph::SetDepthBuffer(int id)
{
	fDepthBufferId = id;
	fFlagsA |= 0x02000000;
}

inline int gm_graph::CreateVertexShader(const char *name)
{
	void *data;
	int size;
	if (!gmLoadArchiveEntry(name, ".vutext", &data, &size))
		return 0;
	int handle = CreateVertexShader((const int *)data, size);
	if (data)
		delete[] (char *)data;
	return handle;
}

inline int gm_graph::CreateVertexShader(const int *data, int size)
{
	/* The microprogram goes into a display list, padded to 16 bytes;
           163.84 = 16384 / 100 gives the percentage of the 16 KB VU1 micro memory. */
	int pad = (-size) & 0xF;
	int aligned = size + pad;
	int dwords = aligned / 8;
	float pct = (float)aligned / 163.84f;
	printf("vshader = %d dwords ( %.1f %% )\n", dwords, (double)pct);
	fPacket.begin_list();
	int *dst = (int *)fPacket.alloc_packet(aligned);
	fPacket.put_packet(dst, aligned);
	memcpy(dst, data, size);
	memset((char *)dst + size, 0, pad);
	int *p;
	int n;
	fPacket.end_list(p, n);
	return (int)p;
}

inline void gm_graph::DeleteVertexShader(int id)
{
	if (id) {
		fPacket.free_heap2((void *)id);
	}
}

inline void gm_graph::SetVertexShader(int id)
{
	if (fCurListOrVsh != 0)
		return;
	if (id == fVertexShaderId)
		return;
	if (id == -1) {
		if ((unsigned)(fVertexShaderId + 3) < 2)
			return;
		id = -2;
	}
	fVertexShaderId = id;
	fFlagsB |= 0x10000;
}

inline int gm_graph::CreateList()
{
	gm_gslist *p = new gm_gslist;
	p->fHead = 0;
	p->fRefCount = 1;
	p->fStateSaved = 0;
	return (int)p;
}

inline void gm_graph::DeleteList(int listHandle)
{
	gm_gslist *p = (gm_gslist *)listHandle;
	if (!p)
		return;
	int rc = --p->fRefCount;
	if (rc == 0) {
		fPacket.free_heap2(p->fHead);
		delete p;
	}
}

inline int gm_graph::DuplicateList(int listHandle)
{
	gm_gslist *p = (gm_gslist *)listHandle;
	if (p)
		p->fRefCount++;
	return (int)p;
}

inline int gm_graph::CallList(int listHandle)
{
	gm_gslist *p = (gm_gslist *)listHandle;
	if (!p)
		return 0;
	int *head = p->fHead;
	if (!head)
		return 0;
	fFlagsA &= ~p->fInitFlags;
	Flush();
	fPacket.put_list(head);
	fFlagsA = p->fExitFlags;
	Flush();
	return 1;
}

inline int gm_graph::BeginList(int listHandle)
{
	if (!listHandle)
		return 0;
	gm_gslist *p = (gm_gslist *)listHandle;
	Flush();
	fCurListOrVsh = listHandle;
	p->fStateSaved = 0;
	p->fInitFlags = 0;
	p->fExitFlags = 0;
	fPacket.begin_list();
	return 1;
}

inline int gm_graph::EndList(int *out)
{
	gm_gslist *p = (gm_gslist *)fCurListOrVsh;
	if (!p)
		return 0;
	Flush();
	fCurListOrVsh = 0;
	int *list;
	int size;
	int result = fPacket.end_list(list, size);
	if (out)
		*out = size;
	fPacket.free_heap2(p->fHead);
	if (fFenceRaw > 0) {
		int cnt = fPacket.max_heap_space();
		if (cnt < fFenceRaw) {
			fPacket.free_heap(list);
			list = 0;
			result = 0;
		}
	}
	p->fHead = list;
	if (result == 0) {
		fPacketFailB++;
	}
	return result;
}

inline int gm_graph::ClearList(int listHandle)
{
	gm_gslist *p = (gm_gslist *)listHandle;
	if (!p || listHandle == fCurListOrVsh)
		return 0;
	fPacket.free_heap2(p->fHead);
	p->fHead = 0;
	return 1;
}

/* Sets or clears one bit of fFlagsE (FrontFace, ShadeModel, DepthMask). */
inline void gm_enable_bit(gm_graph *g, int bit, int on)
{
	int flags = on ? (g->fFlagsE | bit) : (g->fFlagsE & ~bit);
	g->SetEnableBits(flags);
}

inline void gm_graph::SetEnableBits(int bits)
{
	int diff = fFlagsE ^ bits;
	if (diff == 0)
		return;
	fFlagsE = bits;
	fFlagsA |= diff & 0x03E00000;
	fFlagsB |= diff & 0xDF;
	fFlagsC |= diff & 0xF0000000;
	if (diff & 0x1F0000) {
		fLayerActiveMask |= fLayerEnableMask & 0xF;
	}
	if (diff & GM_ENABLE_TEXTURE_FIT) {
		fLayerActiveMask |= 0x10001000;
	}
	if (diff & 1) {
		signed char v = (fFlagsE & 1) ? 15 : 0;
		if (fLitLayerMask != v) {
			fLitLayerMask = v;
			fFlagsB |= 0x80000000;
		}
	}
}

inline void gm_graph::FrontFace(int enable)
{
	gm_enable_bit(this, GM_ENABLE_FRONT_FACE, enable);
}

inline void gm_graph::ShadeModel(int enable)
{
	gm_enable_bit(this, GM_ENABLE_SMOOTH_SHADE, enable);
}

inline void gm_graph::DepthMask(bool enable)
{
	gm_enable_bit(this, GM_ENABLE_DEPTH_MASK, enable);
}

inline void gm_graph::ColorMask(int r, int g, int b, int a)
{
	int packed = (r & 0xFF) | ((g & 0xFF) << 8) | ((b & 0xFF) << 16) | ((a & 0xFF) << 24);
	fColorMaskPacked = packed;
	fFlagsA |= 2;
}

inline void gm_graph::ColorMask(int rgba)
{
	fColorMaskPacked = rgba;
	fFlagsA |= 2;
}

inline void gm_graph::AlphaFunc(int a, int b)
{
	fAlphaFuncA = a;
	fAlphaFuncB = b;
	fFlagsA |= 0x01C00000;
}

inline void gm_graph::DepthFunc(int func)
{
	fDepthFuncVal = func;
	fFlagsA |= 0x01C00000;
}

inline void gm_graph::StencilFunc(int a, int b)
{
	fStencilFuncA = a;
	fStencilFuncB = b;
	fFlagsA |= 0x01C00000;
}

inline void gm_graph::StencilOp(int a, int b, int c)
{
	fStencilOpA = a;
	fStencilOpB = b;
	fStencilOpC = c;
	fFlagsA |= 8;
}

inline void gm_graph::SetLayer(int idx)
{
	if ((unsigned)idx < 4) {
		fCurLayerIndex = idx;
		fCurLayerPtr = &fLayers[idx];
	}
}

inline void gm_graph::BlendFunc(int mode)
{
	gm_layer *l = fCurLayerPtr;
	l->fBlendSrc = mode;
	int idx = fCurLayerIndex;
	fLayerActiveMask |= 0x10000 << idx;
	unsigned stripe = 0x11111111 << idx;
	unsigned enableMask = fLayerEnableMask;
	if (mode != GM_BLEND_OFF) {
		if (!(enableMask & stripe)) {
			fLayerActiveMask |= stripe;
			fLayerEnableMask = enableMask | stripe;
			fFlagsB |= 0x80000000;
		}
	} else {
		if (enableMask & stripe) {
			fLayerEnableMask = enableMask & ~stripe;
			fFlagsB |= 0x80000000;
		}
	}
}

inline void gm_graph::ColorGen(int mode, int arg)
{
	int bit = 1 << fCurLayerIndex;
	int mask = fColorGenMask;
	if (mode == 1)
		mask |= bit;
	else
		mask &= ~bit;
	if (fColorGenMask != mask) {
		fColorGenMask = mask;
		fFlagsB |= 0x80000000;
	}
	fCurLayerPtr->fColorGenMode = mode;
	fCurLayerPtr->fColorGenArg = arg;
	fLayerActiveMask |= 0x1000000 << fCurLayerIndex;
}

/* Stores the current layer's texture-coordinate generation: `source`
   (a GM_TEXGEN_*, GM_TEXGEN_TEXCOORD by default) becomes the code sent to the VU program in
   fTexGenCode; GM_TEXGEN_REFLECTION also sets the layer's bit in fTexGenMask. */
inline void gm_graph::TexGen(int mode, int source)
{
	int bit = 1 << fCurLayerIndex;
	int mask = fTexGenMask;
	if (source == GM_TEXGEN_REFLECTION)
		mask |= bit;
	else
		mask &= ~bit;
	if (fTexGenMask != mask) {
		fTexGenMask = mask;
		fFlagsB |= 0x80000000;
	}
	switch (source) {
	case GM_TEXGEN_TEXCOORD1:
		source = 0x92;
		break;
	case GM_TEXGEN_NORMAL:
		source = 0x4a;
		break;
	case GM_TEXGEN_POSITION:
		source = 0x26;
		break;
	case GM_TEXGEN_REFLECTION:
		source = 0x82e0;
		break;
	case GM_TEXGEN_TEXCOORD:
		source = 0x6e;
		break;  /* redundant as written */
	default:
		source = 0x6e;
		break;
	}
	fCurLayerPtr->fTexGenMode = mode;
	fCurLayerPtr->fTexGenCode = source;
	fLayerActiveMask |= 0x1000000 << fCurLayerIndex;
}

inline void gm_graph::SetTexture(int id)
{
	gm_layer *l = fCurLayerPtr;
	if (id == l->fTexId)
		return;
	l->fTexId = id;
	int idx = fCurLayerIndex;
	int bit = 1 << idx;
	int e = fTexEnableMask;
	if (id != 0) {
		e |= bit;
		fLayerActiveMask |= (16 << idx);
	} else {
		e &= ~bit;
	}
	fLayerActiveMask |= (e ^ fTexEnableMask);
	fTexEnableMask = e;
}

inline void gm_graph::TexFunc(int a, int b)
{
	fCurLayerPtr->fTexFuncA = a;
	fCurLayerPtr->fTexFuncB = b;
	fLayerActiveMask |= 16 << fCurLayerIndex;
}

inline void gm_graph::TexFilter(int a, int b)
{
	fCurLayerPtr->fTexFilterA = a;
	fCurLayerPtr->fTexFilterB = b;
	fLayerActiveMask |= 256 << fCurLayerIndex;
}

inline void gm_graph::TexWrap(int a, int b)
{
	fCurLayerPtr->fTexWrapA = a;
	fCurLayerPtr->fTexWrapB = b;
	fLayerActiveMask |= 4096 << fCurLayerIndex;
}

inline void gm_graph::TexCrop(const vec4 &c)
{
	fCurLayerPtr->fTexCrop = c;
	fLayerActiveMask |= 0x10000000 << fCurLayerIndex;
}

inline void gm_graph::TexCrop(float x, float y, float z, float w)
{
	const vec4 &c = vec4(x, y, z, w);
	fCurLayerPtr->fTexCrop = c;
	fLayerActiveMask |= 0x10000000 << fCurLayerIndex;
}

inline void gm_graph::TexWrapRegion(int a, int b, int c, int d)
{
	fCurLayerPtr->fTexWrapRegionA = a;
	fCurLayerPtr->fTexWrapRegionB = b;
	fCurLayerPtr->fTexWrapRegionC = c;
	fCurLayerPtr->fTexWrapRegionD = d;
	fLayerActiveMask |= 4096 << fCurLayerIndex;
}

inline void gm_graph::TexLodFunc(int a, int b, int c)
{
	fCurLayerPtr->fTexLodA = a;
	fCurLayerPtr->fTexLodB = b;
	fCurLayerPtr->fTexLodC = c;
	fLayerActiveMask |= 256 << fCurLayerIndex;
}

inline void gm_graph::VertexGen(int mode)
{
	fVertexGenMode = mode;
	fFlagsB |= 0xDF;
}

inline void gm_graph::ParticleSize(float v)
{
	fParticleVec.w = v;
	fFlagsB |= 0x04000000;
}

inline void gm_graph::ParticleCrop(const vec4 &c)
{
	fParticleCrop = c;
	fFlagsB |= 0x04000000;
}

inline void gm_graph::ParticleCrop(float x, float y, float z, float w)
{
	const vec4 &c = vec4(x, y, z, w);
	fFlagsB |= 0x04000000;
	fParticleCrop = c;
}

inline void gm_graph::PatchLevel(float v)
{
	fPatchVec.w = v;
	fFlagsB |= 0x08000000;
}

inline void gm_graph::PatchSubdiv(float a, float b)
{
	fPatchSubdiv.x = a;
	fPatchSubdiv.y = b;
	fFlagsB |= 0x10000000;
}

inline void gm_graph::PatchClip(const vec4 &c)
{
	fPatchClip = c;
	fFlagsB |= 0x20000000;
}

inline void gm_graph::PatchClip(float x, float y, float z, float w)
{
	const vec4 &c = vec4(x, y, z, w);
	fFlagsB |= 0x20000000;
	fPatchClip = c;
}

inline void gm_graph::PatchCrop(const vec4 &c)
{
	fPatchCrop = c;
	fFlagsB |= 0x20000000;
}

inline void gm_graph::PatchCrop(float x, float y, float z, float w)
{
	const vec4 &c = vec4(x, y, z, w);
	fFlagsB |= 0x20000000;
	fPatchCrop = c;
}

inline int gm_graph::GetStride(int fmt)
{
	return gmVertexStride(fmt);
}

inline void gm_graph::SetArrays(const void *data, int format, int count, int flag, int weightFormat,
    const void *weightData)
{
	fArrayPtr = data;
	fArrayFormat = format;
	fArrayCount = count;
	int stride = gmVertexStride(format);
	fArrayStride = stride;
	fArrayFlag = flag;
	if (flag < 2)
		return;
	if (weightFormat == 0)
		weightFormat = format;
	if (!weightData)
		weightData = (const char *)data + stride * count;
	fArrayEndPtr = weightData;
	fArrayWeightFormat = weightFormat;
	fArrayWeightStride = gmVertexStride(weightFormat);
}

inline void gm_graph::SetMorphWeights(const float *w, int count)
{
	int n;
	if (count >= 17)
		n = 16;
	else
		n = count;
	fMorphCount = n;
	for (int i = 0; i < n; i++)
		fMorphWeights[i] = w[i];
	fFlagsB |= 0x1000000;
}

inline void gm_graph::DrawArrays(int kind, int start, int stride, int chunkLimit, const short *indices)
{
	gm_draw_arrays(this, kind, start, stride, chunkLimit, indices);
}

inline void gm_graph::DrawParticle(int kind, int start, int stride, int chunkLimit, const short *indices)
{
	SetVertexShader(-2);
	flush_state();
	int prim = gmParticlePrim[kind];
	int tag = gm_giftag_hi(1, prim, 3, kind);
	fDrawMode = 1;
	int tagBits = gmKindTagBits[kind];
	fDrawChunkCap = 18;
	fDrawTagHi = tag | tagBits;
	fDrawChunkOverlap = gmKindOverlap[kind];
	if (kind == 1) {
		unsigned bias = (unsigned)stride >> 31;
		int half = (stride + (int)bias) >> 1;
		stride = 2;
		chunkLimit *= half;
	}
	if (!indices)
		send_arrays_linear(kind, start, stride, chunkLimit);
	else
		send_arrays_indexed(kind, indices + start, stride, chunkLimit);
}

inline void gm_graph::send_arrays_linear(int kind, int start, int stride, int chunkLimit)
{
	int total = stride * chunkLimit;
	int remaining = fArrayCount - start;
	int adjCap = fDrawChunkOverlap;
	int cap = fDrawChunkCap;
	if (remaining < total)
		total = remaining;
	int carry = 0;
	while (total > 0) {
		if (total < cap) {
			send_block_linear(kind, start, stride, total, carry);
			break;
		}
		send_block_linear(kind, start, stride, cap, carry);
		int sum = carry + cap;
		start += cap;
		carry = sum % stride;
		total -= cap;
		if (carry != 0) {
			if (carry < adjCap)
				adjCap = carry;
			start -= adjCap;
			total += adjCap;
			carry -= adjCap;
		}
	}
}

inline void gm_graph::send_arrays_indexed(int kind, const short *indices, int stride, int chunkLimit)
{
	int cap = fDrawChunkCap;
	int adjCap = fDrawChunkOverlap;
	int total = stride * chunkLimit;
	int carry = 0;
	while (total > 0) {
		if (total < cap) {
			send_block_indexed(kind, indices, stride, total, carry);
			break;
		}
		send_block_indexed(kind, indices, stride, cap, carry);
		int sum = carry + cap;
		indices += cap;
		carry = sum % stride;
		total -= cap;
		if (carry != 0) {
			if (carry < adjCap)
				adjCap = carry;
			indices -= adjCap;
			total += adjCap;
			carry -= adjCap;
		}
	}
}

void gm_graph::SetBlendMatrices(const mat4 *matrices, int count)
{
	fCurMatrixMode = 0;
	fCurMatrixPtr = &fMatrixStack[0];
	mat4_stack *e = fCurMatrixPtr;
	mat4 *m = (mat4 *)(e->fDepth * 64 + (int)e->fBase);
	m->m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	m->m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	m->m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	m->m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	(e->fScales)[e->fDepth].Set(1.0f, 1.0f, 1.0f, 1.0f);
	(e->fScales)[e->fDepth].w = 0.0f;
	int n;
	if (count >= 6)
		n = 5;
	else
		n = count;
	fFlagsC |= 1;
	fBlendCount = n;
	for (int i = 0; i < n; i++)
		fBlendMatrices[i] = matrices[i];
	fFlagsB |= 0x2000000;
}

void gm_graph::SetBlendMatrices(const mat4 **matrices, int count)
{
	fCurMatrixMode = 0;
	fCurMatrixPtr = &fMatrixStack[0];
	mat4_stack *e = fCurMatrixPtr;
	mat4 *m = (mat4 *)(e->fDepth * 64 + (int)e->fBase);
	m->m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	m->m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	m->m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	m->m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	(e->fScales)[e->fDepth].Set(1.0f, 1.0f, 1.0f, 1.0f);
	(e->fScales)[e->fDepth].w = 0.0f;
	int n;
	if (count >= 6)
		n = 5;
	else
		n = count;
	fFlagsC |= 1;
	fBlendCount = n;
	for (int i = 0; i < n; i++)
		fBlendMatrices[i] = *matrices[i];
	fFlagsB |= 0x2000000;
}

/* DrawMesh(kind, start, cols, rows, stride, indices): builds on the stack the
   index list of (rows - 1) triangle strips of `cols` pairs each, sequential
   from `start` or copied from `indices` (row step `stride`, 0 = cols), then
   sends it like DrawArrays with start 0, stride cols * 2 and chunk limit
   rows - 1. */
void gm_graph::DrawMesh(int kind, int start, int cols, int rows, int stride, const short *indices)
{
	int rowbytes = (rows - 1) * 4;
	short *buf = (short *)__builtin_alloca(cols * rowbytes);
	short *dst = buf;
	if (stride == 0)
		stride = cols;
	if (!indices) {
		for (int r = 0; r < rows - 1; r++) {
			int cur = start;
			for (int i = 0; i < cols; i++) {
				dst[0] = cur;
				dst[1] = cur + stride;
				cur++;
				dst += 2;
			}
			start += stride;
		}
	} else {
		indices += start;
		for (int r = 0; r < rows - 1; r++) {
			for (int i = 0; i < cols; i++) {
				dst[0] = indices[0];
				dst[1] = indices[stride];
				indices++;
				dst += 2;
			}
			indices += stride - cols;
		}
	}
	gm_draw_arrays(this, kind, 0, cols * 2, rows - 1, buf);
}

/* DrawRectPatch(kind, basis, start, cols, rows, stride, indices): builds on
   the stack the index list of the patch (sequential from `start`, or copied
   from `indices` with row step `stride`, 0 = cols) and sends it with
   send_block_indexed; `basis` (0 bilinear, 15 bicubic in mesh_class) goes to
   fDrawPatchBasis. */
void gm_graph::DrawRectPatch(int kind, int basis, int start, int cols, int rows, int stride, const short *indices)
{
	SetVertexShader(-3);
	flush_state();
	if ((unsigned)(cols - 2) >= 3)
		return;
	if ((unsigned)(rows - 2) >= 3)
		return;
	int tag = gm_giftag_hi(1, kind, 3, kind);
	if (stride == 0)
		stride = cols;
	fDrawMode = 2;
	int tagBits = gmKindTagBits[kind];
	fDrawChunkCap = 36;
	fDrawChunkOverlap = 0;
	fDrawPatchBasis = basis;
	fDrawTagHi = tag | tagBits;

	int rowbytes = rows * 2;
	short *buf = (short *)__builtin_alloca(cols * rowbytes);
	short *dst = buf;
	if (!indices) {
		int r = 0;
		while (r < rows) {
			int cur = start;
			int next = cur + stride;
			for (int i = 0; i < cols; i++)
				*dst++ = cur++;
			r++;
			start = next;
		}
	} else {
		indices += start;
		for (int r = 0; r < rows; r++) {
			for (int i = 0; i < cols; i++)
				*dst++ = *indices++;
			indices += stride - cols;
		}
	}
	send_block_indexed(kind, buf, rows, cols * rows, rows - cols);
}

/* Triangle fans (kind 5): `count` fans of `stride` vertices, linear from
   `start` or read from `indices` row by row. A fan longer than the chunk
   capacity (fDrawChunkCap) is split into chunks that repeat the last vertex,
   buf[0] staying the centre. */
void gm_graph::draw_trifan(int start, int stride, int count, const short *indices)
{
	if (!indices) {
		flush_state();
		int cap = fDrawChunkCap;
		if (stride <= cap) {
			for (int i = 0; i < count; i++) {
				send_block_linear(5, start, stride, stride, 0);
				start += stride;
			}
		} else {
			short *buf = (short *)__builtin_alloca(cap * 2);
			for (int i = 0; i < count; i++) {
				buf[0] = start;
				int n = stride - 1;
				int next = start + stride;
				while (n > 0) {
					int k = cap;
					if (n + 1 < cap)
						k = n + 1;
					int j;
					for (j = 1; j < k; j++)
						buf[j] = ++start;
					send_block_indexed(5, buf, k, k, 0);
					start--;
					n -= k - 1;
					if (n > 0)
						n++;
				}
				start = next;
			}
		}
	} else {
		indices += start;
		flush_state();
		int cap = fDrawChunkCap;
		if (stride <= cap) {
			for (int i = 0; i < count; i++) {
				send_block_indexed(5, indices, stride, stride, 0);
				indices += stride;
			}
		} else {
			short *buf = (short *)__builtin_alloca(cap * 2);
			for (int i = 0; i < count; i++) {
				const short *p = indices;
				int n = stride - 1;
				buf[0] = *indices;
				while (n > 0) {
					int k = cap;
					if (n + 1 < cap)
						k = n + 1;
					int j;
					for (j = 1; j < k; j++)
						buf[j] = *++p;
					send_block_indexed(5, buf, k, k, 0);
					p--;
					n -= k - 1;
					if (n > 0)
						n++;
				}
				indices += stride;
			}
		}
	}
}

inline void mat4_stack::clear()
{
	fDepth = 0;
	load_identity();
}

/* send_block_linear sends `count` vertices of the current arrays to VU1,
   splitting the chunk at the wrap of the ring buffer (firstSpan, wrapSpan).
   For each blend pass (fArrayFlag) it writes a header (tag 0x6C028000) and
   one block per field selected by fArrayFormat; the fields are interleaved in
   the vertex buffer (stride fArrayStride) and are halfwords when
   fArrayFormat & 0x8000. From the second pass on the source is the weight
   buffer (fArrayEndPtr, fArrayWeightStride, fArrayWeightFormat). `kind` is
   unused. */
void gm_graph::send_block_linear(int kind, int start, int stride, int count, int carry)
{
	(void)kind;
	int firstSpan = stride - carry;
	if (count < firstSpan)
		firstSpan = count;
	int rem = count - firstSpan;
	int wrapSpan = stride;
	if (rem < wrapSpan)
		wrapSpan = rem;

	gm_packet *pk = &fPacket;
	char *limit = (char *)((int)pk->fTagCursor & ~0xFFF);
	char *cursor = pk->fCursor;
	if (limit - cursor < fFenceLo) {
		fPacketFailA++;
		return;
	}

	int pass;
	int vuBase = 0;
	const char *src = (const char *)fArrayPtr + fArrayStride * start;
	int elemStride = fArrayStride;
	int fmt = fArrayFormat;
	char *dst = cursor;

	for (pass = 0; pass < fArrayFlag; pass++) {
		*(int *)dst = 0x6C028000;
		char *hdr = dst + 4;
		*(int *)(dst + 4) = fDrawMode;
		*(int *)(hdr + 4) = fmt;
		*(int *)(hdr + 8) = count;
		*(int *)(hdr + 0xC) = pass;
		*(int *)(hdr + 0x10) = fDrawPatchBasis;
		*(int *)(hdr + 0x14) = fDrawTagHi;
		*(int *)(hdr + 0x18) = firstSpan;
		*(int *)(hdr + 0x1C) = wrapSpan;
		dst += 0x24;

		if (!(fmt & 0x8000)) {
			if (fmt & 1) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0xC0C0C0C0;
				*(int *)(dst + 8) = VIF_UNPACK(vuBase + 0x26, count, 0x78000000);
				dst += 0xC;
				const char *rs = src;
				for (int i = 0; i < count; i++) {
					*(int *)dst = *(const int *)rs;
					*(int *)(dst + 4) = *(const int *)(rs + 4);
					*(int *)(dst + 8) = *(const int *)(rs + 8);
					rs += elemStride;
					dst += 0xC;
				}
				src += 0xC;
			}
			if (fmt & 0xF00) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0x3F3F3F3F;
				dst += 8;
				int blkOff = vuBase + 0x26;
				int n = (fmt & 0xF00) >> 8;
				for (int j = 0; j < n; j++) {
					*(int *)dst = VIF_UNPACK(blkOff, count, 0x70000000);
					dst += 4;
					const char *rs = src;
					for (int i = 0; i < count; i++) {
						*(int *)dst = *(const int *)rs;
						rs += elemStride;
						dst += 4;
					}
					src += 4;
					blkOff += 0x24;
				}
			}
			if (fmt & 2) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0xC0C0C0C0;
				*(int *)(dst + 8) = VIF_UNPACK(vuBase + 0x4A, count, 0x78000000);
				dst += 0xC;
				const char *rs = src;
				for (int i = 0; i < count; i++) {
					*(int *)dst = *(const int *)rs;
					*(int *)(dst + 4) = *(const int *)(rs + 4);
					*(int *)(dst + 8) = *(const int *)(rs + 8);
					rs += elemStride;
					dst += 0xC;
				}
				src += 0xC;
			}
			if (fmt & 8) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0xCFCFCFCF;
				*(int *)(dst + 8) = VIF_UNPACK(vuBase + 0x92, count, 0x70000000);
				dst += 0xC;
				const char *rs = src;
				for (int i = 0; i < count; i++) {
					*(int *)dst = *(const int *)rs;
					rs += elemStride;
					dst += 4;
				}
				src += 4;
			}
			if (fmt & 4) {
				*(int *)dst = VIF_UNPACK(vuBase + 2, count, 0x6E004000);
				dst += 4;
				const char *rs = src;
				for (int i = 0; i < count; i++) {
					*(int *)dst = *(const int *)rs;
					rs += elemStride;
					dst += 4;
				}
				src += 4;
			}
			if (fmt & 0xF0) {
				int blkOff = vuBase + 0x6E;
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0xF0F0F0F0;
				dst += 8;
				int n = (fmt & 0xF0) >> 4;
				for (int j = 0; j < n; j++) {
					*(int *)dst = VIF_UNPACK(blkOff, count, 0x74000000);
					dst += 4;
					const char *rs = src;
					for (int i = 0; i < count; i++) {
						*(int *)dst = *(const int *)rs;
						*(int *)(dst + 4) = *(const int *)(rs + 4);
						rs += elemStride;
						dst += 8;
					}
					src += 8;
					blkOff += 0x24;
				}
			}
		} else {
			if (fmt & 1) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0xC0C0C0C0;
				*(int *)(dst + 8) = VIF_UNPACK(vuBase + 0x26, count, 0x79000000);
				const char *rs = src;
				short *h = (short *)(dst + 0xC);
				for (int i = 0; i < count; i++) {
					h[0] = *(const short *)rs;
					h[1] = *(const short *)(rs + 2);
					h[2] = *(const short *)(rs + 4);
					rs += elemStride;
					h += 3;
				}
				src += 6;
				dst = (char *)(h + (count & 1));
			}
			if (fmt & 2) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0xC0C0C0C0;
				*(int *)(dst + 8) = VIF_UNPACK(vuBase + 0x4A, count, 0x79000000);
				const char *rs = src;
				short *h = (short *)(dst + 0xC);
				for (int i = 0; i < count; i++) {
					h[0] = *(const short *)rs;
					h[1] = *(const short *)(rs + 2);
					h[2] = *(const short *)(rs + 4);
					rs += elemStride;
					h += 3;
				}
				src += 6;
				dst = (char *)(h + (count & 1));
			}
			if (fmt & 0xF0) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0xF0F0F0F0;
				dst += 8;
				int blkOff = vuBase + 0x6E;
				int n = (fmt & 0xF0) >> 4;
				for (int j = 0; j < n; j++) {
					*(int *)dst = VIF_UNPACK(blkOff, count, 0x75000000);
					const char *rs = src;
					short *h = (short *)(dst + 4);
					for (int i = 0; i < count; i++) {
						h[0] = *(const short *)rs;
						h[1] = *(const short *)(rs + 2);
						rs += elemStride;
						h += 2;
					}
					src += 4;
					dst = (char *)h;
					blkOff += 0x24;
				}
			}
		}

		*(int *)dst = 0x14000008;
		dst += 4;
		if (fArrayFlag >= 2) {
			*(int *)(dst - 4) = 0x14000006;
			if (pass == 0) {
				vuBase = 18;
				src = (const char *)fArrayEndPtr + fArrayWeightStride * start;
				elemStride = fArrayWeightStride;
				fmt = fArrayWeightFormat;
			} else {
				src += fArrayWeightStride * (fArrayCount - 1);
			}
		}
	}

	if (fArrayFlag >= 2) {
		if (fArrayFlag & 1) {
			*(int *)dst = 0x14000000;
			dst += 4;
		}
		*(int *)dst = 0x6C028000;
		char *unpackData = dst + 4;
		*(int *)(dst + 4) = fDrawMode;
		*(int *)(unpackData + 4) = fArrayFormat;
		*(int *)(unpackData + 8) = count;
		*(int *)(unpackData + 0xC) = 0;
		*(int *)(unpackData + 0x10) = fDrawPatchBasis;
		*(int *)(unpackData + 0x14) = fDrawTagHi;
		*(int *)(unpackData + 0x18) = firstSpan;
		*(int *)(unpackData + 0x1C) = wrapSpan;
		dst += 0x24;
		*(int *)dst = 0x14000008;
		dst += 4;
	} else {
		if (fArrayFormat & 0x8000) {
			*(int *)(dst - 4) = 0x14000006;
			*(int *)dst = 0x14000000;
			*(int *)(dst + 4) = 0x14000008;
			dst += 8;
		}
	}

	char *tail = dst;
	int rem16 = (int)tail & 0xC;
	if (rem16 != 0) {
		while (rem16 < 0x10) {
			*(int *)tail = 0;
			tail += 4;
			rem16 += 4;
		}
	}
	gm_packet *packet = &fPacket;
	int size = tail - packet->fCursor;
	packet->fCursor = tail;
	fPacket.put_packet((int *)cursor, size);
}

/* Same as send_block_linear, except that vertex i is at src + fArrayStride *
   indices[i] (no start), a VU address base (0, then 18 from the second pass)
   is added to all block offsets, and the compressed path has only the bit 0
   block. */
void gm_graph::send_block_indexed(int kind, const short *indices, int stride, int count, int carry)
{
	(void)kind;
	int fmt = 0, pass, vuBase, elemStride = 0;
	const char *src = 0;
	char *dst = 0;
	int firstSpan = stride - carry;
	if (count < firstSpan)
		firstSpan = count;
	int rem = count - firstSpan;
	int wrapSpan = stride;
	if (rem < wrapSpan)
		wrapSpan = rem;

	gm_packet *pk = &fPacket;
	char *limit = (char *)((int)pk->fTagCursor & ~0xFFF);
	char *cursor = pk->fCursor;
	if (limit - cursor < fFenceLo) {
		fPacketFailA++;
		return;
	}

	vuBase = 0;
	src = (const char *)fArrayPtr;
	fmt = fArrayFormat;
	elemStride = fArrayStride;
	dst = cursor;

	for (pass = 0; pass < fArrayFlag; pass++) {
		*(int *)dst = 0x6C028000;
		char *hdr = dst + 4;
		*(int *)hdr = fDrawMode;
		*(int *)(hdr + 4) = fmt;
		*(int *)(hdr + 8) = count;
		*(int *)(hdr + 0xC) = pass;
		*(int *)(hdr + 0x10) = fDrawPatchBasis;
		*(int *)(hdr + 0x14) = fDrawTagHi;
		*(int *)(hdr + 0x18) = firstSpan;
		*(int *)(hdr + 0x1C) = wrapSpan;
		dst += 0x24;

		if (!(fmt & 0x8000)) {
			if (fmt & 1) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0xC0C0C0C0;
				*(int *)(dst + 8) = VIF_UNPACK(vuBase + 0x26, count, 0x78000000);
				dst += 0xC;
				for (int i = 0; i < count; i++) {
					const char *rs = src + elemStride * indices[i];
					*(int *)dst = *(const int *)rs;
					*(int *)(dst + 4) = *(const int *)(rs + 4);
					*(int *)(dst + 8) = *(const int *)(rs + 8);
					dst += 0xC;
				}
				src += 0xC;
			}
			if (fmt & 0xF00) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0x3F3F3F3F;
				dst += 8;
				int blkOff = vuBase + 0x26;
				int n = (fmt & 0xF00) >> 8;
				for (int j = 0; j < n; j++) {
					*(int *)dst = VIF_UNPACK(blkOff, count, 0x70000000);
					dst += 4;
					for (int i = 0; i < count; i++) {
						const char *rs = src + elemStride * indices[i];
						*(int *)dst = *(const int *)rs;
						dst += 4;
					}
					src += 4;
					blkOff += 0x24;
				}
			}
			if (fmt & 2) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0xC0C0C0C0;
				*(int *)(dst + 8) = VIF_UNPACK(vuBase + 0x4A, count, 0x78000000);
				dst += 0xC;
				for (int i = 0; i < count; i++) {
					const char *rs = src + elemStride * indices[i];
					*(int *)dst = *(const int *)rs;
					*(int *)(dst + 4) = *(const int *)(rs + 4);
					*(int *)(dst + 8) = *(const int *)(rs + 8);
					dst += 0xC;
				}
				src += 0xC;
			}
			if (fmt & 8) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0xCFCFCFCF;
				*(int *)(dst + 8) = VIF_UNPACK(vuBase + 0x92, count, 0x70000000);
				dst += 0xC;
				for (int i = 0; i < count; i++) {
					const char *rs = src + elemStride * indices[i];
					*(int *)dst = *(const int *)rs;
					dst += 4;
				}
				src += 4;
			}
			if (fmt & 4) {
				*(int *)dst = VIF_UNPACK(vuBase + 2, count, 0x6E004000);
				dst += 4;
				for (int i = 0; i < count; i++) {
					const char *rs = src + elemStride * indices[i];
					*(int *)dst = *(const int *)rs;
					dst += 4;
				}
				src += 4;
			}
			if (fmt & 0xF0) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0xF0F0F0F0;
				dst += 8;
				int blkOff = vuBase + 0x6E;
				int n = (fmt & 0xF0) >> 4;
				for (int j = 0; j < n; j++) {
					*(int *)dst = VIF_UNPACK(blkOff, count, 0x74000000);
					dst += 4;
					for (int i = 0; i < count; i++) {
						const char *rs = src + elemStride * indices[i];
						*(int *)dst = *(const int *)rs;
						*(int *)(dst + 4) = *(const int *)(rs + 4);
						dst += 8;
					}
					src += 8;
					blkOff += 0x24;
				}
			}
		} else {
			if (fmt & 1) {
				*(int *)dst = 0x20000000;
				*(int *)(dst + 4) = 0xC0C0C0C0;
				*(int *)(dst + 8) = VIF_UNPACK(0x26, count, 0x69000000);
				short *h = (short *)(dst + 0xC);
				for (int i = 0; i < count; i++) {
					const char *rs = src + elemStride * indices[i];
					h[0] = *(const short *)rs;
					h[1] = *(const short *)(rs + 2);
					h[2] = *(const short *)(rs + 4);
					h += 3;
				}
				src += 6;
				dst = (char *)(h + (count & 1));
			}
		}

		*(int *)dst = 0x14000008;
		dst += 4;
		if (fArrayFlag >= 2) {
			*(int *)(dst - 4) = 0x14000006;
			if (pass == 0) {
				src = (const char *)fArrayEndPtr;
				vuBase = 18;
				fmt = fArrayWeightFormat;
				elemStride = fArrayWeightStride;
			} else {
				src += fArrayWeightStride * (fArrayCount - 1);
			}
		}
	}

	if (fArrayFlag >= 2) {
		if (fArrayFlag & 1) {
			*(int *)dst = 0x14000000;
			dst += 4;
		}
		*(int *)dst = 0x6C028000;
		char *unpackData = dst + 4;
		*(int *)unpackData = fDrawMode;
		*(int *)(unpackData + 4) = fArrayFormat;
		*(int *)(unpackData + 8) = count;
		*(int *)(unpackData + 0xC) = 0;
		*(int *)(unpackData + 0x10) = fDrawPatchBasis;
		*(int *)(unpackData + 0x14) = fDrawTagHi;
		*(int *)(unpackData + 0x18) = firstSpan;
		*(int *)(unpackData + 0x1C) = wrapSpan;
		dst += 0x24;
		*(int *)dst = 0x14000008;
		dst += 4;
	}

	char *tail = dst;
	int rem16 = (int)tail & 0xC;
	if (rem16 != 0) {
		while (rem16 < 0x10) {
			*(int *)tail = 0;
			tail += 4;
			rem16 += 4;
		}
	}
	gm_packet *packet = &fPacket;
	int size = tail - packet->fCursor;
	packet->fCursor = tail;
	fPacket.put_packet((int *)cursor, size);
}

void gm_graph::reset_state()
{
	fArrayPtr = 0;
	fArrayCount = 0;
	fArrayEndPtr = 0;
	fArrayFlag = 0;
	fMorphCount = 0;
	fBlendCount = 0;
	fCurListOrVsh = 0;
	MatrixMode(GM_MATRIX_MODEL);

	for (int mi = 0; mi < 7; mi++)
		fMatrixStack[mi].clear();

	fFlagsA = -1;
	fFlagsB = -1;
	fFlagsC = -1;
	fLayerActiveMask = -1;
	fFlagsE = 0;
	fVshFlags = 0;
	fPointLightMask = 0;
	fLayerEnableMask = 0;
	fTexEnableMask = 0;
	fLitLayerMask = 0;
	fSpecularMask = 0;
	fColorGenMask = 0;
	fTexGenMask = 0;
	SetPixelOffset(0.0f, 0.0f);
	Viewport(0.0f, 0.0f, (float)fDisp.get_width(), (float)fDisp.get_height());
	{
		/* DepthRange(0.0f, 1.0f) */
		float *dr = fDepthRange;
		dr[4] = 0.0f;
		dr[5] = 1.0f;
		float sz = (dr[5] - dr[4]) * 0.5f;
		float tz = (dr[5] + dr[4]) * 0.5f;
		float sx = dr[2] * 0.5f;
		float sy = dr[3] * -0.5f;
		float m[16];
		m[0] = sx;
		m[1] = 0.0f;
		m[2] = 0.0f;
		m[3] = 0.0f;
		m[4] = 0.0f;
		m[5] = sy;
		m[6] = 0.0f;
		m[7] = 0.0f;
		m[8] = 0.0f;
		m[9] = 0.0f;
		m[10] = sz;
		m[11] = 0.0f;
		m[12] = dr[0] + sx;
		m[13] = dr[1] - sy;
		m[14] = tz;
		m[15] = 1.0f;
		(&fMatrixStack[3])->load_matrix(m);
		fFlagsC |= 8;
	}
	ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	ClearDepth(1.0f);
	ClearStencil(0);
	Fog(0, vec4(0.0f, 0.0f, 0.0f, 1.0f), 20.0f, 100.0f);
	Ambient(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 dir(0.0f, -1.0f, 0.0f, 1.0f);
	vec4 value;
	for (int li = 0; li < 8; li++) {
		value.Set(0.0f, 0.0f, 0.0f, 0.0f);
		Light(li, GM_LIGHT_DIRECTIONAL, value, dir);
	}
	int specMask = 0;
	value.Set(1.0f, 1.0f, 1.0f, 1.0f);
	Color(value);
	value.Set(0.0f, 0.0f, 0.0f, 1.0f);
	Diffuse(value);
	value.Set(0.0f, 0.0f, 0.0f, 1.0f);
	{
		/* Specular(c, 10.0f) */
		const vec4 &c = value;
		float exp = 10.0f / 6.0f;
		fSpecular = c;
		fSpecularExtraA = exp;
		fSpecularExtraB = 1.0f - exp;
		fFlagsB |= 0x200000;
		if (c.w != 0.0f) {
			if (c.x + c.y + c.z != 0.0f)
				specMask = 15;
		}
	}
	if (fSpecularMask != specMask) {
		fSpecularMask = (signed char)specMask;
		fFlagsB |= 0x80000000;
	}
	value.Set(0.0f, 0.0f, 0.0f, 1.0f);
	fTexFactorBias = 1.0f;
	fColorBufferId = -1;
	fDepthBufferId = -1;
	fEmission = value;
	value.Set(0.0f, 0.0f, 0.0f, 1.0f);
	fFlagsB |= 0x400000;
	fFlagsB |= 0x800000;
	fFlagsA |= 0x2000002;
	float bias2 = 0.0f;              /* negated, as in TexFactor */
	fTexFactorBias2 = -bias2;
	fTexFactor = value;
	/* SetVertexShader(-1) */
	SetVertexShader(-1);

	gm_enable_bit(this, GM_ENABLE_TEXTURE, 1);
	gm_enable_bit(this, GM_ENABLE_BLEND, 1);
	gm_enable_bit(this, GM_ENABLE_DEPTH_TEST, 1);
	gm_enable_bit(this, GM_ENABLE_LIGHTING, 1);
	gm_enable_bit(this, 0x40, 1);
	FrontFace(1);
	ShadeModel(1);
	DepthMask(true);

	ColorMask(-1);
	AlphaFunc(GM_GREATER, 0);
	DepthFunc(GM_LEQUAL);
	StencilFunc(GM_EQUAL, 0);
	StencilOp(1, 1, 1);
	VertexGen(2);
	ParticleSize(1.0f);
	value.Set(0.0f, 0.0f, 1.0f, 1.0f);
	ParticleCrop(value);
	PatchLevel(1.0f);
	PatchSubdiv(16.0f, 16.0f);
	value.Set(0.0f, 0.0f, 1.0f, 1.0f);
	PatchClip(value);
	value.Set(0.0f, 0.0f, 1.0f, 1.0f);
	PatchCrop(value);

	gm_layer *lp = &fLayers[0];
	for (int i = 0; i < 4; i++) {
		SetLayer(i);
		BlendFunc(gmResetBlend[i]);
		ColorGen(gmResetColorGen[i], 0);
		TexGen(0, GM_TEXGEN_TEXCOORD);
		SetTexture(0);
		TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGBA);
		TexFilter(GM_FILTER_LINEAR, GM_FILTER_LINEAR_MIPMAP_LINEAR);
		TexWrap(GM_WRAP_REPEAT, GM_WRAP_REPEAT);
		value.Set(0.0f, 0.0f, 1.0f, 1.0f);
		TexCrop(value);
		TexWrapRegion(0, 0, 0, 0);
		TexLodFunc(0, 0, 0);
		lp->fRegTest[2] = 27;
		lp->fRegTex0[2] = 6;
		lp->fRegTex1[2] = 20;
		lp->fRegClamp[2] = 8;
		lp->fRegAlpha[2] = 66;
		lp->fRegAlpha[1] = 128;
		lp->fRegFog[2] = 61;
		lp->fRegTest[0] = -1;
		lp->fRegFog[0] = -1;
		lp++;
	}
	SetLayer(0);
}

void gm_graph::flush_textures()
{
	unsigned mask = fLayerEnableMask & 0xF0;
	unsigned dirty = fLayerActiveMask & mask;
	if (dirty == 0)
		return;

	int savedGen = tmGetFlushCount(gm_texture::g_texture_manager);
	bool sentOnce = false;
	int retryCount = 0;
	for (;;) {
		gm_layer *l = &fLayers[0];
		int layer = 0;
		do {
			if (((16 << layer) & dirty) != 0) {
				int texId = l->fTexId;
				if (texId != 0) {
					int mip = 0;
					if (texId == -1) {
						fDisp.get_gs_tex0(l->fRegTex0);
					} else {
						if (!tmIsCached(texId)) {
							if (!sentOnce) {
								sentOnce = true;
								fPacket.send();
							}
							((gm_texture *)texId)->send();
						}
						tmGetGsTEX0(texId, (unsigned int *)l->fRegTex0);
						mip = tmGetMipmapLevel(texId);
					}
					if (l->fTexMaxLod != mip) {
						l->fTexMaxLod = mip;
						fLayerActiveMask |= (256 << layer);
					}
					if (fFlagsE & GM_ENABLE_TEXTURE_FIT) {
						if (texId == -1) {
							gm_window *d = &fDisp;
							d->get_nonpow2(&l->fCropScaleX);
							l->fTexW = d->m_width1;
							l->fTexH = d->m_height1;
						} else {
							tmGetGsST(texId, &l->fCropScaleX);
							l->fTexW = tmGetWidth(texId);
							l->fTexH = tmGetHeight(texId);
						}
						fLayerActiveMask |= (0x10001000 << layer);
					}
				}
			}
			layer++;
			l++;
		} while (layer < 4);

		int newGen = tmGetFlushCount(gm_texture::g_texture_manager);
		if (newGen == savedGen || fTexEnableMask == 1)
			break;
		if (++retryCount >= 2)
			break;
	}
	fFlagsA |= 0x10;
}

/* Builds the GS packet of the pipeline registers driven by fFlagsA: one
   conditional block per bit, with the space check and the padding of
   flush_vsh_state. */
void gm_graph::flush_gpu_state()
{
	unsigned f = fFlagsA;
	if (!f)
		return;

	gm_packet *pk = &fPacket;
	char *limit = (char *)((int)pk->fTagCursor & ~0xFFF);
	char *cursor = pk->fCursor;
	if (limit - cursor < fFenceHi) {
		fPacketFailA++;
		return;
	}

	int target[4];
	int *p = (int *)(cursor + 32);
	if (f & 1) {
		if (fState & 2) {
			p[0] = 0;
			p[2] = 26;
			int *q = p + 4;
			q[0] = 128;
			q[1] = 128;
			q[2] = 59;
			p = q + 4;
		} else {
			p[0] = 1;
			p[2] = 26;
			p += 4;
		}
	}

	if (f & 2) {
		int id = fColorBufferId;
		if ((unsigned)(id + 1) < 2)
			(&fDisp)->get_gs_frame(target);
		else {
			if (!tmIsCached(id))
				((gm_texture *)id)->send();
			tmGetGsFRAME(id, (unsigned int *)target);
		}
		int mask = ~fColorMaskPacked;
		p[0] = target[0];
		p[2] = 76;
		p[1] = mask;
		p += 4;
	}

	if (f & 0x02000000) {
		int id = fDepthBufferId;
		if ((unsigned)(id + 1) < 2)
			(&fDisp)->get_gs_zbuf(target);
		else {
			if (!tmIsCached(id))
				((gm_texture *)id)->send();
			tmGetGsZBUF(id, (unsigned int *)target);
		}
		int zv = -1;
		if (fFlagsE & GM_ENABLE_DEPTH_MASK)
			zv = 0;
		p[0] = target[0];
		p[1] = zv;
		p[2] = 78;
		p += 4;
	}

	if (f & 4) {
		int x0 = (int)fScissor.x;
		int xe = x0 + (int)fScissor.z;
		int y0 = (int)fScissor.y;
		int ye = y0 + (int)fScissor.w;
		int w, h;
		int id = fColorBufferId;
		if ((unsigned)(id + 1) < 2) {
			gm_window *dp = &fDisp;
			w = dp->m_width1;
			h = dp->m_height1;
		} else {
			w = tmGetWidth(id);
			h = tmGetHeight(id);
		}
		if (x0 < 0)
			x0 = 0;
		if (y0 < 0)
			y0 = 0;
		if (x0 > w)
			x0 = w;
		if (y0 > h)
			y0 = h;
		if (xe < 0)
			xe = 0;
		if (ye < 0)
			ye = 0;
		if (xe > w)
			xe = w;
		if (ye > h)
			ye = h;
		if (xe == x0)
			xe++;
		if (ye == y0)
			ye++;
		p[0] = x0 | ((xe - 1) << 16);
		p[1] = y0 | ((ye - 1) << 16);
		p[2] = 64;
		p += 4;
	}

	unsigned dither = GM_ENABLE_DITHER;
	if (f & dither) {
		unsigned long long on = (fFlagsE & dither) != 0;
		p[2] = 69;
		*(unsigned long long *)p = on;
		p += 4;
	}

	if (f & 0x01c00000) {
		unsigned e = fFlagsE;
		int t = 0;
		if (e & GM_ENABLE_ALPHA_TEST) {
			t = s_gsAlphaFuncTbl[fAlphaFuncA];
			t |= fAlphaFuncB << 4;
			t |= 0x1000;
		}
		if (e & GM_ENABLE_STENCIL_TEST) {
			t |= 0x4000;
			if (fStencilFuncB)
				t |= 0x8000;
		}
		if (e & GM_ENABLE_DEPTH_TEST)
			t |= s_gsDepthFuncTbl[fDepthFuncVal] << 16;
		else
			t |= s_gsDepthFuncTbl[7] << 16;
		p[0] = t;
		p[2] = 71;
		p += 4;
	}

	if (f & 8) {
		p[0] = (fStencilOpC >= 2);
		p[2] = 74;
		p += 4;
	}

	if (f & 0x10) {
		p[0] = 0;
		p[2] = 63;
		p += 4;
	}

	gm_packet *pkt = &fPacket;
	char *end = (char *)p;
	int wordCount = (end - cursor) >> 2;
	if (wordCount < 9) {
		char *q = cursor;
		int rem = (int)q & 0xc;
		if (rem != 0) {
			while (rem < 16) {
				*(int *)q = 0;
				q += 4;
				rem += 4;
			}
		}
		pkt->fCursor = q;
		return;
	}

	int qwc = wordCount / 4;
	char *tail = end;
	int rem = (int)tail & 0xc;
	int *hdr = (int *)cursor;
	hdr[2] = 0x11000000;
	hdr[3] = (qwc - 1) | 0x50000000;
	hdr[0] = 0;
	hdr[1] = 0;
	hdr[4] = (qwc - 2) | 0x8000;
	hdr[5] = 0x10000000;
	hdr[6] = 14;
	hdr[7] = 0;
	if (rem != 0) {
		while (rem < 16) {
			*(int *)tail = 0;
			tail += 4;
			rem += 4;
		}
	}
	int size = tail - pkt->fCursor;
	pkt->fCursor = tail;
	pkt->put_packet((int *)cursor, size);
}

/* Sends the VU1 state: write_vectors, write_matrices and write_layers into the
   packet, then the tags 0x6C008000 / 0x14000004 and padding to 16 bytes
   (fewer than 5 words: padding only, nothing is sent). */
void gm_graph::flush_vsh_state()
{
	gm_packet *pk = &fPacket;
	char *limit = (char *)((int)pk->fTagCursor & ~0xFFF);
	char *cursor = pk->fCursor;
	if (limit - cursor < fFenceLo) {
		fPacketFailA++;
		return;
	}

	int *dst = (int *)(cursor + 16);
	dst = write_vectors(dst);
	dst = write_matrices(dst);
	dst = write_layers(dst);
	char *end = (char *)dst;

	int wordCount = (end - cursor) >> 2;
	if (wordCount < 5) {
		char *p = cursor;
		int rem = (int)p & 0xC;
		if (rem != 0) {
			while (rem < 16) {
				*(int *)p = 0;
				p += 4;
				rem += 4;
			}
		}
		pk->fCursor = p;
		return;
	}

	wordCount += 4;
	int qwc = wordCount / 4;
	int tagBase = 0x6C008000;
	int endTag = 0x14000004;
	char *tail = end + 32;
	int tailRem = (int)tail & 0xC;
	int tag0 = ((qwc - 1) << 16) | tagBase;

	*(int *)end = 0x8000;
	*(int *)cursor = 0;
	*(int *)(cursor + 4) = 0;
	*(int *)(cursor + 8) = 0;
	*(int *)(cursor + 12) = tag0;
	*(int *)(end + 16) = 0;
	*(int *)(end + 20) = 0;
	*(int *)(end + 24) = 0;
	*(int *)(end + 28) = endTag;

	if (tailRem != 0) {
		while (tailRem < 16) {
			*(int *)tail = 0;
			tail += 4;
			tailRem += 4;
		}
	}

	int size = tail - pk->fCursor;
	pk->fCursor = tail;
	fPacket.put_packet((int *)cursor, size);
}

int *gm_graph::write_vectors(int *dst)
{
	unsigned f = fFlagsB;
	if (!f)
		return dst;

	if ((f & 0x20000) && (fState & 2)) {
		dst = gmPutV4(dst, 1, 2, *(const vec4 *)&s_vuConst2);
		dst = gmPutV4(dst, 1, 3, *(const vec4 *)&s_vuConst3);
		dst = gmPutV4(dst, 1, 4, *(const vec4 *)&s_vuConst3);
		dst = gmPutV4(dst, 1, 7, *(const vec4 *)&s_vuConst7);
		dst = gmPutM4(dst, 2, 8, *(const mat4 *)&s_vuMat8);
		dst = gmPutM4(dst, 2, 96, s_unitMatrix);
		dst = gmPutM4(dst, 2, 12, *(const mat4 *)&s_vuMatPair[0]);
		dst = gmPutM4(dst, 2, 16, *(const mat4 *)&s_vuMatPair[1]);
		int addr = 128;
		int cmd = 1;
		int i = 3;
		do {
			dst = gmPutV4(dst, cmd, addr, *(const vec4 *)&s_vuConst128);
			addr += 16;
		} while (--i >= 0);
	}

	if ((int)f < 0) {
		int nv = fVshFlags;
		signed char t = fLitLayerMask & fSpecularMask & fColorGenMask;
		int cond = (t | (fTexGenMask & fTexEnableMask)) & fLayerEnableMask;
		int old = nv;
		if (cond)
			nv |= 2;
		else
			nv &= ~2;
		if (old != nv) {
			fVshFlags = nv;
			f |= 0xdf;
		}
	}

	if (f & 0xdf) {
		int tmp[4];
		tmp[0] = fFlagsE;
		tmp[1] = fVshFlags;
		tmp[3] = fVertexGenMode;
		float c = 1.0f;
		if (!(fFlagsE & GM_ENABLE_COLOR_2X))
			c = 0.502f;
		*(float *)&tmp[2] = c;
		dst = gmPutV4(dst, 1, 32, *(const vec4 *)tmp);
	}
	if (f & 0x40000)
		dst = gmPutV4(dst, 1, 38, fAmbient);
	if (f & 0x80000)
		dst = gmPutV4(dst, 1, 33, fColor);
	if (f & 0x100000)
		dst = gmPutV4(dst, 1, 34, fDiffuse);
	unsigned spec = f & 0x200000;
	if (spec) {
		dst = gmPutV4(dst, 1, 35, fSpecular);
		dst = gmPutV4(dst, 1, 39, *(const vec4 *)&fSpecularExtraA);
	}
	if (f & 0x400000)
		dst = gmPutV4(dst, 1, 37, fEmission);
	if (f & 0x800000) {
		dst = gmPutV4(dst, 1, 36, fTexFactor);
		if (!spec)
			dst = gmPutV4(dst, 1, 39, *(const vec4 *)&fSpecularExtraA);
	}
	if (f & 0x1000000) {
		for (int i = 0; i < fMorphCount; i += 4)
			dst = gmPutV4(dst, 1, 50 + i, *(const vec4 *)&fMorphWeights[i]);
	}
	if (f & 0x2000000) {
		for (int i = 0; i < fBlendCount; i++)
			dst = gmPutM4(dst, 2, 104 + i * 4, fBlendMatrices[i]);
	}
	if (f & 0x4000000) {
		dst = gmPutV4(dst, 1, 44, fParticleVec);
		dst = gmPutV4(dst, 1, 45, fParticleCrop);
	}
	if (f & 0x8000000)
		dst = gmPutV4(dst, 1, 46, fPatchVec);
	if (f & 0x10000000)
		dst = gmPutV4(dst, 1, 47, fPatchSubdiv);
	if (f & 0x20000000) {
		float sx = fPatchClip.z;
		if (sx != 0.0f)
			sx = fPatchCrop.z / sx;
		float sy = fPatchClip.w;
		if (sy != 0.0f)
			sy = fPatchCrop.w / sy;
		vec4 t(fPatchCrop.x - fPatchClip.x * sx, fPatchCrop.y - fPatchClip.y * sy, sx, sy);
		dst = gmPutV4(dst, 1, 48, fPatchClip);
		dst = gmPutV4(dst, 1, 49, t);
	}
	return dst;
}

int *gm_graph::write_matrices(int *dst)
{
	unsigned f = fFlagsC;
	if (!f)
		return dst;

	gm_light *lp;
	int on;
	const vec4 *src;

	if (f & 0xf) {
		if (f & 1) {
			mat4_stack *e = &fMatrixStack[0];
			dst = gmPutM4(dst, 2, 64, *gmTop(e));
		}
		if (f & 2) {
			mat4_stack *e = &fMatrixStack[1];
			const mat4 *m = gmTop(e);
			dst = gmPutM4(dst, 2, 68, *m);
			float nx = -m->m3.x;
			float ny = -m->m3.y;
			float nz = -m->m3.z;
			vec4 t(m->m0.x * nx + m->m0.y * ny + m->m0.z * nz, m->m1.x * nx + m->m1.y * ny + m->m1.z * nz,
			    m->m2.x * nx + m->m2.y * ny + m->m2.z * nz, 1.0f);
			dst = gmPutV4(dst, 1, 42, t);
		}
		if (f & 4) {
			mat4_stack *e = &fMatrixStack[2];
			dst = gmPutM4(dst, 2, 72, *gmTop(e));
		}
		if (f & 8) {
			mat4_stack *e = &fMatrixStack[3];
			const mat4 *m = gmTop(e);
			float depthMax = (float)((1 << fDisp.m_zbufAlign) - 1);
			float fogNear = fFogNear;
			float fogFar = fFogFar;
			float ratio = (fogNear * 255.0f) / (fogFar - fogNear);
			float m0x = m->m0.x;
			float m1y = m->m1.y;
			float m2z = m->m2.z;
			float m3x = m->m3.x;
			float m3y = m->m3.y;
			float m3z = m->m3.z;
			vec4 a(m0x, m1y, m2z * -depthMax, ratio * fogFar);
			vec4 b(m3x + fPixelOffXAdj, m3y + fPixelOffYAdj, m3z * -depthMax + depthMax, -ratio);
			dst = gmPutV4(dst, 1, 40, a);
			dst = gmPutV4(dst, 1, 41, b);
		}
	}

	if (f & 0xf0) {
		int addr = 137;
		for (int i = 0; i < 4; i++) {
			if ((16 << i) & f) {
				mat4_stack &me =
				    *(mat4_stack *)((char *)this + i * 96 + 1056);  /* fMatrixStack[4 + i] */
				dst = gmPutM4(dst, 2, addr, *gmTop(&me));
			}
			addr += 16;
		}
	}

	if (f & 0xf0000000) {
		int addr = 192;
		for (int i = 0; i < 4; i++) {
			if ((0x10000000u << i) & f) {
				lp = &fLights[i];
				on = 0;
				src = &s_lightDefaultVec;
				if (fFlagsE & ((unsigned)GM_ENABLE_LIGHT0 << i)) {
					on = lp->fType;
					src = &lp->fColor;
				}
				/* the VU reads the two w words as int flags */
				*(int *)&lp->fParams.m2.w = (on != 0);
				*(int *)&lp->fParams.m3.w = ((on ^ 1) != 0);
				dst = gmPutV4(dst, 1, 80 + i, *src);
				dst = gmPutM4(dst, 2, addr, lp->fParams);
				addr += 4;
				fLightDir[0][i] = -lp->fParams.m3.x;
				fLightDir[1][i] = -lp->fParams.m3.y;
				fLightDir[2][i] = -lp->fParams.m3.z;
				fLightDir[3][i] = 0;
			}
		}
		dst = gmPutM4(dst, 2, 76, *(const mat4 *)fLightDir);
	}

	if (f & 0xf000000f) {
		dst[0] = 3;
		dst += 4;
	}
	return dst;
}

/* For each of the 4 layers fLayerActiveMask holds 8 bits, one
   per GS register to resend, spaced by 4 (hence the guard mask 0x11111111).
   Each record is a gmPutV4 (cmd 1, VU address 128 + 16 * layer + k) of the
   register qword, recomputed where needed (TEST/TEX1/TEX0/CLAMP/ALPHA/FOG),
   copied for COLORGEN/TEXGEN and scaled for the crop. */
int *gm_graph::write_layers(int *dst)
{
	int mask = fLayerActiveMask;
	if (!mask)
		return dst;

	if (mask & 0xf)
		fTestBits = (fFlagsE >> 13) & 0xf8;

	gm_layer *l = fLayers;
	vec4 tmp;
	int addr = 128;
	for (int i = 0; i < 4; i++) {
		if (mask & 0x11111111) {
			if (mask & 0x1) {
				int e = fTestBits;
				if (!l->fTexId)
					e &= ~0x10;
				if (e != l->fRegTest[0]) {
					l->fRegTest[0] = e;
					float c = 1.0f;
					if (e & 0x10)
						c = 0.502f;
					*(float *)&l->fRegTest[1] = c;
					dst = gmPutV4(dst, 1, addr + 1, *(const vec4 *)l->fRegTest);
				}
			}
			if (mask & 0x10) {
				l->fRegTex0[1] = (l->fRegTex0[1] & ~0x1c) | (l->fTexFuncB << 2) | (l->fTexFuncA << 3);
				dst = gmPutV4(dst, 1, addr + 2, *(const vec4 *)l->fRegTex0);
			}
			if (mask & 0x100) {
				*(long long *)l->fRegTex1 = (long long)l->fTexLodA | ((long long)l->fTexMaxLod << 2)
				    | ((long long)l->fTexFilterA << 5) | ((long long)l->fTexFilterB << 6) | 512
				    | ((long long)l->fTexLodB << 19) | ((long long)l->fTexLodC << 32);
				dst = gmPutV4(dst, 1, addr + 3, *(const vec4 *)l->fRegTex1);
			}
			if (mask & 0x1000) {
				int wa = l->fTexWrapA;
				int wb = l->fTexWrapB;
				int minu, maxu, minv, maxv;
				if (fFlagsE & GM_ENABLE_TEXTURE_FIT) {
					if (l->fTexId) {
						if (wa == GM_WRAP_CLAMP)
							wa = GM_WRAP_REGION_CLAMP;
						if (wb == GM_WRAP_CLAMP)
							wb = GM_WRAP_REGION_CLAMP;
						maxu = l->fTexW - 1;
						maxv = l->fTexH - 1;
						*(long long *)l->fRegClamp = (long long)wa | ((long long)wb << 2)
						    | ((long long)maxu << 14) | ((long long)maxv << 34);
					}
				} else {
					if (wa >= GM_WRAP_REGION_CLAMP) {
						minu = l->fTexWrapRegionA;
						maxu = l->fTexWrapRegionC - 1;
						if (wa == GM_WRAP_REGION_CLAMP)
							maxu += minu;
					}
					if (wb >= GM_WRAP_REGION_CLAMP) {
						minv = l->fTexWrapRegionB;
						maxv = l->fTexWrapRegionD - 1;
						if (wb == GM_WRAP_REGION_CLAMP)
							maxv += minv;
					}
					*(long long *)l->fRegClamp = (long long)wa | ((long long)wb << 2)
					    | ((long long)minu << 4) | ((long long)maxu << 14) | ((long long)minv << 24)
					    | ((long long)maxv << 34);
				}
				dst = gmPutV4(dst, 1, addr + 4, *(const vec4 *)l->fRegClamp);
			}
			if (mask & 0x10000) {
				int b = l->fBlendSrc;
				int v = 0;
				if (b >= 0) {
					v = s_blendCoef[b & 0xf];
					v |= (b >> 4) & 0x30;
					if (b & GM_BLEND_SWAP)
						v ^= ~(v >> 1) & 0x45;
				}
				l->fRegAlpha[0] = v;
				dst = gmPutV4(dst, 1, addr + 5, *(const vec4 *)l->fRegAlpha);
			}
			if (mask & 0x110000) {
				int fog = 0;
				if (l->fBlendSrc < 2)
					fog = fFogColorPacked;
				if (fog != l->fRegFog[0]) {
					l->fRegFog[0] = fog;
					dst = gmPutV4(dst, 1, addr + 6, *(const vec4 *)l->fRegFog);
				}
			}
			if (mask & 0x1000000)
				dst = gmPutV4(dst, 1, addr + 7, *(const vec4 *)&l->fColorGenMode);
			if (mask & 0x10000000) {
				if (fFlagsE & GM_ENABLE_TEXTURE_FIT) {
					float sx = l->fCropScaleX;
					float sy = l->fCropScaleY;
					tmp.x = l->fTexCrop.x * sx;
					tmp.y = l->fTexCrop.y * sy;
					tmp.z = l->fTexCrop.z * sx;
					tmp.w = l->fTexCrop.w * sy;
					dst = gmPutV4(dst, 1, addr + 8, tmp);
				} else {
					dst = gmPutV4(dst, 1, addr + 8, l->fTexCrop);
				}
			}
		}
		mask >>= 1;
		addr += 16;
		l++;
	}
	return dst;
}

static int gmLoadArchiveEntry(const char *path, const char *name, void **out, int *size)
{
	FILE *f;
	gmArcHeader hdr;
	gmArcEntry ent;
	char *names = 0;
	int i;

	*out = 0;
	*size = 0;
	f = fopen(path, "rb");
	if (!f)
		return 0;

	fread(&hdr, 52, 1, f);
	fseek(f, hdr.entSize * hdr.nameIndex + hdr.dirOffset, SEEK_SET);
	fread(&ent, 40, 1, f);
	names = new char[ent.dataSize];
	fseek(f, ent.dataOffset, SEEK_SET);
	fread(names, ent.dataSize, 1, f);

	for (i = 0; i < hdr.count; i++) {
		fseek(f, hdr.entSize * i + hdr.dirOffset, SEEK_SET);
		fread(&ent, 40, 1, f);
		if (strcmp(names + ent.nameOffset, name) == 0) {
			int n = (ent.dataSize + 3) / 4 * 4;
			char *p;

			*size = ent.dataSize;
			p = new char[n];
			*out = p;
			/* clear the last word of the rounded-up buffer */
			*(int *)(n + (int)p - 4) = 0;
			fseek(f, ent.dataOffset, SEEK_SET);
			fread(*out, *size, 1, f);
			break;
		}
	}

	delete[] names;
	fclose(f);
	return *out != 0;
}

static img_file *gmCreateImageFile(const char *path)
{
	int *buf;
	int size;
	img_file *f;

	if (!gmReadImageFile(path, &buf, &size))
		return 0;

	f = 0;
	if (*buf == TIM2_MAGIC) {
		f = new tm2_file;
		f->load(buf, buf + size / 4);
	} else if ((*buf & 0xffff) == 0x4d42) {   /* 'BM' */
		f = new bmp_file;
		f->load(buf, buf + size / 4);
	}

	delete[] buf;
	return f;
}

inline void gm_graph::flush_state()
{
	if (fFlagsA | fFlagsB | fFlagsC | fLayerActiveMask) {
		if (fCurListOrVsh) {
			gm_gslist *p = (gm_gslist *)fCurListOrVsh;
			if (p->fStateSaved == 0) {
				p->fInitFlags = fFlagsA;
				p->fStateSaved = 1;
			}
			p->fExitFlags |= fFlagsA;
		}
		flush_textures();
		if (fFlagsB & 0x10000) {
			int shaderArg = fVertexShaderId;
			/* -2 and -3 are the two built-in vertex shaders loaded by Init. */
			if (shaderArg == -2)
				shaderArg = fResHandle0;
			else if (shaderArg == -3)
				shaderArg = fResHandle1;
			fPacket.put_list((int *)shaderArg);
		}
		flush_gpu_state();
		flush_vsh_state();
		fFlagsA = 0;
		fFlagsB = 0;
		fFlagsC = 0;
		fLayerActiveMask = 0;
	}
}

inline static int gmReadImageFile(const char *path, int **buf, int *size)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return 0;

	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	if (*size == 0) {
		fclose(f);
		return 0;
	}
	fseek(f, 0, SEEK_SET);

	int n = (*size + 3) / 4 * 4;
	char *p = new char[n];
	*buf = (int *)p;
	*(int *)(n + (int)p - 4) = 0;
	fread(p, *size, 1, f);
	fclose(f);
	return 1;
}

static inline void mat4_set_identity(mat4 &top)
{
	top.m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	top.m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	top.m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	top.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
}

inline void mat4_stack::load_identity()
{
	mat4_set_identity(fBase[fDepth]);
	fScales[fDepth].Set(1.0f, 1.0f, 1.0f, 1.0f);
	fScales[fDepth].w = 0.0f;
}

inline void mat4_stack::load_matrix(const mat4 &m)
{
	fBase[fDepth] = m;
	fDerivedValid = 0;
}

inline void mat4_stack::mult_matrix(const mat4 &m)
{
	fBase[fDepth] = fBase[fDepth] * m;
	fDerivedValid = 0;
}
