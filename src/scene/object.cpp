/*
 * Object: node of the scene graph. Children, transforms (local and world
 * matrices, position, rotation, scale), color, z-sort, references between
 * objects and the generic option interface.
 */

#pragma implementation "object.h"

#include <iostream.h>
#include "gfx/vec.h"
#include <vector>
#include <string>
#include "scene/optid.h"
#include <math.h>

/* Identity matrix and w = 1 row. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

/* Local copy of normalize(). */
static inline vec4 normalize_l(const vec4 &v)
{
	float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	if (len == 0.0f)
		return v;
	len = 1.0 / len;
	return vec4(v.x * len, v.y * len, v.z * len, v.w);
}

/* Full transpose. */
static inline mat4 transpose_l(const mat4 &m)
{
	return mat4(m.m0.x, m.m1.x, m.m2.x, m.m3.x, m.m0.y, m.m1.y, m.m2.y, m.m3.y, m.m0.z, m.m1.z, m.m2.z, m.m3.z,
	    m.m0.w, m.m1.w, m.m2.w, m.m3.w);
}

/* Row-vector transform with w = 1. */
static inline vec4 apply(const mat4 &m, const vec4 &v)
{
	return m.m0 * v.x + m.m1 * v.y + m.m2 * v.z + m.m3 * 1.0f;
}

class gm_graph;

mat4 mat4_translate(float x, float y, float z);
mat4 mat4_scale(float x, float y, float z);
mat4 multiply(const mat4 &a, const mat4 &b);
vec4 apply(const mat4 &m, const vec4 &v, float w);
mat4 mat4_quat(float x, float y, float z, float w);

static inline mat4 translation_matrix(const vec4 &v)
{
	return mat4_translate(v.x, v.y, v.z);
}
static inline mat4 scale_matrix(const vec4 &v)
{
	return mat4_scale(v.x, v.y, v.z);
}

static inline mat4 mat4_quat(const vec4 &q)
{
	return mat4_quat(q.x, q.y, q.z, q.w);
}

inline vec4 apply(const mat4 &m, const vec4 &v, float w)
{
	return m.m0 * v.x + m.m1 * v.y + m.m2 * v.z + m.m3 * w;
}

static inline vec4 apply4_l(const mat4 &m, const vec4 &v)
{
	return apply(m, v, v.w);
}

inline mat4 multiply(const mat4 &a, const mat4 &b)
{
	const vec4 &r0 = apply4_l(a, b.m0);
	return mat4(r0, apply4_l(a, b.m1), apply4_l(a, b.m2), apply4_l(a, b.m3));
}

inline mat4 mat4_translate(float x, float y, float z)
{
	return mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, x, y, z, 1.0f);
}

