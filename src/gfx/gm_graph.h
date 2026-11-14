/*
 * gm_graph: the immediate-mode 3D renderer on top of gm_packet (DMA packets
 * to the VU1 microprogram and the GS): matrix stacks, texture layers, lights,
 * materials, display lists and vertex arrays. Setters only mark state dirty
 * (fFlags*); flush_state() sends it before drawing.
 */

#ifndef GM_GRAPH_H
#define GM_GRAPH_H

#pragma interface

#include "gm_packet.h"
#include "vec_types.h"

#include "gm_window.h"

#include "gm_types.h"
#include "gm_vertex.h"

/* Enable bits (SetEnableBits, the fFlagsE word). Bits 16..20 are the attribute bits of the GS
   PRIM register (IIP, TME, FGE, ABE, AA1) shifted left by 13 (write_layers); bits 21..25 set the
   GS DTHE, TEST and ZBUF registers; the low byte goes to the VU1 program. Bits 4 (set by
   Camera::ViewSetup) and 6 (set by reset_state) are VU1 options the code does not explain. */
#define GM_ENABLE_LIGHTING     0x00000001
#define GM_ENABLE_VERTEX_BLEND 0x00000002  /* morph weights or blend matrices (gmo_bone::draw) */
#define GM_ENABLE_CULL_FACE    0x00000004
#define GM_ENABLE_FRONT_FACE   0x00000008  /* FrontFace: with GM_ENABLE_CULL_FACE, keep the front faces */
#define GM_ENABLE_TEXTURE_FIT  0x00000020  /* texture coordinates 0..1 span the image, not its GS texture */
#define GM_ENABLE_COLOR_2X     0x00000080  /* vertex color scale 1.0 instead of 0.502 */
#define GM_ENABLE_SMOOTH_SHADE 0x00010000  /* ShadeModel: Gouraud shading (IIP) */
#define GM_ENABLE_TEXTURE      0x00020000
#define GM_ENABLE_FOG          0x00040000
#define GM_ENABLE_BLEND        0x00080000
#define GM_ENABLE_ANTIALIAS    0x00100000
#define GM_ENABLE_DITHER       0x00200000
#define GM_ENABLE_ALPHA_TEST   0x00400000
#define GM_ENABLE_STENCIL_TEST 0x00800000  /* the GS destination alpha test */
#define GM_ENABLE_DEPTH_TEST   0x01000000
#define GM_ENABLE_DEPTH_MASK   0x02000000  /* DepthMask: depth writes on */
#define GM_ENABLE_LIGHT0       0x10000000  /* EnableLight: GM_ENABLE_LIGHT0 << light index (0..3) */

/* Matrix modes (MatrixMode, GetMatrix, LoadMatrix, PushMatrix, ...): one matrix stack each. */
enum {
	GM_MATRIX_MODEL,
	GM_MATRIX_VIEW,
	GM_MATRIX_PROJECTION,
	GM_MATRIX_VIEWPORT,  /* Viewport and DepthRange */
	GM_MATRIX_TEXTURE0,  /* texture matrix of layer 0; layers 1..3 follow */
	GM_MATRIX_TEXTURE1,
	GM_MATRIX_TEXTURE2,
	GM_MATRIX_TEXTURE3
};

/* Primitive kinds of DrawArrays and DrawParticle: the GS PRIM types. */
enum {
	GM_PRIM_POINT,
	GM_PRIM_LINE,
	GM_PRIM_LINE_STRIP,
	GM_PRIM_TRIANGLE,
	GM_PRIM_TRIANGLE_STRIP,
	GM_PRIM_TRIANGLE_FAN,
	GM_PRIM_SPRITE
};

/* Compare functions of AlphaFunc, DepthFunc and StencilFunc. */
enum {
	GM_NEVER,
	GM_LESS,
	GM_EQUAL,
	GM_LEQUAL,
	GM_GREATER,
	GM_NOTEQUAL,
	GM_GEQUAL,
	GM_ALWAYS
};

