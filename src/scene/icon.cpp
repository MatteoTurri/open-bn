/*
 * Icon methods (drawing, shadow pass, incremental file loading, icon cache)
 * and the ico_* loaders for the PS2 save-icon files.
 */

#include <string>

#include "gfx/vec.h"
#include "icon.h"
#include "gfx/gm_vertex.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/* ===== Icon ===== */

#include "scene/optid.h"
#include "gfx/gm_graph.h"
#include "scenefacade.h"

/* Head of the doubly linked list of all icons. */
Icon *Icon::g_top = 0;

/* The default shape: the panel (vertex format 21) and the rim (format 7). */
static gmVertexPCT s_iconDefaultShapeB[8] = {
	{ -0.25f, 0.25f, 0.05f, 0xff808080, 0.0f, 0.0f },
	{ -0.25f, -0.25f, 0.05f, 0xff808080, 0.0f, 1.0f },
	{ 0.25f, 0.25f, 0.05f, 0xff808080, 1.0f, 0.0f },
	{ 0.25f, -0.25f, 0.05f, 0xff808080, 1.0f, 1.0f },
	{ 0.25f, 0.25f, -0.05f, 0xff808080, 0.0f, 0.0f },
	{ 0.25f, -0.25f, -0.05f, 0xff808080, 0.0f, 1.0f },
	{ -0.25f, 0.25f, -0.05f, 0xff808080, 1.0f, 0.0f },
	{ -0.25f, -0.25f, -0.05f, 0xff808080, 1.0f, 1.0f },
};
static gmVertexPNC s_iconDefaultShapeA[16] = {
	{ -0.25f, 0.25f, -0.05f, -0.259f, 0.966f, 0.0f, 0xff303030 },
	{ -0.25f, 0.25f, 0.05f, -0.259f, 0.966f, 0.0f, 0xff303030 },
	{ 0.25f, 0.25f, -0.05f, 0.259f, 0.966f, 0.0f, 0xff303030 },
	{ 0.25f, 0.25f, 0.05f, 0.259f, 0.966f, 0.0f, 0xff303030 },
	{ -0.25f, -0.25f, 0.05f, -0.259f, -0.966f, 0.0f, 0xff303030 },
	{ -0.25f, -0.25f, -0.05f, -0.259f, -0.966f, 0.0f, 0xff303030 },
	{ 0.25f, -0.25f, 0.05f, 0.259f, -0.966f, 0.0f, 0xff303030 },
	{ 0.25f, -0.25f, -0.05f, 0.259f, -0.966f, 0.0f, 0xff303030 },
	{ 0.25f, 0.25f, 0.05f, 0.966f, 0.259f, 0.0f, 0xff303030 },
	{ 0.25f, -0.25f, 0.05f, 0.966f, -0.259f, 0.0f, 0xff303030 },
	{ 0.25f, 0.25f, -0.05f, 0.966f, 0.259f, 0.0f, 0xff303030 },
	{ 0.25f, -0.25f, -0.05f, 0.966f, -0.259f, 0.0f, 0xff303030 },
	{ -0.25f, 0.25f, -0.05f, -0.966f, 0.259f, 0.0f, 0xff303030 },
	{ -0.25f, -0.25f, -0.05f, -0.966f, -0.259f, 0.0f, 0xff303030 },
	{ -0.25f, 0.25f, 0.05f, -0.966f, 0.259f, 0.0f, 0xff303030 },
	{ -0.25f, -0.25f, 0.05f, -0.966f, -0.259f, 0.0f, 0xff303030 },
};

/* Background colors returned by GetOption('COL4'): [0] with no icon
   loaded, [1] for the default shape (ICON_PS1). */
