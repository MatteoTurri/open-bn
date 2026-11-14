/*
 * Records of the IME panel renderer (textimsub.c) shared with the units that
 * draw through it.
 */
#ifndef TEXTIMSUB_H
#define TEXTIMSUB_H

/* Textured sprite: position, GS depth (always 0) and size, one GS quadword.
   textimsub_tex_opt scales x, y, w and h and turns them into 12.4 fixed
   point for textimsub_tex_dir. */
typedef struct {
	/* 0x00 */ int x;
	/* 0x04 */ int y;
	/* 0x08 */ int z;
	/* 0x0C */ unsigned short w;
	/* 0x0E */ unsigned short h;
} __attribute__((aligned(16))) TexSprite; /* size = 0x10 */

/* Texture source rectangle: origin and extent, in texels 12.4 fixed point. */
typedef struct {
	/* 0x00 */ unsigned short u;
	/* 0x02 */ unsigned short v;
	/* 0x04 */ unsigned short w;
	/* 0x06 */ unsigned short h;
} TexUv; /* size = 0x8 */

/* A sprite of a panel: its rectangle and texture rectangle, as indices into the tables of the
 * panel, and its color (the keyboard decorations, the pieces of the candidate list frame). */
typedef struct {
	/* 0x00 */ unsigned short rect;
	/* 0x02 */ unsigned short tex;
	/* 0x04 */ unsigned int color;
} SkbDeco; /* size = 0x08 */

int textimsub_init(void);
void textimsub_shutdown(void);
void textimsub_shutdown_noreuse(void);
void textimsub_exec_1(void);
void textimsub_exec_2(void);
void textimsub_flush(void);
void textimsub_sound(int id);

void textimsub_black(void);
void textimsub_pane(unsigned int color, int x, int y, int w, int h);
void textimsub_pane16(unsigned int color, int x, int y, int w, int h);
void textimsub_grad(unsigned int c0, unsigned int c1, int x, int y, int w, int h);

/* Sprites: `base` is the origin of the panel, `src` a rectangle relative to
   it, scaled by the screen factors of TextIM_arg. */
void textimsub_tex_use(int idx);
int textimsub_texsym_use(int sym, int base);
void textimsub_tex_dir(unsigned int color, TexSprite *dst, const TexUv *uv, const TexSprite *src);
void textimsub_tex_opt(unsigned int color, int baseX, int baseY, const TexSprite *src, const TexUv *uv);
void textimsub_calc_opt(const TexSprite *src, int *outX, int *outY, int *outX2, int *outY2);
int textimsub_mousecheck_opt(int baseX, int baseY, const TexSprite *src);
int textimsub_poscheck_opt(int baseX, int baseY, const TexSprite *src, int px, int py, int *outDist);

/* Texture files: a raw symbol image and the TIM2 loader. */
struct TIM2_PIC_HEAD;
int textimsub_raw_sym(const char *path);
void textimsub_settim2(struct TIM2_PIC_HEAD *p);
int textimsub_parsetim2(void *buf);
int textimsub_tim2(const char *path);

#endif
