/*
 * Cloud methods: options, particle colors and motion, alpha shapes and
 * drawing.
 */

#include <vector>
#include "gfx/vec.h"
#include "cloud.h"
#include "gfx/gm_graph.h"
#include "gfx/gm_vertex.h"
#include <stdlib.h>
#include "scenefacade.h"
#include "scene/optid.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

Cloud::ClassInitializer __Cloud_ClassInitializer__;

/* One particle: position (w = 1), velocity and RGBA color. */
struct CloudParticle {
	/* 0x00 */ float x, y, z, w;
	/* 0x10 */ float vx, vy, vz;
	/* 0x1C */ unsigned char color[4];
	CloudParticle() {}  /* redundant as written */
};  /* size = 0x20 */

static void CloudNoiseAxis(float scale, int n, float coord, float *w0, int *i0, float *w1, int *i1);

/* Full-screen quad of the post effect (s_quadB) and particle sprite (s_quadA). */
static gmVertexPT s_quadB[2] = {
	{ -1.0f, 1.0f, 0.0f, 0.0f, 0.0f },
	{ 1.0f, -1.0f, 0.0f, 1.0f, 1.0f },
};

static gmVertexPCT s_quadA[2] = {
	{ -0.5f, 0.5f, 0.0f, { 0xffffffff }, 0.0f, 0.0f },
	{ 0.5f, -0.5f, 0.0f, { 0xffffffff }, 1.0f, 1.0f },
};

/* 1D noise table (a sum of sines over 11 octaves, normalized to 0-255) for CalcAlphaValue. */
static unsigned char s_noise[4096] = {
#include "cloud_noise.inc"
};
/* Noise phase and frame counter. */
static int s_phase = 0;
static int s_frame = 0;

/* Copies the 4 color bytes of particle `i` of the array at `base` into a vertex. */
static inline void cloud_copy_color(unsigned char *dst, const CloudParticle *base, int i)
{
	const unsigned char *src = base[i].color;
	for (int k = 3; k != -1; k--)
		*dst++ = *src++;
}

Cloud::Cloud(Object *parent, int count, int unused) : Object(parent)
{
	vec4 &jitter = *(vec4 *)&jitter_r;
	jitter_r = 0.0f;
	jitter.y = 0.0f;
	jitter.z = 0.0f;
	jitter.w = 0.0f;

	GetOptionState()->always_mask |= EFF_TEXTURE_MODE;

	animate_noise = 1;
	num_particles = count;
	num_sprites = count;
	particles = new CloudParticle[count];
	num_dparticles = count;
	num_dsprites = count;
	dparticles = new CloudParticle[count];

	cloud_tex_id = 0;
	dcloud_tex_id = 0;
	alpha_shape = CLOUD_ALPHA_SHELL;
	ray_x = 128;
	ray_y = 32;
	ray_scale_x = 10;
	ray_scale_y = 32;
	ray_offset_x = 0.0f;
	ray_offset_y = 0.0f;
	color_dirty = 1;
	speed = 1.0f;
	offset_x = offset_y = offset_z = ray_offset_x;

	for (int i = 0; i < num_particles; i++) {
		particles[i].x = (float)(rand() % 65536) / 65536.0f * 3.0f - 1.5f;
		particles[i].y = (float)(rand() % 65536) / 65536.0f - 0.5f;
		particles[i].z = (float)(rand() % 65536) / 65536.0f - 0.5f;
		particles[i].w = 1.0f;
		particles[i].vx = (float)(rand() % 65536) / 65536.0f * 2.0e-4f - 1.0e-4f;
		particles[i].vy = (float)(rand() % 65536) / 65536.0f * 2.0e-4f - 1.0e-4f;
		particles[i].vz = (float)(rand() % 65536) / 65536.0f * 2.0e-4f - 1.0e-4f;
		particles[i].vz = 0;
	}

	for (int i = 0; i < num_dparticles; i++) {
		dparticles[i].x = (float)(rand() % 65536) / 65536.0f * 3.0f - 1.5f;
		dparticles[i].y = (float)(rand() % 65536) / 65536.0f - 0.5f;
		dparticles[i].z = (float)(rand() % 65536) / 65536.0f - 0.5f;
		dparticles[i].w = 1.0f;
		dparticles[i].vx = (float)(rand() % 65536) / 65536.0f * 2.0e-4f - 1.0e-4f;
		dparticles[i].vy = (float)(rand() % 65536) / 65536.0f * 2.0e-4f - 1.0e-4f;
		dparticles[i].vz = (float)(rand() % 65536) / 65536.0f * 2.0e-4f - 1.0e-4f;
		dparticles[i].vz = 0;
	}

	color_buffer = 0;
}