/* BlendFunc modes (Cs source color, Cd destination color, As source alpha), plus the modifier
   bits; GM_BLEND_OFF turns the layer off. */
#define GM_BLEND_OFF        (-1)
#define GM_BLEND_COPY       0       /* Cs */
#define GM_BLEND_ALPHA      1       /* (Cs - Cd) * As + Cd */
#define GM_BLEND_ADD        2       /* Cs * As + Cd */
#define GM_BLEND_SUB        3       /* Cd - Cs * As */
#define GM_BLEND_KEEP       4       /* Cd: the color buffer is left as it is */
#define GM_BLEND_DEST_ALPHA 0x100   /* destination alpha instead of As */
#define GM_BLEND_FIX_ALPHA  0x200   /* the fixed alpha 1.0 instead of As */
#define GM_BLEND_SWAP       0x1000  /* swap Cs and Cd */

/* TexFunc(function, component): the GS TEX0 TFX and TCC fields. */
enum {
	GM_TEXFUNC_MODULATE,
	GM_TEXFUNC_DECAL,
	GM_TEXFUNC_HIGHLIGHT,
	GM_TEXFUNC_HIGHLIGHT2
};
enum {
	GM_TEXFUNC_RGB,  /* the alpha comes from the vertex color */
	GM_TEXFUNC_RGBA  /* the alpha comes from the texture */
};

/* TexFilter(magnification, minification): the GS TEX1 MMAG and MMIN values. */
enum {
	GM_FILTER_NEAREST,
	GM_FILTER_LINEAR,
	GM_FILTER_NEAREST_MIPMAP_NEAREST,
	GM_FILTER_NEAREST_MIPMAP_LINEAR,
	GM_FILTER_LINEAR_MIPMAP_NEAREST,
	GM_FILTER_LINEAR_MIPMAP_LINEAR
};

/* TexWrap(s, t): the GS CLAMP WMS and WMT values. */
enum {
	GM_WRAP_REPEAT,
	GM_WRAP_CLAMP,
	GM_WRAP_REGION_CLAMP,  /* clamp to TexWrapRegion */
	GM_WRAP_REGION_REPEAT
};

/* Sources of TexGen(mode, source): the vertex attribute that becomes the texture coordinates
   (the GM_VERTEX_* flag of that attribute), or the reflection vector. */
#define GM_TEXGEN_POSITION   GM_VERTEX_POSITION
#define GM_TEXGEN_NORMAL     GM_VERTEX_NORMAL
#define GM_TEXGEN_TEXCOORD   GM_VERTEX_TEXCOORD
#define GM_TEXGEN_TEXCOORD1  (2 * GM_VERTEX_TEXCOORD)  /* the second coordinate set */
#define GM_TEXGEN_REFLECTION 0x1000

/* Light types (Light). */
enum {
	GM_LIGHT_DIRECTIONAL,
	GM_LIGHT_POINT,
	GM_LIGHT_SPOT  /* LightCone */
};

/* A display list; the public API passes it around as an int handle. */
struct gm_gslist {
	/* 0x00 */ int *fHead;  /* head of the GS list */
	/* 0x04 */ short fRefCount;
	/* 0x06 */ short fStateSaved;  /* fInitFlags holds the state at the first flush of the list */
	/* 0x08 */ int fInitFlags;  /* fFlagsA bits CallList clears before the list */
	/* 0x0C */ int fExitFlags;  /* fFlagsA value CallList sets after the list */
}; /* size = 0x10 */