static int IconColorTable[2][4] = {
	{ 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{ 0xe63c1414, 0xe63c1414, 0xe63c1414, 0xe63c1414 },
};

/* Identity matrix and w = 1 row. */
static mat4 s_iconUnitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_iconUnitRow3(0.0f, 0.0f, 0.0f, 1.0f);

/* Full-screen quad rewritten by IconBlitLayer on every pass (vertex format
   17), z always 1. */
static gmVertexPT s_iconQuad[4] = {
	{ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 1.0f, 0.0f, 1.0f },
	{ 1.0f, 0.0f, 1.0f, 1.0f, 0.0f },
	{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
};

static vec4 s_iconWorldColor;
static vec4 s_iconShadowA;
static vec4 s_iconShadowB;
static vec4 s_iconShadowC;

static int IconShadowSetup(gm_graph &g, float radius, vec4 *a, vec4 *b, vec4 *c);
static void IconBlitLayer(gm_graph &g, int colorBuf, int tex, int flip);

/* Row-vector transform: v.x * m0 + v.y * m1 + v.z * m2 + w * m3. */
static inline vec4 icon_apply(const mat4 &m, const vec4 &v, float w)
{
	return m.m0 * v.x + m.m1 * v.y + m.m2 * v.z + m.m3 * w;
}

static void deleteArrayField(void **pp, int *size);

static int IconFileStart(const string &path, int *pfd, void **pbuf, int *psize, int *pused);
static int IconFileStep(int *pfd, void **pbuf, int *psize, int *pused);
static int IconSolveQuad(float a, float b, float c, float *t0, float *dt);

Icon::Icon(Object *parent) : Object(parent)
{
	EffOptionState &os = option_state;
	os.always_mask |= EFF_LIGHTING | EFF_TEXTURE_CLAMP | EFF_TEXTURE_CROP | EFF_PIXEL_TEST | EFF_PIXEL_WRITE;

	flags = 0;
	fd = -1;
	display_list = 0;
	shadow_buffer = 0;
	texture = 0;
	load_buf = 0;
	frame = 0;
	anim_speed = 1.0f;

	basis_a[0].Set(1.0f, 0.0f, 0.0f, 0.0f);
	basis_a[1].Set(0.0f, 1.0f, 0.0f, 0.0f);
	basis_a[2].Set(0.0f, 0.0f, 1.0f, 0.0f);
	basis_a[3].Set(0.0f, 0.0f, 0.0f, 1.0f);
	basis_b[0].Set(1.0f, 0.0f, 0.0f, 0.0f);
	basis_b[1].Set(0.0f, 1.0f, 0.0f, 0.0f);
	basis_b[2].Set(0.0f, 0.0f, 1.0f, 0.0f);
	basis_b[3].Set(0.0f, 0.0f, 0.0f, 1.0f);

	gm_graph *mgr = SceneGetGraph();
	int list = mgr->CreateList();
	display_list = list;
	if (!g_top) {
		next = 0;
	} else {
		next = g_top;
		g_top->prev = this;
	}
	g_top = this;
	prev = 0;
	perspective = 1.0f;
}

/* Unlinks the icon from the global list and releases its display list,
   shadow color buffer and file descriptor. */
Icon::~Icon()
{
	Unload();
	gm_graph *g = SceneGetGraph();
	g->DeleteColorBuffer(shadow_buffer);
	g->DeleteList(display_list);
	if (prev)
		prev->next = next;
	if (next)
		next->prev = prev;
	if (g_top == this)
		g_top = next;
	if (fd >= 0)
		close(fd);
}

/*
 * Draws the icon. Phases: (1) fade from the world color and update of the
 * hide/translucent bits; (2) early exits to continue a reload or rebuild, or
 * when not ready; (3) shadow rendered into a separate color buffer
 * (ICON_SHADOW and ICON_TRANSLUCENT); (4) 'PERS' scaling of the view and projection
 * matrices (ICON_PERSPECTIVE);
 * (5) material and texture state; (6) lights (ICON_AMBIENT, ICON_LIGHTS); (7) the
 * display list or DrawShape; (8) state restore and final shadow blit.
 */
void Icon::DrawThis(gm_graph &g)
{
	s_iconWorldColor = GetWorldColor();
	float fade = s_iconWorldColor.w;
	if (fade < 0.0f)
		fade = 0.0f;
	if (1.0f < fade)
		fade = 1.0f;

	int hide = ICON_HIDDEN;
	if (visible) {
		if (!(fade < 0.01f))
			hide = 0;
	}
	if (hide != (flags & ICON_HIDDEN)) {
		flags = (flags & ~ICON_HIDDEN) | hide;
		RebuildSet();
	}

	flags &= ~ICON_TRANSLUCENT;
	if (0.99f <= fade) {
		if (flags & ICON_SHADOW_ONCE)
			flags &= ~(ICON_SHADOW | ICON_SHADOW_ONCE);
	} else if (!hide) {
		flags |= ICON_TRANSLUCENT;
	}

	if (flags & ICON_RELOAD) {
		ReloadRun(g);
		return;
	}
	if (!(flags & ICON_LOADED))
		return;
	if (flags & ICON_REBUILD) {
		RebuildRun(g);
		return;
	}
	if (!(flags & ICON_BUILT))
		return;

	if ((flags & (ICON_SHADOW | ICON_TRANSLUCENT)) == (ICON_SHADOW | ICON_TRANSLUCENT)) {
		const vec4 &sc = GetLocalScale();
		if (!IconShadowSetup(g, sqrtf(sc.x * sc.x + sc.y * sc.y + sc.z * sc.z) * 0.5f, &s_iconShadowA,
		        &s_iconShadowB, &s_iconShadowC))
			return;
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE_FIT);
		g.BlendFunc(GM_BLEND_COPY);
		g.Color(1.0f, 1.0f, 1.0f, 1.0f);
		g.Viewport(0.0f, 0.0f, 256.0f, 256.0f);
		IconBlitLayer(g, shadow_buffer, -1, 0);
		g.SetColorBuffer(-1);
		g.Viewport(0.0f, 0.0f, 640.0f, 448.0f);
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_DEPTH_TEST);
		g.DepthMask(1);
		g.Color(0.0f, 0.0f, 0.0f, 0.0f);
		g.BlendFunc(GM_BLEND_KEEP);
		IconBlitLayer(g, -1, 0, 1);
		g.StencilOp(3, 3, 3);
	}

	if (flags & ICON_PERSPECTIVE) {
		mat4 mv = *g.GetMatrix(GM_MATRIX_VIEW);
		mv.m3.z += zsort_value * (perspective - 1.0f);
		g.PushMatrix(GM_MATRIX_VIEW, mv);
		mat4 mp = *g.GetMatrix(GM_MATRIX_PROJECTION);
		mp.m0.x *= perspective;
		mp.m1.y *= perspective;
		g.PushMatrix(GM_MATRIX_PROJECTION, mp);
		g.MatrixMode(GM_MATRIX_MODEL);
	}

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT);
	g.TexWrap(GM_WRAP_CLAMP, GM_WRAP_CLAMP);
	int texbits;
	if (texture)
		texbits = g.fFlagsE | GM_ENABLE_TEXTURE;
	else
		texbits = g.fFlagsE & ~GM_ENABLE_TEXTURE;
	g.SetEnableBits(texbits);
	g.SetTexture(texture);
	g.BlendFunc(blend_mode);
	g.ShadeModel(shading & 1);
	g.SetEnableBits(g.fFlagsE & ~0x10);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_CULL_FACE);
	g.FrontFace(1);
	if ((flags & (ICON_SHADOW | ICON_TRANSLUCENT)) != ICON_TRANSLUCENT) {
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_ANTIALIAS);
		g.Color(s_iconWorldColor.x, s_iconWorldColor.y, s_iconWorldColor.z, 1.0f);
	}
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_LIGHTING);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_DEPTH_TEST);
	g.DepthMask(1);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_COLOR_2X);

	ico_conf *cf = &conf;
	if (cf->is_loaded() && (flags & (ICON_AMBIENT | ICON_LIGHTS))) {
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_LIGHTING);
		if (flags & ICON_AMBIENT)
			g.Ambient(cf->ambient);
		if (flags & ICON_LIGHTS) {
			for (int i = 0; i < 3; i++) {
				const vec4 &col = cf->light_color[i];
				vec4 dir = icon_apply(*(const mat4 *)basis_a, cf->light_dir[i], 0.0f);
				g.EnableLight(i, 1);
				g.Light(i, GM_LIGHT_DIRECTIONAL, col, dir);
			}
		}
	}

	if (flags & ICON_PS1)
		g.MultMatrix(*(const mat4 *)basis_b);
	if (flags & ICON_MORPH)
		Animate(g);

	if ((flags & (ICON_SHADOW | ICON_PS1)) != ICON_SHADOW) {
		if (!g.CallList(display_list))
			DrawShape(g);
	} else {
		unsigned bits = g.fFlagsE;
		g.SetEnableBits((bits | GM_ENABLE_ANTIALIAS) & ~GM_ENABLE_DEPTH_MASK);
		g.CallList(display_list);
		g.SetEnableBits((bits & ~GM_ENABLE_ANTIALIAS) | GM_ENABLE_DEPTH_MASK);
		g.CallList(display_list);
	}

	if (flags & ICON_PERSPECTIVE) {
		g.PopMatrix(GM_MATRIX_VIEW);
		g.PopMatrix(GM_MATRIX_PROJECTION);
		g.MatrixMode(GM_MATRIX_MODEL);
	}
	g.ShadeModel(1);
	g.SetEnableBits(g.fFlagsE | 0x10);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_CULL_FACE);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_ANTIALIAS);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_COLOR_2X);

	if ((flags & (ICON_SHADOW | ICON_TRANSLUCENT)) == (ICON_SHADOW | ICON_TRANSLUCENT)) {
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_LIGHTING);
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_DEPTH_TEST);
		g.DepthMask(0);
		g.TexCrop(0.0f, 0.0f, 1.0f, 1.0f);
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_STENCIL_TEST);
		g.StencilFunc(GM_EQUAL, 128);
		g.StencilOp(1, 1, 1);
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT);
		g.BlendFunc(GM_BLEND_ALPHA);
		g.TexWrap(GM_WRAP_CLAMP, GM_WRAP_CLAMP);
		g.Color(1.0f, 1.0f, 1.0f, 1.0f - fade);
		IconBlitLayer(g, -1, shadow_buffer, 1);
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_STENCIL_TEST);
	}
}