/*
 * Fills the particle colors: in the first array each channel is the base
 * color plus a random jitter of up to +-jitter_r..27C, clamped to [0, 1];
 * the second array is white with alpha 0.
 */
void Cloud::InitColor()
{
	for (int i = 0; i < num_particles; i++) {
		float r = color.x + ((float)(rand() % 65536) / 65536.0f * (jitter_r + jitter_r) - jitter_r);
		if (r < 0.0f)
			particles[i].color[0] = 0;
		else if (1.0f <= r)
			particles[i].color[0] = 255;
		else
			particles[i].color[0] = (int)(r * 255.0f);

		float g = color.y + ((float)(rand() % 65536) / 65536.0f * (jitter_g + jitter_g) - jitter_g);
		if (g < 0.0f)
			particles[i].color[1] = 0;
		else if (1.0f <= g)
			particles[i].color[1] = 255;
		else
			particles[i].color[1] = (int)(g * 255.0f);

		float b = color.z + ((float)(rand() % 65536) / 65536.0f * (jitter_b + jitter_b) - jitter_b);
		if (b < 0.0f)
			particles[i].color[2] = 0;
		else if (1.0f <= b)
			particles[i].color[2] = 255;
		else
			particles[i].color[2] = (int)(b * 255.0f);

		float a = color.w + ((float)(rand() % 65536) / 65536.0f * (jitter_a + jitter_a) - jitter_a);
		if (a < 0.0f)
			particles[i].color[3] = 0;
		else if (1.0f <= a)
			particles[i].color[3] = 255;
		else
			particles[i].color[3] = (int)(a * 255.0f);
	}

	for (int i = 0; i < num_dparticles; i++) {
		dparticles[i].color[0] = 255;
		dparticles[i].color[1] = 255;
		dparticles[i].color[2] = 255;
		dparticles[i].color[3] = 0;
	}

	color_dirty = 0;
}

/*
 * Alpha 0-255 of a particle at `out` for shape `i` (CLOUD_ALPHA_*).
 */
