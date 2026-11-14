/*
 * Vertex records of the gm_graph vertex formats (the format argument of gm_graph::SetArrays). The
 * format is a set of GM_VERTEX_* flags.
 */

#ifndef GM_VERTEX_H
#define GM_VERTEX_H

/* Vertex format flags (gm_graph::SetArrays, GetStride). Each texture coordinate set adds
   GM_VERTEX_TEXCOORD (two floats); GM_VERTEX_16BIT halves the stride (16-bit components). */
#define GM_VERTEX_POSITION 0x0001
#define GM_VERTEX_NORMAL   0x0002
#define GM_VERTEX_COLOR    0x0004
#define GM_VERTEX_POINTSIZE 0x0008  /* one float, the point size of gm_graph::DrawParticle */
#define GM_VERTEX_TEXCOORD 0x0010
#define GM_VERTEX_16BIT    0x8000

/* The formats of the records below. */
#define GM_VERTEX_PN   (GM_VERTEX_POSITION | GM_VERTEX_NORMAL)
#define GM_VERTEX_PC   (GM_VERTEX_POSITION | GM_VERTEX_COLOR)
#define GM_VERTEX_PNC  (GM_VERTEX_POSITION | GM_VERTEX_NORMAL | GM_VERTEX_COLOR)
#define GM_VERTEX_PT   (GM_VERTEX_POSITION | GM_VERTEX_TEXCOORD)
#define GM_VERTEX_PNT  (GM_VERTEX_POSITION | GM_VERTEX_NORMAL | GM_VERTEX_TEXCOORD)
#define GM_VERTEX_PCT  (GM_VERTEX_POSITION | GM_VERTEX_COLOR | GM_VERTEX_TEXCOORD)
#define GM_VERTEX_PNCT (GM_VERTEX_POSITION | GM_VERTEX_NORMAL | GM_VERTEX_COLOR | GM_VERTEX_TEXCOORD)

/* Packed vertex color, written as one word or byte by byte (c[0] red ... c[3] alpha). */
union gmVertexColor {
	unsigned int rgba;
	unsigned char c[4];
};

/* GM_VERTEX_PN: position, normal. */
struct gmVertexPN {
	float x, y, z;
	float nx, ny, nz;
}; /* size = 0x18 */

/* GM_VERTEX_PC: position, color. */
struct gmVertexPC {
	float x, y, z;
	union gmVertexColor color;
}; /* size = 0x10 */

/* GM_VERTEX_PNC: position, normal, color. */
struct gmVertexPNC {
	float x, y, z;
	float nx, ny, nz;
	union gmVertexColor color;
}; /* size = 0x1C */

/* GM_VERTEX_PT: position, texture coordinates. */
struct gmVertexPT {
	float x, y, z;
	float u, v;
}; /* size = 0x14 */

/* GM_VERTEX_PNT: position, normal, texture coordinates. */
struct gmVertexPNT {
	float x, y, z;
	float nx, ny, nz;
	float u, v;
}; /* size = 0x20 */

/* GM_VERTEX_PCT: position, color, texture coordinates. */
struct gmVertexPCT {
	float x, y, z;
	union gmVertexColor color;
	float u, v;
}; /* size = 0x18 */

/* GM_VERTEX_PNCT: position, normal, color, texture coordinates. */
struct gmVertexPNCT {
	float x, y, z;
	float nx, ny, nz;
	union gmVertexColor color;
	float u, v;
}; /* size = 0x24 */

#endif