/* Draws the loaded custom model (vertex format 23, triangles), or the
   default two-part shape for ICON_PS1. */
void Icon::DrawShape(gm_graph &g)
{
	if (!(flags & ICON_PS1)) {
		g.SetArrays(vertices, GM_VERTEX_PNCT, vertex_count, num_shapes, GM_VERTEX_16BIT | GM_VERTEX_POSITION,
		    0);
		g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 3, vertex_count / 3, 0);
		return;
	}

	g.SetEnableBits(g.fFlagsE | GM_ENABLE_LIGHTING);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE);
	g.SetArrays(s_iconDefaultShapeA, GM_VERTEX_PNC, 16, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 4, 0);

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_LIGHTING);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
	g.SetArrays(s_iconDefaultShapeB, GM_VERTEX_PCT, 8, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 2, 0);
}

/* Rebuild state machine driven by flags and rebuild_state. Returns 1 to be
   called again, 0 when done (clearing ICON_REBUILD). */
int Icon::RebuildRun(gm_graph &g)
{
	int r;
	int f = flags;

	if (!(f & ICON_LOADED))
		goto done;
	if (f & ICON_HIDDEN)
		goto done;
	if (f & ICON_PS1) {
		r = g.CreateTexture(&shape_tex, 20, 16, 48, &shape_clut, 2, 0);
		texture = r;
		goto check;
	}
	switch (rebuild_state) {
	case ICON_BUILD_TEXTURE:
		if (file.texture) {
			const void *p = file.texture->buffer;
			r = g.CreateTexture(p, 2, 128, 128, 0, 0, 0);
			texture = r;
			if (!r)
				goto done;
		}
		rebuild_state = rebuild_state + 1;
		goto in_progress;
	case ICON_BUILD_LIST:
		g.BeginList(display_list);
		DrawShape(g);
		r = g.EndList(0);
		goto check;
	}
	goto in_progress;
check:
	if (r)
		flags |= ICON_BUILT;
	goto done;
in_progress:
	return 1;
done:
	flags &= ~ICON_REBUILD;
	return 0;
}

/* Starts loading a new icon file: resets the state and drops the old data. */
void Icon::ReloadSet(const String &name)
{
	flags |= ICON_RELOAD;
	file_name = name;
	reload_state = ICON_LOAD_START;
	flags &= ~(ICON_LOADED | ICON_PS1);
	flags &= ~(ICON_MODEL | ICON_MORPH | ICON_TEXTURED);
	conf.unload();
	file.unload(ICO_PART_ALL);
	deleteArrayField((void **)&load_buf, &bytes_read);
	flags &= ~ICON_BUILT;
	gm_graph *mgr = SceneGetGraph();
	mgr->DeleteTexture(texture);
	texture = 0;
	mgr->ClearList(display_list);
}

/* Load state machine on reload_state. Returns 1 to be called again, 0 when
   done (clearing ICON_RELOAD and freeing the load buffer). */
int Icon::ReloadRun(gm_graph &g)
{
	switch (reload_state) {
	case ICON_LOAD_START: {
		Icon *p = FindCache();
		if (p) {
			if (p->flags & ICON_PS1) {
				shape_frames = p->shape_frames;
				shape_tex = p->shape_tex;
				shape_clut = p->shape_clut;
				reload_state = ICON_LOAD_PS1_DONE;
				break;
			}
			conf.copy(p->conf);
			file.copy(p->file);
			reload_state = ICON_LOAD_DONE;
			break;
		}
		if (!IconFileStart(file_name, &fd, (void **)&load_buf, &bytes_read, &bytes_expected))
			goto done;
		reload_state = ICON_LOAD_READ;
		return 1;
	}
	case ICON_LOAD_READ: {
		int magic;
		if (IconFileStep(&fd, (void **)&load_buf, &bytes_read, &bytes_expected))
			break;
		if (bytes_expected < bytes_read)
			goto done;
		parse_cur = load_buf;
		parse_end = load_buf + (bytes_read + 3) / 4;
		magic = *load_buf;
		if ((magic & 0xffff) == ICO_PS1_MAGIC) {
			reload_state = ICON_LOAD_PS1;
			break;
		}
		if (magic != ICO_SYS_MAGIC)
			goto done;
		reload_state = ICON_LOAD_SYS;
		break;
	}
	case ICON_LOAD_PS1:
		shape_frames = 0;
		if (bytes_read >= 5) {
			int n = ((short *)load_buf)[1];
			shape_frames = n & 3;
		}
		if (bytes_read < (shape_frames << 7) + 128)
			goto done;
		memcpy(&shape_tex, (char *)load_buf + 128, shape_frames << 7);
		shape_clut = *(clut_data *)(load_buf + 24);
		reload_state = reload_state + 1;
		break;
	case ICON_LOAD_PS1_DONE:
		shading = 7;
		flags |= ICON_MODEL | ICON_MORPH | ICON_TEXTURED | ICON_LOADED | ICON_PS1;
		anim_length = (float)shape_frames;
		if (shape_frames != 2)
			anim_step = 0.1f;
		else
			anim_step = 0.0666666701f;
		goto done;
	case ICON_LOAD_SYS:
		parse_cur = conf.load(parse_cur, parse_end);
		if (!parse_cur)
			goto done;
		reload_state = reload_state + 1;
		break;
	case ICON_LOAD_MODEL:
		parse_cur = file.load(parse_cur, parse_end, ICO_PART_MODEL);
		if (!parse_cur)
			goto done;
		reload_state = reload_state + 1;
		break;
	case ICON_LOAD_MOTION:
		parse_cur = file.load(parse_cur, parse_end, ICO_PART_MOTION);
		if (!parse_cur)
			goto done;
		reload_state = reload_state + 1;
		break;
	case ICON_LOAD_TEXTURE:
		parse_cur = file.load(parse_cur, parse_end, ICO_PART_TEXTURE);
		if (!parse_cur)
			goto done;
		reload_state = reload_state + 1;
		break;
	case ICON_LOAD_DONE: {
		ico_file *f = &file;
		ico_model *m = f->model;
		shading = m->attrib;
		vertices = m->buffer;
		vertex_count = m->vertex_count;
		num_shapes = m->count;
		flags |= ICON_MODEL;
		ico_motion *mo = f->motion;
		anim_length = (float)mo->frame_length;
		anim_step = mo->speed;
		if (num_shapes >= 2)
			flags |= ICON_MORPH;
		if (f->texture)
			flags |= ICON_TEXTURED;
		flags |= ICON_LOADED;
		goto done;
	}
	}
	return 1;
done:
	flags &= ~ICON_RELOAD;
	deleteArrayField((void **)&load_buf, &bytes_read);
	return 0;
}