int Cloud::CalcAlphaValue(int i, vec4 *out)
{
	int a;

	switch (i) {
	case CLOUD_ALPHA_NOISE1D: {
		int n = (int)((out->x + 1.5f) * 4096.0f / 3.0f);
		int idx = n + s_phase;
		n = idx % 4096;
		a = (int)((float)s_noise[n] * ((0.25f - out->y * out->y) * 4.0f) * ((0.25f - out->z * out->z) * 4.0f));
		break;
	}
	case CLOUD_ALPHA_COSINES: {
		float v = 0.0f;
		v = gmGetTableCos((int)((out->x + out->x) * 32768.0f / 3.14159274f)) + 2.0f + v;
		v = v + (gmGetTableCos((int)(out->x * 4.5f * 32768.0f / 3.14159274f)) + 1.0f) * 2.5f;
		v = v + (gmGetTableCos((int)(out->y * 32768.0f / 3.14159274f)) + 1.0f);
		v = v * ((0.25f - out->y * out->y) * 4.0f);
		v = v * ((0.25f - out->z * out->z) * 4.0f);
		v = v - 0.4f;
		if (v < 0.0f) {
			a = 0;
			break;
		}
		if (1.0f < v) {
			a = 255;
			break;
		}
		a = (int)(v * 255.0f);
		break;
	}
	case CLOUD_ALPHA_SPHERE: {
		float r = out->x * out->x + out->y * out->y + out->z * out->z;
		if (0.25f < r)
			r = 0.25f;
		a = (int)((1.0f - r * 4.0f) * 255.0f);
		break;
	}
	case CLOUD_ALPHA_SHELL: {
		float r2 = out->x * out->x + out->y * out->y;
		float r3 = r2 + out->z * out->z;
		if (0.25f < r3)
			r3 = 0.25f;
		if (0.09f < r2)
			r2 = 0.09f;
		float outer = 1.0f - r3 * 4.0f;
		float inner = 1.0 - r2 * (100.0f / 9.0f);
		if (inner < 0.0f)
			inner = 0.0f;
		float d = outer - inner;
		a = (int)((d + d) * 255.0f);
		if (a < 0)
			a = 0;
		else if (a >= 256)
			a = 255;
		break;
	}
	case CLOUD_ALPHA_VOLUME:
	default: {
		float wx0;
		int ix0;
		float wx1;
		int ix1;
		float wy0;
		int iy0;
		float wy1;
		int iy1;
		float wz0;
		int iz0;
		float wz1;
		int iz1;

		CloudNoiseAxis(ray_offset_x, ray_scale_x, out->x, &wx0, &ix0, &wx1, &ix1);
		ix0 += ray_x;
		ix1 += ray_x;
		ix0 = (ix0 % ray_width + ray_width) % ray_width;
		ix1 = (ix1 % ray_width + ray_width) % ray_width;

		CloudNoiseAxis(ray_offset_y, ray_scale_y, out->y, &wy0, &iy0, &wy1, &iy1);
		iy0 += ray_y;
		iy1 += ray_y;
		iy0 = (iy0 % ray_height + ray_height) % ray_height;
		iy1 = (iy1 % ray_height + ray_height) % ray_height;

		CloudNoiseAxis(0.0f, 8, out->z, &wz0, &iz0, &wz1, &iz1);
		iz0 += 7;
		iz1 += 7;
		iz0 = (iz0 % ray_depth + ray_depth) % ray_depth;
		iz1 = (iz1 % ray_depth + ray_depth) % ray_depth;

		const unsigned char **slices = (const unsigned char **)ray_slices;
		int r0 = iy0 * ray_width;
		int r1 = iy1 * ray_width;

		float q00 = wx0 * (float)slices[iz0][r0 + ix0] + wx1 * (float)slices[iz0][r0 + ix1];
		float q01 = wx0 * (float)slices[iz0][r1 + ix0] + wx1 * (float)slices[iz0][r1 + ix1];
		float q10 = wx0 * (float)slices[iz1][r0 + ix0] + wx1 * (float)slices[iz1][r0 + ix1];
		float q11 = wx0 * (float)slices[iz1][r1 + ix0] + wx1 * (float)slices[iz1][r1 + ix1];
		float p0 = wy0 * q00 + wy1 * q01;
		float p1 = wy0 * q10 + wy1 * q11;

		a = (int)(wz0 * p0 + wz1 * p1);
		a *= 2;
		if (a < 0)
			a = 0;
		else if (a >= 256)
			a = 255;
		break;
	}
	}
	return a;
}

/* Moves the particles by their velocity (doubled in the second array) times the speed, wrapping them. */
void Cloud::UpdatePosition()
{
	for (int i = 0; i < num_particles; i++) {
		particles[i].x += particles[i].vx * speed;
		float x = particles[i].x;
		if (x < -1.5f)
			particles[i].x = x + 3.0f;
		else if (1.5f < x)
			particles[i].x = x - 3.0f;

		particles[i].y += particles[i].vy * speed;
		float y = particles[i].y;
		if (y < -0.5f)
			particles[i].y = y + 1.0f;
		else if (0.5f < y)
			particles[i].y = y - 1.0f;

		particles[i].z += particles[i].vz * speed;
		float z = particles[i].z;
		if (z < -0.5f)
			particles[i].y = particles[i].y + 1.0f; /* NOTE: bug: wraps y, not z (also in the loop below) */
		else if (0.5f < z)
			particles[i].z = z - 1.0f;
	}

	for (int i = 0; i < num_dparticles; i++) {
		dparticles[i].x += (dparticles[i].vx + dparticles[i].vx) * speed;
		float x = dparticles[i].x;
		if (x < -1.5f)
			dparticles[i].x = x + 3.0f;
		else if (1.5f < x)
			dparticles[i].x = x - 3.0f;

		dparticles[i].y += (dparticles[i].vy + dparticles[i].vy) * speed;
		float y = dparticles[i].y;
		if (y < -0.5f)
			dparticles[i].y = y + 1.0f;
		else if (0.5f < y)
			dparticles[i].y = y - 1.0f;

		dparticles[i].z += (dparticles[i].vz + dparticles[i].vz) * speed;
		float z = dparticles[i].z;
		if (z < -0.5f)
			dparticles[i].y = dparticles[i].y + 1.0f;
		else if (0.5f < z)
			dparticles[i].z = z - 1.0f;
	}
}