/* Texture and blend state of one layer (SetLayer 0..3). */
struct gm_layer {
	/* 0x00 */ int fBlendSrc;
	/* 0x04 */ int fTexId;
	/* 0x08 */ int fTexFuncA;
	/* 0x0C */ int fTexFuncB;
	/* 0x10 */ int fTexFilterA;
	/* 0x14 */ int fTexFilterB;
	/* 0x18 */ int fTexWrapA;
	/* 0x1C */ int fTexWrapB;
	/* 0x20 */ int fTexWrapRegionA;
	/* 0x24 */ int fTexWrapRegionB;
	/* 0x28 */ int fTexWrapRegionC;
	/* 0x2C */ int fTexWrapRegionD;
	/* 0x30 */ int fTexLodA;
	/* 0x34 */ int fTexLodB;
	/* 0x38 */ int fTexLodC;
	/* 0x3C */ int pad_3C[2];
	/* 0x44 */ float fCropScaleX;  /* texture ST scale (tmGetGsST) */
	/* 0x48 */ float fCropScaleY;
	/* 0x4C */ int fTexW;
	/* 0x50 */ int fTexH;
	/* 0x54 */ int fTexMaxLod;
	/* The records below are sent to the VU as one qword each (write_layers);
       word 2 is the GS register address. */
	/* 0x58 */ int fRegTest[4];   /* test bits (-1 forces a resend), color scale, 27 */
	/* 0x68 */ int fRegTex0[4];   /* TEX0_1 */
	/* 0x78 */ int fRegTex1[4];   /* TEX1_1 */
	/* 0x88 */ int fRegClamp[4];  /* CLAMP_1 */
	/* 0x98 */ int fRegAlpha[4];  /* ALPHA_1, FIX = 128 */
	/* 0xA8 */ int fRegFog[4];    /* FOGCOL (-1 forces a resend) */
	/* 0xB8 */ int fColorGenMode;
	/* 0xBC */ int fColorGenArg;
	/* 0xC0 */ int fTexGenMode;
	/* 0xC4 */ int fTexGenCode;
	/* 0xC8 */ int pad_C8[2];
	/* 0xD0 */ vec4 fTexCrop;
}; /* size = 0xE0 */

/* One light. The API sets lights 0..3; the array has room for 8. */
struct gm_light {
	/* 0x00 */ int fType;  /* GM_LIGHT_* */
	/* 0x04 */ int pad_04[3];
	/* 0x10 */ vec4 fColor;
	/* Sent to the VU as one matrix (write_matrices). m0: cone factors (x, y);
       m1: range squared, range and two more range factors; m2: position, its w
       the int flag "not directional"; m3: normalized direction, its w the int
       flag "directional". */
	/* 0x20 */ mat4 fParams;
}; /* size = 0x60 */

class gm_graph {
public:
	gm_graph();
	~gm_graph();

	int Init(int, int);
	void Term();
	void Start();
	void Finish();
	void Flush();
	void SwapBuffers();

	void SetTextureManager(int);
	int GetTextureManager();
	void SetPacketFence(int, int);
	int GetPacketFailCount();
	void SetPixelOffset(float, float);
	void GetPixelOffset(float &, float &);

	void Clear(int);
	void ClearColor(float, float, float, float);
	void ClearColor(const vec4 &);
	void ClearDepth(float);
	void ClearStencil(int);
	void Scissor(float, float, float, float);
	void Scissor(const vec4 &);

	void MatrixMode(int);
	mat4 *GetMatrix(int);
	mat4 *GetMatrix();
	void PushMatrix();
	void PushMatrix(int);
	void PushMatrix(int, const mat4 &);
	void LoadMatrix(const mat4 &);
	void LoadMatrix(int, const mat4 &);
	void MultMatrix(const mat4 &);
	void MultMatrix(int, const mat4 &);
	void PopMatrix();
	void PopMatrix(int);
	void PushIdentity();
	void PushIdentity(int);
	void LoadIdentity();
	void LoadIdentity(int);
	void Translate(float, float, float);
	void Translate(const vec4 &);
	void RotateX(float);
	void RotateY(float);
	void RotateZ(float);
	void Rotate(float, float, float, float);
	void Rotate(float, const vec4 &);
	void RotateYXZ(float, float, float);
	void RotateYXZ(const vec4 &);
	void RotateZYX(float, float, float);
	void RotateZYX(const vec4 &);
	void Scale(float, float, float);
	void Scale(const vec4 &);
	void Perspective(float, float, float, float);
	void Frustum(float, float, float, float, float, float);
	void Ortho(float, float, float, float, float, float);
	void Viewport(float, float, float, float);
	void Viewport(const vec4 &);
	void Viewport();
	void LookAt(float, float, float, float, float, float, float, float, float);
	void LookAt(const vec4 &, const vec4 &, const vec4 &);
	void SetBlendMatrices(const mat4 *, int);
	void SetBlendMatrices(const mat4 **, int);