void Icon::Animate(gm_graph &g)
{
	if (!(flags & ICON_PS1)) {
		float local[8];
		file.motion->get_weights(local, num_shapes, frame);
		g.SetMorphWeights(local, num_shapes);
	} else {
		g.TexWrap(GM_WRAP_REGION_CLAMP, GM_WRAP_REGION_CLAMP);
		int s0 = (int)frame;
		g.TexWrapRegion(0, s0 << 4, 16, 16);
		float f1 = (float)s0 * 0.25f;
		g.TexCrop(0.0f, f1, 1.0f, 0.25f);
	}
	float f0 = frame + anim_step * anim_speed;
	frame = f0;
	if (anim_length <= f0)
		frame = 0.0f;
}

void Icon::Unbuild()
{
	flags &= ~ICON_BUILT;
	gm_graph *mgr = SceneGetGraph();
	mgr->DeleteTexture(texture);
	texture = 0;
	mgr->ClearList(display_list);
}

void Icon::RebuildSet()
{
	flags = (flags | ICON_REBUILD) & ~ICON_BUILT;
	rebuild_state = ICON_BUILD_TEXTURE;
	gm_graph *mgr = SceneGetGraph();
	mgr->DeleteTexture(texture);
	texture = 0;
	mgr->ClearList(display_list);
}

void Icon::Unload()
{
	flags &= ~(ICON_LOADED | ICON_PS1);
	flags &= ~(ICON_MODEL | ICON_MORPH | ICON_TEXTURED);
	conf.unload();
	file.unload(ICO_PART_ALL);
	deleteArrayField((void **)&load_buf, &bytes_read);
	flags &= ~ICON_BUILT;
	gm_graph *mgr = SceneGetGraph();
	mgr->DeleteTexture(texture);
	texture = 0;
	mgr->ClearList(display_list);
}

/* Returns another loaded icon with the same file name, or 0. */

Icon *Icon::FindCache()
{
	if (file_name.compare("") == 0)
		return 0;

	for (Icon *p = g_top; p; p = p->next) {
		if (p != this && p->file_name.compare(file_name) == 0 && (p->flags & ICON_LOADED))
			return p;
	}
	return 0;
}

int Icon::SetOption(int option, const int &value, unsigned int size)
{
	switch (option) {
	case OPT_SYNC:
		flags = value ? flags | ICON_SYNC : flags & ~ICON_SYNC;
		return 1;
	case OPT_AMBS:
		if (value) {
			flags |= ICON_AMBIENT;
			GetOptionState()->always_mask |= EFF_AMBIENT;
		} else {
			flags &= ~ICON_AMBIENT;
			GetOptionState()->always_mask &= ~EFF_AMBIENT;
		}
		return 1;
	case OPT_LITS:
		if (value) {
			flags |= ICON_LIGHTS;
			GetOptionState()->always_mask |= EFF_LIGHT_RELOAD;
		} else {
			/* NOTE: bug: clears the 'AMBS' bit 0x02000000 instead of 0x04000000. */
			flags &= ~ICON_AMBIENT;
			GetOptionState()->always_mask &= ~EFF_LIGHT_RELOAD;
		}
		return 1;
	case OPT_FBUF: {
		gm_graph *g = SceneGetGraph();
		if (value) {
			flags |= ICON_SHADOW;
			if (value >= 2)
				flags |= ICON_SHADOW_ONCE;
			if (shadow_buffer == 0)
				shadow_buffer = g->CreateColorBuffer(256, 256, 0);
		} else {
			flags &= ~ICON_SHADOW;
			g->DeleteColorBuffer(shadow_buffer);
			shadow_buffer = 0;
		}
		return 1;
	}
	}
	return Object::SetOption(option, value, 1);
}

int Icon::SetOption(int option, const float &value, unsigned int size)
{
	int nf;

	switch (option) {
	case OPT_FRAM:
		frame = value;
		return 1;
	case OPT_SPED:
		anim_speed = value;
		return 1;
	case OPT_PERS:
		if (value != 1.0f)
			nf = flags | ICON_PERSPECTIVE;
		else
			nf = flags & ~ICON_PERSPECTIVE;
		flags = nf;
		perspective = value;
		return 1;
	}
	return Object::SetOption(option, value, 1);
}

/* 'FILE' starts loading a new icon file; with 'SYNC' set the load and
   rebuild complete before returning. */
int Icon::SetOption(int option, const String &value)
{
	if (option == OPT_FILE) {
		ReloadSet(value);

		flags = (flags | ICON_REBUILD) & ~ICON_BUILT;
		rebuild_state = ICON_BUILD_TEXTURE;
		gm_graph *mgr = SceneGetGraph();
		mgr->DeleteTexture(texture);
		texture = 0;
		mgr->ClearList(display_list);

		if (flags & ICON_SYNC) {
			gm_graph *g = SceneGetGraph();
			while (ReloadRun(*g))
				;
			if (!(flags & ICON_HIDDEN)) {
				while (RebuildRun(*g))
					;
			}
		}
		return 1;
	}
	return Object::SetOption(option, value);
}

/* 'ROTL'/'ROT1' take three rotation angles (radians), 'POS1' a position,
   'FILE' a C string forwarded to the String overload. Rotations compose
   as Y * X * Z. */
int Icon::SetOption(int option, const void *data, unsigned int size)
{
	switch (option) {
	case OPT_ROTL: {
		const float *f = (const float *)data;
		float ax = f[0], ay = f[1], az = f[2];
		*(mat4 *)basis_a = mat4_rotate_y(ay) * mat4_rotate_x(ax) * mat4_rotate_z(az);
		return 1;
	}
	case OPT_ROT1: {
		const float *f = (const float *)data;
		/* Only the first three rows. */
		mat4 m = mat4_rotate_y(f[1]) * mat4_rotate_x(f[0]) * mat4_rotate_z(f[2]);
		*(vec4 *)&basis_b[0] = m.m0;
		*(vec4 *)&basis_b[1] = m.m1;
		*(vec4 *)&basis_b[2] = m.m2;
		return 1;
	}
	case OPT_POS1: {
		const float *f = (const float *)data;
		basis_b[3].x = f[0];
		basis_b[3].y = f[1];
		basis_b[3].z = f[2];
		return 1;
	}
	case OPT_FILE: {
		String s((const char *)data);
		return SetOption(option, s);
	}
	}
	return Object::SetOption(option, data, 0);
}