inline mat4 mat4_scale(float x, float y, float z)
{
	return mat4(x, 0.0f, 0.0f, 0.0f, 0.0f, y, 0.0f, 0.0f, 0.0f, 0.0f, z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

inline mat4 mat4_quat(float x, float y, float z, float w)
{
	float x2 = x + x;
	float w2 = w + w;
	float z2 = z + z;
	float y2 = y + y;
	return mat4(1.0f - y2 * y - z2 * z, x2 * y + w2 * z, z2 * x - w2 * y, 0.0f, x2 * y - w2 * z,
	    1.0f - x2 * x - z2 * z, y2 * z + w2 * x, 0.0f, z2 * x + w2 * y, y2 * z - w2 * x, 1.0f - x2 * x - y2 * y,
	    0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

#include "object.h"

/* Rotation from three angles, composed Y * X * Z or Z * Y * X. */
static inline mat4 rot_yxz(const vec4 &r)
{
	return mat4_rotate_y(r.y) * mat4_rotate_x(r.x) * mat4_rotate_z(r.z);
}
static inline mat4 rot_zyx(const vec4 &r)
{
	return mat4_rotate_z(r.z) * mat4_rotate_y(r.y) * mat4_rotate_x(r.x);
}

int Object::instance_count = 0;
int Object::upmat_count_L = 0;
int Object::upmat_count_W = 0;

/* Debug report of the instance count; unused. */
static inline void PrintInstanceCount(const String &msg)
{
	cout << msg.c_str() << "Object::instance_count = " << Object::InstanceCount() << endl;
}

static inline int imax(int a, int b)
{
	return (a < b) ? b : a;
}

static inline int imin(int a, int b)
{
	return (b < a) ? b : a;
}

/* Local copies of quat_from_mat4, quat_to_rotzyx and quat_to_rotyxz. */
static inline vec4 quat_from_mat4_l(const mat4 &m) return q;
{
	float m00 = m.m0.x;
	float m11 = m.m1.y;
	float m22 = m.m2.z;
	float trace = m00 + m11 + m22;
	if (trace + 1.0f > 0.01f) {
		float s = sqrtf(trace + 1.0f);
		float h = s * 0.5f;
		float d = 0.25f / h;
		float t1 = m.m1.z - m.m2.y;
		float t2 = m.m2.x - m.m0.z;
		float t3 = m.m0.y - m.m1.x;
		q.w = h;
		q.x = t1 * d;
		q.y = t2 * d;
		q.z = t3 * d;
	} else if (m00 > m11) {
		if (!(m00 > m22))
			goto lastcase;
		float s = sqrtf(m00 + 1.0f - m11 - m22);
		float h = s * 0.5f;
		float d = 0.25f / h;
		float t1 = m.m1.x + m.m0.y;
		float t2 = m.m2.x + m.m0.z;
		float t3 = m.m1.z - m.m2.y;
		q.x = h;
		q.y = t1 * d;
		q.z = t2 * d;
		q.w = t3 * d;
	} else if (m11 > m22) {
		float s = sqrtf(m11 + 1.0f - m22 - m00);
		float h = s * 0.5f;
		float d = 0.25f / h;
		float t1 = m.m1.x + m.m0.y;
		float t2 = m.m2.y + m.m1.z;
		float t3 = m.m2.x - m.m0.z;
		q.y = h;
		q.x = t1 * d;
		q.z = t2 * d;
		q.w = t3 * d;
	} else {
lastcase:
		float s = sqrtf(m.m2.z + 1.0f - m.m0.x - m.m1.y);
		float h = s * 0.5f;
		float d = 0.25f / h;
		float t1 = m.m2.x + m.m0.z;
		float t2 = m.m2.y + m.m1.z;
		float t3 = m.m0.y - m.m1.x;
		q.z = h;
		q.x = t1 * d;
		q.y = t2 * d;
		q.w = t3 * d;
	}
}

static inline vec4 quat_to_rotzyx_l(const vec4 &q)
{
	float qx = q.x, qy = q.y, qz = q.z, qw = q.w;
	float sp = 2.0f * qw * qy - 2.0f * qz * qx;
	float ey, rn, rd, yn, yd;
	if (sp <= 0.99995 && sp >= -0.99995) {
		yn = 2.0f * qx * qy + 2.0f * qw * qz;
		yd = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
		rd = 1.0f - 2.0f * qx * qx - 2.0f * qy * qy;
		rn = 2.0f * qy * qz + 2.0f * qw * qx;
		ey = asinf(sp);
	} else {
		rn = 0.0f;
		yd = 1.0f - 2.0f * qx * qx - 2.0f * qz * qz;
		rd = 1.0f;
		yn = 2.0f * qw * qz - 2.0f * qx * qy;
		ey = atan2f(sp, 1.0f - 2.0f * qx * qx - 2.0f * qy * qy);
	}
	float ex = atan2f(rn, rd);
	return vec4(ex, ey, atan2f(yn, yd), 1.0f);
}

static inline vec4 quat_to_rotyxz_l(const vec4 &q)
{
	float qx = q.x, qy = q.y, qz = q.z, qw = q.w;
	float sp = 2.0f * qw * qx - 2.0f * qy * qz;
	float ex, zn, zd, yn, yd;
	if (sp <= 0.99995 && sp >= -0.99995) {
		zd = 1.0f - 2.0f * qx * qx - 2.0f * qz * qz;
		zn = 2.0f * qx * qy + 2.0f * qw * qz;
		yn = 2.0f * qz * qx + 2.0f * qw * qy;
		yd = 1.0f - 2.0f * qx * qx - 2.0f * qy * qy;
		ex = asinf(sp);
	} else {
		zn = 0.0f;
		yd = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
		zd = 1.0f;
		yn = 2.0f * qw * qy - 2.0f * qz * qx;
		ex = atan2f(sp, 1.0f - 2.0f * qx * qx - 2.0f * qz * qz);
	}
	return vec4(ex, atan2f(yn, yd), atan2f(zn, zd), 1.0f);
}

/* Rotation `v` in frame `w`, composed in the given axis order, expressed in
   frame `ow`. */
static inline void rel_rot_zyx(mat4 &out, const mat4 &ow, const mat4 &w, const vec4 &v)
{
	mat4 r = multiply(transpose_l(ow) * w, rot_zyx(v));
	out = r;
}

static inline void rel_rot_yxz(mat4 &out, const mat4 &ow, const mat4 &w, const vec4 &v)
{
	mat4 r = multiply(transpose_l(ow) * w, rot_yxz(v));
	out = r;
}

/* Reference list helpers, defined at the end of the file. */
static int SearchReference(ObjVec *v, Object *obj);
static void EraseReference(ObjVec *v, int no);
static void RemoveReference(ObjVec *v, Object *obj);

Object::Object(Object *p)
        : Hook(2), parent(p), zsort_mode(ZSORT_NONE), zsort_offset(0.0f), zsort_value(0.0f), sphere_radius(0.0f),
          sphere_visibility(1), visible(1), children_visible(1), debug(0), color_enable(0), suicide_timer(-1), flip(0),
          blend_mode(1), pos(0.0f, 0.0f, 0.0f, 1.0f), prepos(0.0f, 0.0f, 0.0f, 1.0f), scale(1.0f, 1.0f, 1.0f, 1.0f),
          local_scale(1.0f, 1.0f, 1.0f, 1.0f),
          local_rot(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),
          world_rot(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),
          local_mat(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),
          world_mat(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),
          world_inv(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),
          color(1.0f, 1.0f, 1.0f, 1.0f), local_dirty(1), world_dirty(1), reference(0), referenced_by(0)
{
	if (p) {
		p->children.push_back(this);
		scene = p->scene;
		flip = p->flip;
	} else {
		scene = this;
		local_dirty = 0;
		world_dirty = 0;
	}
	instance_count++;
	option_state.Init(this);
}

Object::~Object()
{
	CleanupReference();
	while (children.end() - children.begin() != 0)
		delete *children.begin();
	DoCallback(1, 0);
	if (parent) {
		Object **i = parent->children.begin();
		while (i != parent->children.end()) {
			if (*i == this)
				parent->children.erase(i);
			else
				i++;
		}
	}
	instance_count--;
}

void Object::AnimChildren()
{
	ObjVec dead;
	for (Object **p = children.begin(); p != children.end(); p++) {
		int dead_now;
		if ((*p)->suicide_timer <= 0) {
			dead_now = 0;
		} else {
			int t = (*p)->suicide_timer - 1;
			(*p)->suicide_timer = t;
			dead_now = t <= 0;
		}
		if (dead_now)
			dead.push_back(*p);
		else
			(*p)->Anim();
	}
	for (Object **p = dead.begin(); p != dead.end(); p++)
		delete *p;
}

vec4 Object::GetPos(Object *o, const vec4 &v)
{
	if (o) {
		UpdateMatrix();
		o->UpdateMatrix();
		int f = flip;
		float x = v.x;
		if (f & 1)
			x = -x;
		float y = v.y;
		if (f & 2)
			y = -y;
		float z = v.z;
		if (f & 4)
			z = -z;
		vec4 t(x, y, z, 1.0f);
		vec4 r = apply(o->world_inv * world_mat, t);
		f = o->flip;
		float tx = r.x;
		if (f & 1)
			tx = -tx;
		r.x = tx;
		float ty = r.y;
		if (f & 2)
			ty = -ty;
		r.y = ty;
		float tz = r.z;
		if (f & 4)
			tz = -tz;
		r.z = tz;
		return r;
	}
	return pos;
}

vec4 Object::GetPrePos(Object *o, const vec4 &v)
{
	if (o) {
		UpdateMatrix();
		o->UpdateMatrix();
		int f = flip;
		float x = v.x;
		if (f & 1)
			x = -x;
		float y = v.y;
		if (f & 2)
			y = -y;
		float z = v.z;
		if (f & 4)
			z = -z;
		vec4 t(x, y, z, 1.0f);
		float px = prepos.x;
		if (f & 1)
			px = -px;
		float py = prepos.y;
		if (f & 2)
			py = -py;
		float pz = prepos.z;
		if (f & 4)
			pz = -pz;
		vec4 p(px, py, pz, 1.0f);
		vec4 r = apply(o->world_inv * parent->world_mat, p + local_mat * t);
		f = o->flip;
		float tx = r.x;
		if (f & 1)
			tx = -tx;
		r.x = tx;
		float ty = r.y;
		if (f & 2)
			ty = -ty;
		r.y = ty;
		float tz = r.z;
		if (f & 4)
			tz = -tz;
		r.z = tz;
		return r;
	}
	return prepos;
}

/* Builds local_rot from three angles; mode selects the composition order
   (0 = Y * X * Z, 1 = Z * Y * X). With `o` the angles are given in the
   frame of o and first converted into the parent frame. */
void Object::SetRot(float x, float y, float z, Object *o, int mode)
{
	if (o) {
		vec4 t(x, y, z, 1.0f);
		switch (mode) {
		case 0:
			local_rot = rot_yxz(o->GetRot(parent, t, 0));
			break;
		case 1:
			local_rot = rot_zyx(o->GetRot(parent, t, 1));
			break;
		}
	} else {
		switch (mode) {
		case 0:
			local_rot = rot_yxz(vec4(x, y, z, 1.0f));
			break;
		case 1:
			local_rot = rot_zyx(vec4(x, y, z, 1.0f));
			break;
		}
	}
	SetLocalDirty();
}

void Object::SetRot(const vec4 &v, Object *o, int mode)
{
	if (o) {
		switch (mode) {
		case 0:
			local_rot = rot_yxz(o->GetRot(parent, v, 0));
			break;
		case 1:
			local_rot = rot_zyx(o->GetRot(parent, v, 1));
			break;
		}
	} else {
		switch (mode) {
		case 0:
			local_rot = rot_yxz(v);
			break;
		case 1:
			local_rot = rot_zyx(v);
			break;
		}
	}
	SetLocalDirty();
}

void Object::SetQuat(float x, float y, float z, float w, Object *o)
{
	if (o) {
		vec4 t(x, y, z, w);
		local_rot = mat4_quat(o->GetQuat(parent, t));
	} else {
		local_rot = mat4_quat(x, y, z, w);
	}
	SetLocalDirty();
}

/* Converts rotation `v` (angles in this object's frame) into angles in the
   frame of `o`, composed in the order given by mode; when `o` is 0 returns
   the angles of local_rot. */
vec4 Object::GetRot(Object *o, const vec4 &v, int mode)
{
	if (o) {
		UpdateMatrix();
		o->UpdateMatrix();
		mat4 m;
		switch (mode) {
		case 1:
			rel_rot_zyx(m, o->world_rot, world_rot, v);
			return quat_to_rotzyx_l(quat_from_mat4_l(m));
		case 0:
		default:
			rel_rot_yxz(m, o->world_rot, world_rot, v);
			return quat_to_rotyxz_l(quat_from_mat4_l(m));
		}
	}
	switch (mode) {
	case 1:
		return quat_to_rotzyx_l(quat_from_mat4_l(local_rot));
	case 0:
	default:
		return quat_to_rotyxz_l(quat_from_mat4_l(local_rot));
	}
}

/* Converts quaternion `q` from this object's frame into the frame of `o`;
   when `o` is 0 returns the quaternion of local_rot. */
vec4 Object::GetQuat(Object *o, const vec4 &q)
{
	if (o) {
		UpdateMatrix();
		o->UpdateMatrix();
		return quat_from_mat4_l(multiply(transpose_l(o->world_rot) * world_rot, mat4_quat(q)));
	}
	return quat_from_mat4_l(local_rot);
}

/* Builds local_rot as an orthonormal basis pointing from pos towards
   target. up.w == -10 means "use the global Y axis". */
void Object::LookAt(const vec4 &target, const vec4 &up)
{
	vec4 f = target - pos;
	vec4 u = up - pos;
	if (up.w == -10.0f)
		u.Set(0.0f, 1.0f, 0.0f, 0.0f);
	local_rot.m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	local_rot.m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	local_rot.m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	local_rot.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	f.w = 0.0f;
	u.w = 0.0f;
	vec4 z = normalize(f);
	vec4 x = normalize_l(cross(u, z));
	local_rot = mat4(x, cross(z, x), z, local_rot.m3);
	SetLocalDirty();
}

void Object::LookAt(Object *targetObj, const vec4 &target, Object *upObj, const vec4 &up)
{
	if (!targetObj)
		return;
	vec4 f = targetObj->GetPos(parent, target) - pos;
	vec4 u(0.0f, 1.0f, 0.0f, 0.0f);
	if (upObj)
		u = upObj->GetPos(parent, up) - pos;
	local_rot.m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	local_rot.m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	local_rot.m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	local_rot.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	f.w = 0.0f;
	u.w = 0.0f;
	vec4 z = normalize(f);
	vec4 x = normalize_l(cross(u, z));
	local_rot = mat4(x, cross(z, x), z, local_rot.m3);
	SetLocalDirty();
}

void Object::SetPosture(const vec4 &dir, const vec4 &up, Object *o)
{
	vec4 f, u;
	if (o) {
		UpdateMatrix();
		mat4 m = transpose_l(world_rot) * o->world_rot;
		f = m * dir;
		u = m * up;
	} else {
		f = dir;
		u = up;
	}
	local_rot.m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	local_rot.m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	local_rot.m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	local_rot.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	f.w = 0.0f;
	u.w = 0.0f;
	vec4 z = normalize(f);
	vec4 x = normalize_l(cross(u, z));
	vec4 y = cross(z, x);
	local_rot = mat4(x, y, z, local_rot.m3);
	SetLocalDirty();
}

/* Rebuilds local_mat (scale, rotation, translation) and world_mat /
   world_inv from the parent when they are dirty. */
mat4 &Object::UpdateMatrix()
{
	if (local_dirty) {
		upmat_count_L++;
		float x = local_mat.m3.x;
		if (flip & 1)
			x = -x;
		float y = local_mat.m3.y;
		if (flip & 2)
			y = -y;
		float z = local_mat.m3.z;
		if (flip & 4)
			z = -z;
		prepos.Set(x, y, z, 1.0f);
		x = pos.x;
		if (flip & 1)
			x = -x;
		y = pos.y;
		if (flip & 2)
			y = -y;
		z = pos.z;
		if (flip & 4)
			z = -z;
		vec4 t(x, y, z, 1.0f);
		local_mat = translation_matrix(t);
		local_mat = local_mat * local_rot;
		mat4 sm = scale_matrix(scale);
		local_mat = local_mat * sm;
		local_dirty = 0;
	}
	if (world_dirty) {
		upmat_count_W++;
		/* identity for the root (the scene), otherwise the parent matrices */
		mat4 pw(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		mat4 prot(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		    1.0f);
		if (this != (Object *)scene) {
			pw = parent->UpdateMatrix();
			prot = parent->world_rot;
		}
		world_mat = pw * local_mat;
		world_inv = inverse_trs(world_mat);
		world_rot = prot * local_rot;
		world_dirty = 0;
	}
	return world_mat;
}

void Object::CleanupReference()
{
	if (reference) {
		for (int i = 0; i < (int)(reference->end() - reference->begin()); i++) {
			Object *o = (*reference)[i];
			if (o)
				RemoveReference(o->referenced_by, this);
		}
		delete reference;
		reference = 0;
	}
	if (referenced_by) {
		for (int i = 0; i < (int)(referenced_by->end() - referenced_by->begin()); i++) {
			Object *o = (*referenced_by)[i];
			if (o)
				RemoveReference(o->reference, this);
		}
		delete referenced_by;
		referenced_by = 0;
	}
}

static int AddReference(ObjVec *&v, Object *obj)
{
	if (!v)
		v = new ObjVec;
	int no = SearchReference(v, 0);
	if (no >= 0) {
		(*v)[no] = obj;
	} else {
		no = v->end() - v->begin();
		v->push_back(obj);
	}
	return no;
}

void Object::SetSuicideTimer(int time)
{
	if (time > 0)
		suicide_timer = time;
	else
		suicide_timer = 1;
}

int Object::CountDownSuicide()
{
	if (suicide_timer <= 0)
		return 0;
	suicide_timer--;
	return suicide_timer <= 0;
}

void Object::Draw(gm_graph &g)
{
	UpdateMatrix();
	DrawChildren(g);
}

void Object::DrawThis(gm_graph &g)
{}

void Object::DrawChildren(gm_graph &g)
{
	if (!children_visible)
		return;
	for (Object **p = children.begin(); p != children.end(); p++)
		(*p)->Draw(g);
}

void Object::Anim()
{
	DoCallback(0, 0);
	AnimChildren();
}

void Object::SetColor(float r, float g, float b, float a)
{
	color.Set(r, g, b, a);
}

void Object::SetColor(const vec4 &c)
{
	color = c;
}

const vec4 &Object::GetColor() const
{
	return color;
}

const vec4 &Object::GetWorldColor() const
{
	Object *s = scene;
	if (!s->color_enable)
		return color;
	static vec4 world_color;
	world_color = s->color * color;
	return world_color;
}

void Object::SetPos(float x, float y, float z, Object *o)
{
	if (o) {
		vec4 t(x, y, z, 1.0f);
		pos = o->GetPos(parent, t);
	} else {
		pos.Set(x, y, z, 1.0f);
	}
	SetLocalDirty();
}

void Object::SetPos(const vec4 &v, Object *o)
{
	if (o)
		pos = o->GetPos(parent, v);
	else
		pos = v;
	SetLocalDirty();
}

void Object::SetPrePos(float x, float y, float z, Object *o)
{
	if (o) {
		vec4 t(x, y, z, 1.0f);
		prepos = o->GetPos(parent, t);
	} else {
		prepos.Set(x, y, z, 1.0f);
	}
}

void Object::SetPrePos(const vec4 &v, Object *o)
{
	if (o)
		prepos = o->GetPos(parent, v);
	else
		prepos = v;
}

void Object::SetScale(float x, float y, float z)
{
	vec4 *s = &scale;
	s->x = x;
	s->y = y;
	s->z = z;
	s->w = 1.0f;
	SetLocalDirty();
}

void Object::SetScale(const vec4 &s)
{
	scale = s;
	SetLocalDirty();
}

const vec4 &Object::GetScale()
{
	return scale;
}

void Object::SetLocalScale(float x, float y, float z)
{
	vec4 *s = &local_scale;
	s->x = x;
	s->y = y;
	s->z = z;
	s->w = 1.0f;
}

void Object::SetLocalScale(const vec4 &s)
{
	local_scale = s;
}

const vec4 &Object::GetLocalScale()
{
	return local_scale;
}

void Object::SetQuat(const vec4 &q, Object *o)
{
	if (o)
		local_rot = mat4_quat(o->GetQuat(parent, q));
	else
		local_rot = mat4_quat(q);
	SetLocalDirty();
}

void Object::SetLocalDirty()
{
	local_dirty = 1;
	SetWorldDirty();
}

void Object::SetWorldDirty()
{
	if (world_dirty)
		return;
	world_dirty = 1;
	for (Object **p = children.begin(); p != children.end(); p++)
		(*p)->SetWorldDirty();
}

void Object::SetTexture(int texture, int option, int mode)
{
	option_state.SetTexture(texture, option, mode);
}

void Object::SetObject(Object *obj, int option, int mode)
{
	option_state.SetObject(obj, option, mode);
}

int Object::SetOption(int option, const int &value, unsigned int size)
{
	switch (option) {
	case OPT_ALPH:
		blend_mode = value;
		return 1;
	case OPT_ZSRT:
		zsort_mode = imin(imax(value, 0), 5);
		return 1;
	case OPT_DEBG:
		debug = value != 0;
		return 1;
	case OPT_FLIP:
		flip = value;
		return 1;
	case OPT_SPHV:
		sphere_visibility = value != 0;
		return 1;
	default:
		return option_state.SetOption(option, value, size);
	}
}

int Object::SetOption(int option, const float &value, unsigned int size)
{
	if (option != OPT_SPHR) {
		if (option == OPT_ZOFS) {
			zsort_offset = value;
			return 1;
		}
	} else {
		sphere_radius = value;
		return 1;
	}
	return option_state.SetOption(option, value, size);
}

int Object::SetOption(int option, const String &value)
{
	return 0;
}

int Object::SetOption(int option, const void *data, unsigned int size)
{
	return option_state.SetOption(option, data, size);
}

int Object::GetOption(int option, int &value, unsigned int size) const
{
	switch (option) {
	case OPT_ALPH:
		value = blend_mode;
		return 1;
	case OPT_DEBG:
		value = debug;
		return 1;
	case OPT_ZSRT:
		value = zsort_mode;
		return 1;
	case OPT_SPHV:
		value = sphere_visibility;
		return 1;
	default:
		return option_state.GetOption(option, value, size);
	}
}

int Object::GetOption(int option, float &value, unsigned int size) const
{
	if (option != OPT_SPHR) {
		if (option == OPT_ZOFS) {
			value = zsort_offset;
			return 1;
		}
	} else {
		value = sphere_radius;
		return 1;
	}
	return option_state.GetOption(option, value, size);
}

int Object::GetOption(int option, String &value) const
{
	return 0;
}

int Object::GetOption(int option, void *data, unsigned int size) const
{
	return option_state.GetOption(option, data, size);
}

int Object::SetItemOption(int item, int option, const int &value, unsigned int size)
{
	return 0;
}

int Object::SetItemOption(int item, int option, const float &value, unsigned int size)
{
	return 0;
}

int Object::SetItemOption(int item, int option, const String &value)
{
	return 0;
}

int Object::SetItemOption(int item, int option, const void *data, unsigned int size)
{
	return 0;
}

int Object::GetItemOption(int item, int option, int &value, unsigned int size) const
{
	return 0;
}

int Object::GetItemOption(int item, int option, float &value, unsigned int size) const
{
	return 0;
}

int Object::GetItemOption(int item, int option, String &value) const
{
	return 0;
}

int Object::GetItemOption(int item, int option, void *data, unsigned int size) const
{
	return 0;
}

int Object::CreateReference(Object *obj)
{
	if (!obj)
		return -1;
	if (obj == this)
		return -1;
	int no = AddReference(reference, obj);
	AddReference(obj->referenced_by, this);
	return no;
}

void Object::DeleteReference(int no)
{
	ObjVec *v = reference;
	if (!v)
		return;
	if (no < 0)
		return;
	if (no >= (int)(v->end() - v->begin()))
		return;
	Object *o = (*v)[no];
	if (!o)
		return;
	EraseReference(v, no);
	RemoveReference(o->referenced_by, this);
}

Object *Object::GetReference(int no) const
{
	ObjVec *v = reference;
	if (!v)
		return 0;
	if (no < 0 || no >= (int)(v->end() - v->begin()))
		return 0;
	return (*v)[no];
}

static void RemoveReference(ObjVec *v, Object *obj)
{
	if (!v)
		return;
	EraseReference(v, SearchReference(v, obj));
}

static void EraseReference(ObjVec *v, int no)
{
	if (!v)
		return;
	if (no < 0)
		return;
	if (no < (int)(v->end() - v->begin()))
		(*v)[no] = 0;
}

/* Helpers for the reference lists. */

static int SearchReference(ObjVec *v, Object *obj)
{
	if (!v)
		return -1;
	for (int i = 0; i < (int)v->size(); i++) {
		if ((*v)[i] == obj)
			return i;
	}
	return -1;
}