	void DepthRange(float, float);
	void DepthFunc(int);
	void DepthMask(bool);
	void SetDepthBuffer(int);
	void SetColorBuffer(int);
	void ColorMask(int);
	void ColorMask(int, int, int, int);
	void FrontFace(int);
	void ShadeModel(int);
	void BlendFunc(int);
	void AlphaFunc(int, int);
	void StencilFunc(int, int);
	void StencilOp(int, int, int);

	void FogMode(int);
	void FogColor(float, float, float, float);
	void FogColor(const vec4 &);
	void FogRange(float, float);
	void Fog(int, const vec4 &, float, float);

	void Ambient(float, float, float, float);
	void Ambient(const vec4 &);
	void Color(float, float, float, float);
	void Color(const vec4 &);
	void Diffuse(float, float, float, float);
	void Diffuse(const vec4 &);
	void Specular(float, float, float, float, float);
	void Specular(const vec4 &, float);
	void Emission(float, float, float, float);
	void Emission(const vec4 &);
	void EnableLight(int, bool);
	void DisableLight(int, bool);
	void LightRange(int, float, float, float, float);
	void LightCone(int, float, float);
	void Light(int, int, const vec4 &, const vec4 &);
	void Light(int, int, const vec4 &, const vec4 &, const vec4 &);

	void SetLayer(int);
	void SetTexture(int);
	void DeleteTexture(int);
	void DuplicateTexture(int);
	int CreateTexture(const void *, int);
	int CreateTexture(const char *);
	int CreateTexture(const void *, int, int, int, const void *, int, int);
	int LoadTexture(int, const char *);
	int LoadTextureImage(int, const void *);
	int LoadTextureClut(int, const void *);
	int CreateColorBuffer(int, int, int);
	int CreateDepthBuffer(int, int, int);
	void DeleteColorBuffer(int);
	void DeleteDepthBuffer(int);
	void TexFunc(int, int);
	void TexFilter(int, int);
	void TexWrap(int, int);
	void TexWrapRegion(int, int, int, int);
	void TexLodFunc(int, int, int);
	void TexCrop(float, float, float, float);
	void TexCrop(const vec4 &);
	void TexFactor(float, float, float, float, float, float);
	void TexFactor(const vec4 &, float, float);
	void ColorGen(int, int);
	void TexGen(int, int);
	void VertexGen(int);
	static int GetStride(int);
	void SetEnableBits(int);

	int CreateVertexShader(const char *);
	int CreateVertexShader(const int *, int);
	void SetVertexShader(int);
	void DeleteVertexShader(int);

	void ParticleSize(float);
	void ParticleCrop(float, float, float, float);
	void ParticleCrop(const vec4 &);
	void PatchLevel(float);
	void PatchSubdiv(float, float);
	void PatchClip(float, float, float, float);
	void PatchClip(const vec4 &);
	void PatchCrop(float, float, float, float);
	void PatchCrop(const vec4 &);
	void SetMorphWeights(const float *, int);
	void SetArrays(const void *, int, int, int, int, const void *);
	void DrawArrays(int, int, int, int, const short *);
	void DrawParticle(int, int, int, int, const short *);
	void DrawMesh(int, int, int, int, int, const short *);
	void DrawRectPatch(int, int, int, int, int, int, const short *);
	void DrawText(int, int, int, const char *);