int Icon::GetOption(int option, int &value, unsigned int size) const
{
	if (option == OPT_TYPE) {
		value = ICON_TYPE_NONE;
		int f = flags;
		if (f & ICON_LOADED)
			value = (f & ICON_PS1) ? ICON_TYPE_PS1 : ICON_TYPE_PS2;
		return 1;
	}
	return Object::GetOption(option, value, 1);
}

int Icon::GetOption(int option, float &value, unsigned int size) const
{
	if (option == OPT_FRAM) {
		value = frame;
		return 1;
	}
	return Object::GetOption(option, value, 1);
}

int Icon::GetOption(int option, void *data, unsigned int size) const
{
	if (option == OPT_COL4) {
		if (size < 16)
			return 0;
		int f = flags;
		const int *src = IconColorTable[0];
		if (f & ICON_LOADED) {
			if (f & ICON_PS1) {
				src += 4;
			} else {
				const int *c = conf.color;
				src = c;
			}
		}
		int *dst = (int *)data;
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = src[3];
		return 1;
	}
	return Object::GetOption(option, data, size);
}

/* Rewrites the blit quad from the rectangle s_iconShadowC (x, y, width,
   height) and the transform s_iconShadowA (offset, scale), then draws it in
   screen coordinates. With flip, position and texture coordinates swap: the
   pass that renders the shadow texture onto the color buffer. */
static void IconBlitLayer(gm_graph &g, int colorBuf, int tex, int flip)
{
	const vec4 &r = s_iconShadowC;
	float x0, y0, x1, y1;
	float u0, v0, u1, v1;
	int bits;

	x0 = r.x;
	y0 = r.y;
	x1 = x0 + r.z;
	y1 = y0 + r.w;
	if (!flip) {
		x0 -= 0.01f;
		y0 -= 0.01f;
		x1 += 0.02f;
		y1 += 0.02f;
	}
	u0 = s_iconShadowA.x + s_iconShadowA.z * x0;
	v0 = s_iconShadowA.y + s_iconShadowA.w * y0;
	u1 = s_iconShadowA.x + s_iconShadowA.z * x1;
	v1 = s_iconShadowA.y + s_iconShadowA.w * y1;

	if (!flip) {
		s_iconQuad[0].u = u0;
		s_iconQuad[0].v = v0;
		s_iconQuad[1].u = u0;
		s_iconQuad[1].v = v1;
		s_iconQuad[2].u = u1;
		s_iconQuad[2].v = v0;
		s_iconQuad[3].u = u1;
		s_iconQuad[3].v = v1;
		s_iconQuad[0].x = x0;
		s_iconQuad[0].y = y0;
		s_iconQuad[1].x = x0;
		s_iconQuad[1].y = y1;
		s_iconQuad[2].x = x1;
		s_iconQuad[2].y = y0;
		s_iconQuad[3].x = x1;
		s_iconQuad[3].y = y1;
	} else {
		s_iconQuad[0].u = x0;
		s_iconQuad[0].v = y0;
		s_iconQuad[1].u = x0;
		s_iconQuad[1].v = y1;
		s_iconQuad[2].u = x1;
		s_iconQuad[2].v = y0;
		s_iconQuad[3].u = x1;
		s_iconQuad[3].v = y1;
		s_iconQuad[0].x = u0;
		s_iconQuad[0].y = v0;
		s_iconQuad[1].x = u0;
		s_iconQuad[1].y = v1;
		s_iconQuad[2].x = u1;
		s_iconQuad[2].y = v0;
		s_iconQuad[3].x = u1;
		s_iconQuad[3].y = v1;
	}

	g.PushIdentity(GM_MATRIX_MODEL);
	g.PushIdentity(GM_MATRIX_VIEW);
	/* Orthographic projection filled element by element. */
	float m[4][4];
	m[0][0] = 2.0f;
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = 0.0f;
	m[1][0] = 0.0f;
	m[1][1] = -2.0f;
	m[1][2] = 0.0f;
	m[1][3] = 0.0f;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = 2.0f;
	m[2][3] = 0.0f;
	m[3][0] = -1.0f;
	m[3][1] = 1.0f;
	m[3][2] = -1.0f;
	m[3][3] = 1.0f;
	g.PushMatrix(GM_MATRIX_PROJECTION, *(const mat4 *)m);
	g.SetColorBuffer(colorBuf);
	if (tex != 0)
		bits = g.fFlagsE | GM_ENABLE_TEXTURE;
	else
		bits = g.fFlagsE & ~GM_ENABLE_TEXTURE;
	g.SetEnableBits(bits);
	g.SetTexture(tex);
	g.TexGen(0, GM_TEXGEN_TEXCOORD);
	g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGB);
	g.SetArrays(s_iconQuad, GM_VERTEX_PT, 4, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);
	g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGBA);
	g.PopMatrix(GM_MATRIX_PROJECTION);
	g.PopMatrix(GM_MATRIX_VIEW);
	g.PopMatrix(GM_MATRIX_MODEL);
}

#define ICON_SHADOW_MIN_W 0.4f
#define ICON_SHADOW_MIN_H 0.5714286f

/* From the shadow radius computes the render-target rectangle (a, with a
   minimum size and clamped to the screen), the texture transform back to
   [0,1] (b) and the projected rectangle inside the crop (c). */