/* Wraps the particle positions into x [-1.5, 1.5], y and z [-0.5, 0.5]. */
void Cloud::CheckPosRange()
{
	for (int i = 0; i < num_particles; i++) {
		float x = particles[i].x;
		if (x < -1.5f)
			particles[i].x = x + 3.0f;
		else if (1.5f < x)
			particles[i].x = x - 3.0f;

		float y = particles[i].y;
		if (y < -0.5f)
			particles[i].y = y + 1.0f;
		else if (0.5f < y)
			particles[i].y = y - 1.0f;

		float z = particles[i].z;
		if (z < -0.5f)
			particles[i].z = z + 1.0f;
		else if (0.5f < z)
			particles[i].z = z - 1.0f;
	}

	for (int i = 0; i < num_particles; i++) { /* NOTE: bug: bounded by the count of the first array */
		float x = dparticles[i].x;
		if (x < -1.5f)
			dparticles[i].x = x + 3.0f;
		else if (1.5f < x)
			dparticles[i].x = x - 3.0f;

		float y = dparticles[i].y;
		if (y < -0.5f)
			dparticles[i].y = y + 1.0f;
		else if (0.5f < y)
			dparticles[i].y = y - 1.0f;

		float z = dparticles[i].z;
		if (z < -0.5f)
			dparticles[i].z = z + 1.0f;
		else if (0.5f < z)
			dparticles[i].z = z - 1.0f;
	}
}

void Cloud::DrawThis(gm_graph &g)
{

	if (!visible)
		return;
	if (color_dirty)
		InitColor();
	if (color_buffer) {
		g.SetColorBuffer(color_buffer);
		g.Viewport();
		g.Clear(0xffff);
	}
	g.BlendFunc(GM_BLEND_ADD);
	g.Color(scene->GetColor());

	mat4 save = *g.GetMatrix(GM_MATRIX_MODEL);
	const mat4 *psave = &save;
	const float *mv = (const float *)g.GetMatrix(GM_MATRIX_VIEW);
	const float *mp = (const float *)g.GetMatrix(GM_MATRIX_MODEL);
	mat4 bb(mv[0], mv[4], mv[8], 0.0f, mv[1], mv[5], mv[9], 0.0f, mv[2], mv[6], mv[10], 0.0f, mp[12], mp[13],
	    mp[14], 1.0f);

	s_quadA[0].x = -0.3f;
	s_quadA[0].y = -0.25f;
	s_quadA[0].z = 0.0f;
	s_quadA[1].x = 0.3f;
	s_quadA[1].y = 0.25f;
	s_quadA[1].z = 0.0f;

	if (animate_noise) {
		s_phase = (s_frame << 12) / 216000;
		s_frame++;
		if (s_frame > 216000)
			s_frame -= 216000;
	}

	if (cloud_tex) {
		if (((const int *)cloud_tex)[cloud_tex_id]) {
			g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
			g.SetTexture(((const int *)cloud_tex)[cloud_tex_id]);
		} else {
			g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE);
		}
		for (int i = 0; i < num_sprites; i++) {
			int a = CalcAlphaValue(alpha_shape, (vec4 *)&particles[i]);
			if (a > 255)
				a = 255;
			if (a > 0) {
				cloud_copy_color(s_quadA[0].color.c, particles, i);
				cloud_copy_color(s_quadA[1].color.c, particles, i);
				unsigned char *al = &s_quadA[0].color.c[3];
				al[24] = a;
				al[0] = a;
				g.SetArrays(s_quadA, GM_VERTEX_PCT, 2, 1, 0, 0);
				bb.m3 = *psave * *(const vec4 *)&particles[i];
				g.LoadMatrix(GM_MATRIX_MODEL, bb);
				g.Scale(local_scale);
				g.DrawArrays(GM_PRIM_SPRITE, 0, 2, 1, 0);
			}
		}
	}

	if (dcloud_tex) {
		if (((const int *)dcloud_tex)[dcloud_tex_id]) {
			g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
			g.SetTexture(((const int *)dcloud_tex)[dcloud_tex_id]);
		} else {
			g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE);
		}
		g.BlendFunc(GM_BLEND_ALPHA);
		for (int i = 0; i < num_dsprites; i++) {
			int a = CalcAlphaValue(alpha_shape, (vec4 *)&dparticles[i]);
			if (a > 255)
				a = 255;
			if (a > 0) {
				cloud_copy_color(s_quadA[0].color.c, dparticles, i);
				cloud_copy_color(s_quadA[1].color.c, dparticles, i);
				unsigned char *al = &s_quadA[0].color.c[3];
				al[24] = a >> 2;
				al[0] = a >> 2;
				g.SetArrays(s_quadA, GM_VERTEX_PCT, 2, 1, 0, 0);
				bb.m3 = *psave * *(const vec4 *)&dparticles[i];
				g.LoadMatrix(GM_MATRIX_MODEL, bb);
				g.Scale(local_scale);
				g.DrawArrays(GM_PRIM_SPRITE, 0, 2, 1, 0);
			}
		}
	}

	if (color_buffer) {
		g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGB);
		g.SetColorBuffer(-1);
		g.Viewport();
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
		g.SetTexture(color_buffer);
		g.VertexGen(0);
		g.SetArrays(s_quadB, GM_VERTEX_PT, 2, 1, 0, 0);
		g.DrawArrays(GM_PRIM_SPRITE, 0, 2, 1, 0);
		g.VertexGen(2);
		g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGBA);
	}

	g.LoadMatrix(GM_MATRIX_MODEL, save);
	UpdatePosition();
}