	int CreateList();
	int BeginList(int);
	int EndList(int *);
	int ClearList(int);
	void DeleteList(int);
	int DuplicateList(int);
	int CallList(int);

private:
	void flush_state();
	void flush_gpu_state();
	void flush_vsh_state();
	void flush_textures();
	void reset_state();
	void send_arrays_linear(int, int, int, int);
	void send_arrays_indexed(int, const short *, int, int);
	void draw_trifan(int, int, int, const short *);
	friend void gm_draw_arrays(gm_graph *, int, int, int, int, const short *);
	friend void gm_enable_bit(gm_graph *, int, int);
	void send_block_linear(int, int, int, int, int);
	void send_block_indexed(int, const short *, int, int, int);

	/* Write the VU1 microprogram input (vectors, matrices, layer state) at
       `dst` in the DMA packet and return the advanced pointer; flush_vsh_state
       calls them in a chain. */
	int *write_vectors(int *);
	int *write_matrices(int *);
	int *write_layers(int *);

public:
	/* The drawing code of the scene classes reads fFlagsE directly. */
	/* 0x000 */ int fState;  /* bit 0 = initialized, bit 1 = inside a frame */
	/* 0x004 */ gm_packet fPacket;
	/* 0x064 */ int pad_064;
	/* 0x068 */ gm_window fDisp;
	/* 0x0A8 */ int fFenceLo;  /* 0..1023 */
	/* 0x0AC */ int fFenceHi;  /* fFenceLo + 0x7C00 */
	/* 0x0B0 */ int fPacketFailA;
	/* 0x0B4 */ int fFenceRaw;
	/* 0x0B8 */ int fPacketFailB;
	/* 0x0BC */ float fPixelOffXRaw;
	/* 0x0C0 */ float fPixelOffYRaw;
	/* 0x0C4 */ float fPixelOffXAdj;
	/* 0x0C8 */ float fPixelOffYAdj;
	/* 0x0CC */ int fResHandle0;
	/* 0x0D0 */ int fResHandle1;
	/* 0x0D4 */ int fResHandle2;
	/* 0x0D8 */ const void *fArrayPtr;
	/* 0x0DC */ int fArrayFormat;
	/* 0x0E0 */ int fArrayStride;
	/* 0x0E4 */ int fArrayCount;
	/* 0x0E8 */ const void *fArrayEndPtr;
	/* 0x0EC */ int fArrayWeightFormat;
	/* 0x0F0 */ int fArrayWeightStride;
	/* 0x0F4 */ int fArrayFlag;
	/* 0x0F8 */ float fMorphWeights[16];
	/* 0x138 */ int fMorphCount;
	/* 0x13C */ char pad_13C[0x140 - 0x13C];
	/* 0x140 */ mat4 fBlendMatrices[5];
	/* 0x280 */ int fBlendCount;
	/* 0x284 */ char pad_284[0x28C - 0x284];
	/* 0x28C */ int fCurMatrixMode;
	/* 0x290 */ mat4_stack *fCurMatrixPtr;
	/* 0x294 */ char pad_294[0x2A0 - 0x294];
	/* 0x2A0 */ mat4_stack fMatrixStack[8];
	/* 0x5A0 */ int fCurLayerIndex;
	/* 0x5A4 */ gm_layer *fCurLayerPtr;
	/* 0x5A8 */ char pad_5A8[0x5B0 - 0x5A8];
	/* 0x5B0 */ gm_layer fLayers[4];
	/* 0x930 */ unsigned fFlagsA;  /* dirty: depth/color/stencil buffer state */
	/* 0x934 */ unsigned fFlagsB;  /* dirty: material, lights, vertex shader */
	/* 0x938 */ unsigned fFlagsC;  /* dirty: depth/fog range, lights, pixel offset */
	/* 0x93C */ unsigned fLayerActiveMask;  /* dirty: blend/texture per layer */
	/* 0x940 */ unsigned fFlagsE;  /* GM_ENABLE_* bits */
	/* 0x944 */ int fVshFlags;  /* sent to the VU: bit 1 specular or texgen, bit 3 point light */
	/* 0x948 */ int fPointLightMask;  /* GM_ENABLE_LIGHT0 << light index: not a directional light */
	/* 0x94C */ unsigned fLayerEnableMask;
	/* 0x950 */ unsigned fTexEnableMask;
	/* 0x954 */ char fLitLayerMask;  /* 0xF while GM_ENABLE_LIGHTING is set */
	/* 0x955 */ signed char fSpecularMask;  /* one bit per layer */
	/* 0x956 */ signed char fColorGenMask;  /* one bit per layer */
	/* 0x957 */ signed char fTexGenMask;  /* one bit per layer */
	/* 0x958 */ char pad_958[0x960 - 0x958];
	/* 0x960 */ float fDepthRange[6];  /* viewport x, y, w, h; DepthRange near, far */
	/* 0x978 */ char pad_978[0x980 - 0x978];
	/* 0x980 */ vec4 fScissor;
	/* 0x990 */ vec4 fClearColor;  /* w also holds the stencil test enable of ClearStencil */
	/* 0x9A0 */ float fClearDepthVal;
	/* 0x9A4 */ int fClearStencilVal;
	/* 0x9A8 */ char pad_9A8[0x9B0 - 0x9A8];
	/* 0x9B0 */ int fFogMode;
	/* 0x9B4 */ int fFogColorPacked;
	/* 0x9B8 */ float fFogNear, fFogFar;
	/* 0x9C0 */ vec4 fAmbient;
	/* 0x9D0 */ gm_light fLights[8];
	/* 0xCD0 */ vec4 fColor;
	/* 0xCE0 */ vec4 fDiffuse;
	/* 0xCF0 */ vec4 fSpecular;
	/* 0xD00 */ vec4 fEmission;
	/* 0xD10 */ vec4 fTexFactor;
	/* 0xD20 */ float fSpecularExtraA, fSpecularExtraB;
	/* 0xD28 */ float fTexFactorBias, fTexFactorBias2;
	/* 0xD30 */ int fColorBufferId;
	/* 0xD34 */ int fDepthBufferId;
	/* 0xD38 */ int fVertexShaderId;
	/* 0xD3C */ int fCurListOrVsh;
	/* 0xD40 */ int fColorMaskPacked;
	/* 0xD44 */ int fVertexGenMode;
	/* 0xD48 */ int fAlphaFuncA, fAlphaFuncB;
	/* 0xD50 */ int fDepthFuncVal;
	/* 0xD54 */ int fStencilFuncA, fStencilFuncB;
	/* 0xD5C */ int fStencilOpA, fStencilOpB, fStencilOpC;
	/* 0xD68 */ char pad_D68[0xD70 - 0xD68];
	/* 0xD70 */ vec4 fParticleVec;   /* w = particle size */
	/* 0xD80 */ vec4 fParticleCrop;
	/* 0xD90 */ vec4 fPatchVec;      /* w = patch level */
	/* 0xDA0 */ vec4 fPatchSubdiv;   /* x, y */
	/* 0xDB0 */ vec4 fPatchClip;
	/* 0xDC0 */ vec4 fPatchCrop;
	/* 0xDD0 */ float fLightDir[4][4];  /* row r, column i: minus component r of the direction of light i */
	/* 0xE10 */ int fTestBits;  /* GS PRIM attribute bits derived from fFlagsE (write_layers) */
	/* 0xE14 */ int fDrawMode;  /* 0 arrays, 1 particles, 2 rect patch: first word of the block header */
	/* 0xE18 */ int fDrawTagHi;  /* GIF tag high word of the current draw */
	/* 0xE1C */ int fDrawChunkCap;  /* vertices per block */
	/* 0xE20 */ int fDrawChunkOverlap;  /* vertices repeated across a block boundary */
	/* 0xE24 */ int fDrawPatchBasis;
	/* 0xE28 */ char pad_E28[0x8];
}; /* size = 0xE30 */

#endif