static int IconShadowSetup(gm_graph &g, float radius, vec4 *a, vec4 *b, vec4 *c)
{
	vec4 p = apply(*g.GetMatrix(GM_MATRIX_VIEW), g.GetMatrix(GM_MATRIX_MODEL)->m3, 1.0f);
	float x, w, y, h;

	if (!IconSolveQuad(p.z, p.x, radius, &x, &w))
		return 0;
	if (!IconSolveQuad(p.z, -p.y, radius, &y, &h))
		return 0;

	const mat4 &vp = *g.GetMatrix(GM_MATRIX_PROJECTION);
	float sx = vp.m0.x * 0.5f;
	float sy = vp.m1.y * 0.5f;
	x = x * sx + 0.5f;
	w = w * sx;
	y = y * sy + 0.5f;
	h = h * sy;

	float bx = x, by = y, bw = w, bh = h;
	if (bw < ICON_SHADOW_MIN_W)
		bw = ICON_SHADOW_MIN_W;
	if (bh < ICON_SHADOW_MIN_H)
		bh = ICON_SHADOW_MIN_H;
	if (bx < 0.0f) {
		bw += bx;
		bx = 0.0f;
		if (bw < ICON_SHADOW_MIN_W)
			bw = ICON_SHADOW_MIN_W;
	}
	if (by < 0.0f) {
		bh += by;
		by = 0.0f;
		if (bh < ICON_SHADOW_MIN_H)
			bh = ICON_SHADOW_MIN_H;
	}
	if (1.0f - bw < bx) {
		if (1.0f - ICON_SHADOW_MIN_W < bx)
			bx = 1.0f - ICON_SHADOW_MIN_W;
		bw = 1.0f - bx;
	}
	if (1.0f - bh < by) {
		if (1.0f - ICON_SHADOW_MIN_H < by)
			by = 1.0f - ICON_SHADOW_MIN_H;
		bh = 1.0f - by;
	}

	a->x = bx;
	a->y = by;
	a->z = bw;
	a->w = bh;
	b->z = 1.0f / bw;
	b->w = 1.0f / bh;
	b->x = -a->x * b->z;
	b->y = -a->y * b->w;

	c->x = (x - bx) / bw;
	c->y = (y - by) / bh;
	c->z = w / bw;
	c->w = h / bh;
	return 1;
}

/* Solves the intersection quadratic: *t0 is the smaller root, *dt the
   distance to the larger one. Returns 0 without two distinct roots. */
static int IconSolveQuad(float a, float b, float c, float *t0, float *dt)
{
	float ab = a * b;
	float aa = a * a;
	float cc = c * c;
	float qa = aa - cc;
	float qb = b * b - cc;
	float d;

	if (qa <= 0.0f)
		return 0;
	d = ab * ab - qa * qb;
	if (d <= 0.0f)
		return 0;
	d = sqrtf(d);
	qa = 1.0f / qa;
	*t0 = (-ab - d) * qa;
	*dt = (d + d) * qa;
	return 1;
}

ico_conf::ico_conf()
{
	int i;

	magic = 0;
	title = "";
	title_break = 0;
	for (i = 0; i < 3; i++)
		icon_names[i] = "";
	for (i = 0; i < 3; i++) {
		light_dir[i].Set(0.0f, 0.0f, 0.0f, 0.0f);
		light_color[i].Set(0.0f, 0.0f, 0.0f, 0.0f);
	}
	ambient.Set(0.0f, 0.0f, 0.0f, 0.0f);
	for (i = 0; i < 4; i++)
		color[i] = 0;
}

ico_conf::~ico_conf()
{
	unload();
}

void ico_conf::unload()
{
	magic = 0;
}

bool ico_conf::is_loaded() const
{
	return magic != 0;
}

ico_conf *ico_conf::copy(const ico_conf &other)
{
	magic = other.magic;
	title = other.title;
	title_break = other.title_break;
	icon_names[0] = other.icon_names[0];
	icon_names[1] = other.icon_names[1];
	icon_names[2] = other.icon_names[2];
	light_dir[0] = other.light_dir[0];
	light_dir[1] = other.light_dir[1];
	light_dir[2] = other.light_dir[2];
	light_color[0] = other.light_color[0];
	light_color[1] = other.light_color[1];
	light_color[2] = other.light_color[2];
	ambient = other.ambient;
	color[0] = other.color[0];
	color[1] = other.color[1];
	color[2] = other.color[2];
	color[3] = other.color[3];
	return this;
}

/*
 * Parses icon.sys, a fixed 964-byte record. Returns the pointer just past
 * it, or 0 if fewer than 964 bytes are available. Layout:
 *   +0    int      magic -> magic
 *   +6    short    title line-break offset -> title_break
 *   +12   int      background alpha (scaled by 255/128)
 *   +16   4 x 16   background colors (r, g, b ints) -> color[0..3]
 *   +80   3 x 16   light directions (x, -y, -z, w = 1) -> light_dir
 *   +128  3 x 16   light colors (w = 1) -> light_color
 *   +176  3 float  ambient (w = 1) -> ambient
 *   +192  string   title -> title
 *   +260  3 x 64   icon file names -> icon_names[0..2]
 */
int *ico_conf::load(int *p, int *end)
{
	if (end < p + 241)
		return 0;

	magic = p[0];
	title_break = *(short *)((char *)p + 6);
	p += 3;
	int alpha = (p[0] * 255) / 128;
	p += 1;
	int i;
	for (i = 0; i < 4; i++) {
		int r = p[0], g = p[1], b = p[2];
		color[i] = r | (g << 8) | (b << 16) | (alpha << 24);
		p += 4;
	}
	float *f = (float *)p;
	for (i = 0; i < 3; i++) {
		light_dir[i].Set(f[0], -f[1], -f[2], 1.0f);
		light_color[i].Set(f[12], f[13], f[14], 1.0f);
		f += 4;
	}
	f = (float *)p + 24;
	ambient.Set(f[0], f[1], f[2], 1.0f);
	char *s = (char *)p + 112;
	title = s;
	s = (char *)p + 180;
	for (i = 0; i < 3; i++) {
		icon_names[i] = s;
		s += 64;
	}
	return (int *)(s + 512);
}

ico_file::ico_file() : unk_00(0), model(0), motion(0), texture(0)
{}

ico_file::~ico_file()
{
	unload(ICO_PART_ALL);
}

ico_file *ico_file::copy(const ico_file &other)
{
	unk_00 = other.unk_00;
	unload(ICO_PART_ALL);

	if (other.model) {
		model = new ico_model();
		model->copy(*other.model);
	}
	if (other.motion) {
		motion = new ico_motion();
		motion->copy(*other.motion);
	}
	if (other.texture) {
		texture = new ico_texture();
		texture->copy(*other.texture);
	}
	return this;
}

void ico_file::unload(int mask)
{
	if (mask & ICO_PART_MODEL) {
		if (model)
			delete model;
		model = 0;
	}
	if (mask & ICO_PART_MOTION) {
		if (motion)
			delete motion;
		motion = 0;
	}
	if (mask & ICO_PART_TEXTURE) {
		if (texture)
			delete texture;
		texture = 0;
	}
}