int Cloud::SetOption(int option, const void *data, unsigned int size)
{
	switch (option) {
	case OPT_CLUD: {
		const int *in = (const int *)data;
		num_cloud_tex = in[0];
		cloud_tex = in[1];
		return 1;
	}
	case OPT_DCLD: {
		const int *in = (const int *)data;
		num_dcloud_tex = in[0];
		dcloud_tex = in[1];
		return 1;
	}
	case OPT_RAY: {
		const int *in = (const int *)data;
		ray_width = in[0];
		ray_height = in[1];
		ray_depth = in[2];
		ray_slices = in[3];
		return 1;
	}
	case OPT_OFS: {
		const float *in = (const float *)data;
		offset_x = in[0];
		offset_y = in[1];
		offset_z = in[2];
		for (int i = 0; i < num_particles; i++) {
			particles[i].x += offset_x;
			particles[i].y += offset_y;
			particles[i].z += offset_z;
		}
		for (int i = 0; i < num_dparticles; i++) {
			dparticles[i].x += offset_x;
			dparticles[i].y += offset_y;
			dparticles[i].z += offset_z;
		}
		CheckPosRange();
		return 1;
	}
	case OPT_CDW: {
		const float *in = (const float *)data;
		jitter_r = in[0];
		jitter_g = in[1];
		jitter_b = in[2];
		jitter_a = in[3];
		color_dirty = 1;
		return 1;
	}
	case OPT_RSCL:
		animate_noise = (data != 0);
		return 1;
	}
	return Object::SetOption(option, data, 0);
}

int Cloud::SetOption(int option, const float &value, unsigned int size)
{
	switch (option) {
	case OPT_RXO:
		ray_offset_x = value;
		return 1;
	case OPT_RYO:
		ray_offset_y = value;
		return 1;
	case OPT_SPD0:
		speed = value;
		return 1;
	case OPT_XOFS: {
		offset_x = value;
		for (int i = 0; i < num_particles; i++)
			particles[i].x += offset_x;
		CheckPosRange();
		return 1;
	}
	case OPT_YOFS: {
		offset_y = value;
		for (int i = 0; i < num_particles; i++)
			particles[i].y += offset_y;
		CheckPosRange();
		return 1;
	}
	case OPT_ZOFS: {
		offset_z = value;
		for (int i = 0; i < num_particles; i++)
			particles[i].z += offset_z;
		CheckPosRange();
		return 1;
	}
	}
	return Object::SetOption(option, value, 1);
}

int Cloud::SetOption(int option, const int &value, unsigned int size)
{
	switch (option) {
	case OPT_RAYW:
		ray_scale_x = value;
		return 1;
	case OPT_RAYH:
		ray_scale_y = value;
		return 1;
	case OPT_RAYX: {
		int r = value % ray_width;
		r = (r + ray_width) % ray_width;
		ray_x = r;
		s_phase = (value % 4096 + 4096) % 4096;
		return 1;
	}
	case OPT_RAYY: {
		int r = value % ray_height;
		r = (r + ray_height) % ray_height;
		ray_y = r;
		return 1;
	}
	case OPT_CLID: {
		int r = value % num_cloud_tex;
		r = (r + num_cloud_tex) % num_cloud_tex;
		cloud_tex_id = r;
		return 1;
	}
	case OPT_DCID: {
		int r = value % num_dcloud_tex;
		r = (r + num_dcloud_tex) % num_dcloud_tex;
		dcloud_tex_id = r;
		return 1;
	}
	case OPT_NSPR:
		num_sprites = value % num_particles;
		return 1;
	case OPT_NDSP:
		num_dsprites = value;
		return 1;
	case OPT_RAYM:
		alpha_shape = value;
		return 1;
	case OPT_BSIZ: {
		gm_graph *mgr = SceneGetGraph();
		mgr->DeleteColorBuffer(color_buffer);
		color_buffer = 0;
		if ((unsigned int)(value - 1) < 256)
			color_buffer = mgr->CreateColorBuffer(value, value, 0);
		return 1;
	}
	}
	return Object::SetOption(option, value, 1);
}