int *ico_file::load(int *p, int *end, int mask)
{
	if (mask & ICO_PART_MODEL) {
		if (model)
			delete model;
		model = 0;
		if (end < p + 1)
			return 0;
		if (*p++ != ICO_FILE_MAGIC)
			return 0;
		ico_model *m = new ico_model();
		p = m->load(p, end);
		if (p == 0) {
			delete m;
			return 0;
		}
		model = m;
	}

	if (mask & ICO_PART_MOTION) {
		if (motion)
			delete motion;
		motion = 0;
		ico_motion *m = new ico_motion();
		p = m->load(p, end);
		if (p == 0) {
			delete m;
			return 0;
		}
		motion = m;
	}

	if (mask & ICO_PART_TEXTURE) {
		if (texture)
			delete texture;
		texture = 0;
		if (model == 0)
			return 0;
		int subflags = model->attrib;
		if (subflags & ICO_ATTR_TEXTURE) {
			ico_texture *m = new ico_texture();
			p = m->load(p, end, subflags);
			if (p == 0) {
				delete m;
				return 0;
			}
			texture = m;
		}
	}

	return p;
}

bool ico_file::is_loaded(int mask) const
{
	if ((mask & ICO_PART_MODEL) && model == 0)
		return false;
	if ((mask & ICO_PART_MOTION) && motion == 0)
		return false;
	if ((mask & ICO_PART_TEXTURE) && texture == 0)
		return false;
	return true;
}

ico_model::ico_model()
{
	count = 0;
	attrib = 0;
	vertex_count = 0;
	buffer = 0;
	unk_04 = 1.0f;
}

ico_model::~ico_model()
{
	if (buffer)
		delete[] (char *)buffer;
}

ico_model *ico_model::copy(const ico_model &other)
{
	attrib = other.attrib;
	unk_04 = other.unk_04;
	int cnt = other.count;
	count = cnt;
	int vertexSize = (cnt - 1) * 6 + 36;
	int nverts = other.vertex_count;
	vertex_count = nverts;
	int size = vertexSize * nverts;
	void *old = buffer;
	if (old)
		delete[] (char *)old;
	int alloc = (size + 3) / 4 * 4;
	buffer = new char[alloc];
	memcpy(buffer, other.buffer, size);
	return this;
}

#define ICO_M_POS	(1.0f / 20480.0f)
#define ICO_M_NRM	(1.0f / 4096.0f)

/*
 * Converts the icon model (short-integer vertices) into float vertices.
 * Header of 16 bytes (count, attrib, unk_04, vertex_count); count must be 1..8.
 * The buffer holds vertex_count 36-byte float vertices (shape 0) followed by
 * (count - 1) * vertex_count short triples for the other shapes, interleaved per
 * vertex. attrib & ICO_ATTR_PACKED selects the packed source format (3-short vertices,
 * 7-short trailer, reached through a short index table) over the plain one
 * (4-short vertices, 8-short trailer).
 */
int *ico_model::load(int *p, int *end)
{
	if ((char *)end < (char *)p + 16)
		return 0;

	int n = *p++;
	count = n;
	attrib = *p++;
	unk_04 = *(const float *)p;
	p++;
	vertex_count = *p++;
	if ((unsigned)(n - 1) >= 8)
		return 0;

	int size = ((n - 1) * 6 + 36) * vertex_count;
	if (buffer)
		delete[] (char *)buffer;
	int alloc = (size + 3) / 4 * 4;
	buffer = new char[alloc];

	gmVertexPNCT *dst = (gmVertexPNCT *)buffer;
	short *morphDst = (short *)((char *)buffer + vertex_count * 36);

	if (!(attrib & ICO_ATTR_PACKED)) {
		/* vsize = bytes per vertex: count 4-short shapes plus 16 bytes of
		   normal, uv and color. */
		int vsize = count * 8 + 16;
		if (end < p + vsize * vertex_count / 4)
			return 0;

		short *src = (short *)p;

		for (int i = 0; i < vertex_count; i++) {
			short *h = (short *)((char *)src + (vsize - 16));

			dst->x = src[0] * ICO_M_POS;
			dst->y = -src[1] * ICO_M_POS - 0.5f;
			dst->z = -src[2] * ICO_M_POS;
			dst->nx = h[0] * ICO_M_NRM;
			dst->ny = -h[1] * ICO_M_NRM;
			dst->nz = -h[2] * ICO_M_NRM;
			dst->color.rgba = *(int *)(h + 6) | 0xff000000;
			dst->u = h[4] * ICO_M_NRM;
			dst->v = h[5] * ICO_M_NRM;

			short *q = morphDst;
			for (int j = 1; j < count; j++) {
				src += 4;
				q[0] = src[0] / 5;
				q[1] = -src[1] / 5 - 2048;
				q[2] = -src[2] / 5;
				q += vertex_count * 3;
			}
			dst++;
			morphDst += 3;
			src = h + 8;
		}
		return (int *)src;
	}

	short *s = (short *)p;
	int nsh = count * 3;
	int total = *s++;
	int fstride = nsh + 7;
	fstride += fstride & 1;
	int pad = (vertex_count & 1) + 1;
	int nbytes = (fstride * total + (vertex_count + pad + 1)) * 2;
	if (end < p + nbytes / 4)
		return 0;

	short *idx = s + pad;
	s = idx + vertex_count;

	for (int i = 0; i < vertex_count; i++) {
		short *src = s + fstride * (*idx++ - pad);
		short *h = src + nsh;

		dst->x = src[0] * ICO_M_POS;
		dst->y = -src[1] * ICO_M_POS - 0.5f;
		dst->z = -src[2] * ICO_M_POS;
		dst->nx = h[0] * ICO_M_NRM;
		dst->ny = -h[1] * ICO_M_NRM;
		dst->nz = -h[2] * ICO_M_NRM;
		dst->color.rgba = *(int *)(h + 5) | 0xff000000;
		dst->u = h[3] * ICO_M_NRM;
		dst->v = h[4] * ICO_M_NRM;

		short *q = morphDst;
		for (int j = 1; j < count; j++) {
			src += 3;
			q[0] = src[0] / 5;
			q[1] = -src[1] / 5 - 2048;
			q[2] = -src[2] / 5;
			q += vertex_count * 3;
		}
		dst++;
		morphDst += 3;
	}
	return (int *)(s + fstride * total);
}

#undef ICO_M_POS
#undef ICO_M_NRM

ico_motion::ico_motion() : tag(1), frame_length(0), speed(1.0f), play_offset(0), valid_count(0)
{
	for (int i = 0; i < 8; i++) {
		entries[i].shape = -1;
		entries[i].key_count = 0;
		entries[i].keys = 0;
	}
}

ico_motion::~ico_motion()
{
	for (int i = 0; i < 8; i++) {
		if (entries[i].keys)
			delete[] (char *)entries[i].keys;
	}
}