int Cloud::GetOption(int option, void *data, unsigned int size) const
{
	switch (option) {
	case OPT_CLUD: {
		if (size < 8)
			return 0;
		int *out = (int *)data;
		out[0] = num_cloud_tex;
		out[1] = cloud_tex;
		return 1;
	}
	case OPT_DCLD: {
		if (size < 8)
			return 0;
		int *out = (int *)data;
		out[0] = num_dcloud_tex;
		out[1] = dcloud_tex;
		return 1;
	}
	case OPT_RAY: {
		if (size < 16)
			return 0;
		int *out = (int *)data;
		out[0] = ray_width;
		out[1] = ray_height;
		out[2] = ray_depth;
		out[3] = ray_slices;
		return 1;
	}
	case OPT_OFS: {
		if (size < 12)
			return 0;
		float *out = (float *)data;
		out[0] = offset_x;
		out[1] = offset_y;
		out[2] = offset_z;
		return 1;
	}
	case OPT_CDW: {
		if (size < 16)
			return 0;
		float *out = (float *)data;
		out[0] = jitter_r;
		out[1] = jitter_g;
		out[2] = jitter_b;
		out[3] = jitter_a;
		return 1;
	}
	}
	return Object::GetOption(option, data, size);
}

Cloud::ClassInitializer::ClassInitializer()
{}

Cloud::~Cloud()
{
	if (dparticles)
		delete[] dparticles;
	if (particles)
		delete[] particles;
	gm_graph *mgr = SceneGetGraph();
	mgr->DeleteColorBuffer(color_buffer);
}

void Cloud::InitModel()
{}

void Cloud::CalcModel()
{}

void Cloud::SetColor(float r, float g, float b, float a)
{
	Object::SetColor(r, g, b, a);
	color_dirty = 1;
}

int Cloud::SetOption(int option, const String &value)
{
	return Object::SetOption(option, value);
}

int Cloud::GetOption(int option, float &value, unsigned int size) const
{
	switch (option) {
	case OPT_SPD0:
		value = speed;
		return 1;
	case OPT_XOFS:
		value = offset_x;
		return 1;
	case OPT_YOFS:
		value = offset_y;
		return 1;
	case OPT_ZOFS:
		value = offset_z;
		return 1;
	}
	return Object::GetOption(option, value, 1);
}

int Cloud::GetOption(int option, int &value, unsigned int size) const
{
	switch (option) {
	case OPT_RAYM:
		value = alpha_shape;
		return 1;
	case OPT_CLID:
		value = cloud_tex_id;
		return 1;
	case OPT_DCID:
		value = dcloud_tex_id;
		return 1;
	case OPT_NSPR:
		value = num_sprites;
		return 1;
	case OPT_NDSP:
		value = num_dsprites;
		return 1;
	}
	return Object::GetOption(option, value, 1);
}

int Cloud::GetOption(int option, String &value) const
{
	return Object::GetOption(option, value);
}

/* Per-axis sampler of the 3D noise: the samples around `coord` (indices i0/i1, weights w0/w1). */
static void CloudNoiseAxis(float scale, int n, float coord, float *w0, int *i0, float *w1, int *i1)
{
	float v;

	if (coord < 0.0f) {
		float frac;
		int k;

		coord = -coord;
		v = coord * n * 2.0f;
		v += scale;
		k = (int)v;
		*i0 = k;
		frac = v - (float)k;
		*w1 = frac;
		*w0 = 1.0f - frac;
		*i1 = (int)(v + 1.0f);
		*i0 = -*i0;
		*i1 = -*i1;
	} else {
		float frac;
		int k;

		v = coord * n * 2.0f;
		v += scale;
		k = (int)v;
		*i0 = k;
		frac = v - (float)k;
		*w1 = frac;
		*w0 = 1.0f - frac;
		*i1 = (int)(v + 1.0f);
	}
}