ico_motion *ico_motion::copy(const ico_motion &other)
{

	tag = other.tag;
	frame_length = other.frame_length;
	speed = other.speed;
	play_offset = other.play_offset;
	valid_count = other.valid_count;

	for (int i = 0; i < 8; i++) {
		const Entry &oe = other.entries[i];
		Entry &e = entries[i];
		if (e.keys)
			delete[] (char *)e.keys;
		e.keys = 0;
		if (i < valid_count) {
			e.shape = oe.shape;
			e.key_count = oe.key_count;
			int sz = oe.key_count * 8;
			e.keys = new int[sz / 4];
			memcpy(e.keys, oe.keys, sz);
		}
	}
	return this;
}

/* Header of 20 bytes, at most 8 entries {shape, key_count, keys}; each keys buffer holds key_count
   keyframes of 8 bytes. valid_count and each new buffer are stored even when
   the following bounds check fails. */
int *ico_motion::load(int *p, int *end)
{
	if ((char *)end < (char *)p + 20)
		return 0;

	tag = *p++;
	frame_length = *p++;
	speed = *(const float *)p;
	p++;
	play_offset = *p++;
	int vc = *p++;
	valid_count = vc;
	if ((unsigned)vc >= 9)
		return 0;

	for (int i = 0; i < valid_count; i++) {
		Entry &e = entries[i];

		if ((char *)end < (char *)p + 8)
			return 0;

		e.shape = *p++;
		e.key_count = *p++;
		int size = e.key_count * 8;
		delete[] (char *)e.keys;
		e.keys = new int[size / 4];

		if (end < p + size / 4)
			return 0;

		memcpy(e.keys, p, size);
		p += size / 4;
	}
	return p;
}

struct ico_motion_key {
	float time;
	float value;
};

/* For each active entry (0 <= shape < shapeCount) interpolates its keyframes at time t
   into out[shape]; if the weights do not sum to 1 (+-0.001) they are normalized. */
void ico_motion::get_weights(float *out, int shapeCount, float t) const
{
	for (int k = 0; k < shapeCount; k++)
		out[k] = 0.0f;

	float total = 0.0f;
	for (int j = 0; j < valid_count; j++) {
		const Entry &e = entries[j];
		int shape = e.shape;
		if (shape < 0 || shape >= shapeCount)
			continue;

		ico_motion_key *p = (ico_motion_key *)e.keys;
		ico_motion_key *lo = p;
		ico_motion_key *hi = p;
		for (int k = 0; k < e.key_count; k++, p++) {
			hi = p;
			if (t < hi->time)
				break;
			lo = hi;
		}

		float value = lo->value;
		if (lo != hi) {
			float loTime = lo->time;
			float ratio = (t - loTime) / (hi->time - loTime);
			value += (hi->value - value) * ratio;
		}
		total += value;
		out[shape] = value;
	}

	if (total < 0.999f || total > 1.001f) {
		float inv = 1.0f / total;
		for (int k = 0; k < shapeCount; k++)
			out[k] *= inv;
	}
}

ico_texture::ico_texture() : size(0), buffer(0)
{}

ico_texture::~ico_texture()
{
	if (buffer)
		delete[] (char *)buffer;
}

ico_texture *ico_texture::copy(const ico_texture &other)
{
	size = 0x8000;
	if (buffer == 0)
		buffer = new char[0x8000];
	memcpy(buffer, other.buffer, size);
	return this;
}

/* mask is the model attrib: ICO_ATTR_TEXTURE enables the load (fixed 0x8000-byte buffer); without ICO_ATTR_RLE
   the pixels are copied and zero-padded, with it they are RLE-decoded.
   size is cleared first, even on the early return. */
int *ico_texture::load(int *p, int *end, int mask)
{
	int avail = (char *)end - (char *)p;

	if (buffer)
		delete[] (char *)buffer;
	buffer = 0;
	size = 0;

	if (!(mask & ICO_ATTR_TEXTURE))
		return p;

	size = 0x8000;
	buffer = new char[size];

	if (!(mask & ICO_ATTR_RLE)) {
		int n = avail < size ? avail : size;
		memcpy(buffer, p, n);
		memset((char *)buffer + n, 0, size - n);
		p = (int *)((char *)p + n);
	} else {
		if (avail < 4 || avail < *p + 4)
			return 0;
		p = decode_runlength((short *)buffer, p);
	}
	return p;
}

/* 16-bit RLE decoder. The first word is the compressed size in bytes.
   Each record starts with a short count c: c > 0 repeats the next short c
   times, c < 0 copies -c literal shorts. Returns the source pointer
   realigned to 4. */
int *ico_texture::decode_runlength(short *dst, int *src)
{
	int n = *src;
	short *p = (short *)(src + 1);
	short *end;

	size = n;
	end = p + n / 2;
	while (p < end) {
		int c = *p++;

		if (c > 0) {
			short val = *p++;

			for (int i = 0; i < c; i++)
				*dst++ = val;
		} else {
			c = -c;
			if (c > 0) {
				int k = c;

				do {
					*dst++ = *(unsigned short *)p++;
				} while (--k);
			}
		}
	}
	return ((int)p & 3) ? (int *)(p + 1) : (int *)p;
}

/* Opens the file and allocates the load buffer. Returns 0 on failure. */
static int IconFileStart(const string &path, int *pfd, void **pbuf, int *psize, int *pused)
{
	int fd;
	int n;
	char *p;
	char *e;

	if (*pfd >= 0) {
		close(*pfd);
		*pfd = -1;
	}
	if (path.compare("") == 0)
		return 0;
	*pbuf = 0;
	fd = open(path.c_str(), O_RDONLY);
	*pfd = fd;
	if (fd < 0) {
		printf("load start fail \"%s\"\n", path.c_str());
		return 0;
	}
	*psize = lseek(fd, 0, SEEK_END);
	if (*psize == 0) {
		close(*pfd);
		*pfd = -1;
		return 0;
	}
	lseek(*pfd, 0, SEEK_SET);
	n = (*psize + 3) / 4 * 4;
	p = new char[n];
	e = (char *)(n + (int)p);
	*pbuf = p;
	*(int *)(e - 4) = 0;
	*pused = 0;
	return 1;
}

/* Frees an array and clears the pointer; the size argument is unused. */
static void deleteArrayField(void **pp, int *)
{
	if (*pp != 0) {
		delete[] (char *)*pp;
		*pp = 0;
	}
}

/* Reads at most 16 KB into the load buffer. Returns 1 while data remains;
   at end of file or on error closes the descriptor and returns 0 (the
   caller compares *pused with *psize). */
static int IconFileStep(int *pfd, void **pbuf, int *psize, int *pused)
{
	if (*pfd < 0)
		return 0;
	int n = *psize - *pused;
	if (n > 0) {
		if (n > 16384)
			n = 16384;
		int r = read(*pfd, (char *)*pbuf + *pused, n);
		*pused += r;
		if (r > 0 && *pused < *psize)
			return 1;
	}
	close(*pfd);
	*pfd = -1;
	return 0;
}
