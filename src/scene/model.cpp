/*
 * Model methods: drawing, model and texture binding, and the option
 * dispatchers for the model, its bones, materials, layers and motions.
 */

#pragma implementation "model.h"
#include "model.h"
#include "gfx/vec.h"
#include "gfx/gm_graph.h"
#include "modeldata.h"
/* Parse and result helpers shared with the other widgets. */
#include "script/parseresult.h"
#include "scene/optid.h"
#include <math.h>

/* Identity matrix and w = 1 row. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

Model::Model(Object *parent) : Object(parent)
{
	GetOptionState()->SetLightingMode(1);
	GetOptionState()->SetPixelTest(0x60);
	GetOptionState()->SetPixelWrite(0x1f);
	/* Always-active state: fog, texture mode/clamp/filter/crop, diffuse,
	   specular, emission, texture factor, and the four layers. */
	GetOptionState()->always_mask |= EFF_FOG_MODE | EFF_TEXTURE_MASK | EFF_MATERIAL_MASK;
	GetOptionState()->always_mask |= EFF_LAYER_MASK;

	flags = 0;
	model_inst = 0;
	model_data = 0;
	int no_ref = -1;
	for (int i = 3; i >= 0; --i)
		item_ref[i] = no_ref;
	for (int i = 0; i < 4; i++) {
		item_state[i] = 0;
		item_angle[i] = 60.0f;
	}
	face_mode = 0;
	fade_mode = 0;
	anti_mode = 0;
	particle_size = 1.0f;
	div_level = 1.0f;
}

Model::~Model()
{
	SetModel(0);
}

/* Replaces the shared ModelData and the private working copy: the gmo_model
   of `data` is copied into a new ModelData. */
void Model::SetModel(ModelData *data)
{
	flags &= ~MODEL_LAYER_WRAPS;

	if (model_inst) {
		ModelData *old_data = model_data;
		if (--old_data->refcount == 0)
			delete old_data;

		model_data = 0;
		delete model_inst;
		model_inst = 0;
	}

	if (data) {
		model_data = data;
		data->refcount++;

		ModelData *fresh = new ModelData(0);
		model_inst = fresh;

		fresh->copy(*data);
	}
}

/* Sets the texture of layer option - EFF_SLOT_LAYER0 on material `mode` of the private
   model, or on the global material when mode < 0. */
void Model::SetTexture(int texture, int option, int mode)
{
	int opt = option ? option : EFF_SLOT_LAYER0;
	if ((unsigned int)(opt - EFF_SLOT_LAYER0) >= 4)
		return;

	if (!model_inst)
		return;

	gmo_material *mat;
	if (mode < 0)
		mat = model_inst->global_material;
	else
		mat = model_inst->get_material(mode);

	if (mat)
		mat->set_layer_texture_handle(opt - EFF_SLOT_LAYER0, texture);
}

void Model::SetObject(Object *obj, int option, int mode)
{
	int index = option ? option - EFF_SLOT_LAYER0 : 0;
	if ((unsigned int)index < 4) {
		int *slot = &item_ref[index];
		DeleteReference(*slot);
		*slot = CreateReference(obj);
		int bit = MODEL_ITEM0_OBJECT << index;
		flags = obj ? (flags | bit) : (flags & ~bit);
	}
}

/* Option ids are FourCC codes. Options not handled directly fall back to
   GetItemOption(option, 0, ...) or to the void * overload. */

/* Draws the model: binds the referenced Objects as mappers, then draws
   with the anti-alias, fade and face passes, and advances the motions. */
void Model::DrawThis(gm_graph &g)
{
	if (!visible)
		return;
	if (!model_inst)
		return;

	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE_FIT);

	if (flags & MODEL_ITEM_OBJECTS) {
		int i = 0;
		int *p = item_ref;
		do {
			int ref = *p;
			if (ref >= 0) {
				Object *o = GetReference(ref);
				const mat4 *mm = o ? &o->world_inv : 0;
				model_inst->set_mapper(i, mm);
			}
			i++;
			p++;
		} while (i < 4);
	}

	model_inst->calc();
	g.ParticleSize(particle_size);
	g.PatchLevel(div_level);

	if (anti_mode) {
		int doit = (anti_mode == ANTI_ALWAYS);
		const vec4 &c = GetWorldColor();
		if (c.w > 0.99f) {
			g.Color(c.x, c.y, c.z, 1.0f);
			doit = 1;
		}
		if (doit) {
			int save = g.fFlagsE;
			int e = save | GM_ENABLE_ANTIALIAS;
			if (face_mode) {
				e |= GM_ENABLE_CULL_FACE | GM_ENABLE_FRONT_FACE;
				if (face_mode == FACE_BACK)
					e &= ~GM_ENABLE_FRONT_FACE;
			}
			g.SetEnableBits(e);
			model_inst->draw();
			g.Color(GetWorldColor());
			g.SetEnableBits(save);
		}
	}

	if (fade_mode && (fade_mode == FADE_ALWAYS || GetWorldColor().w < 0.99f)) {
		g.SetTexture(0);
		g.ColorMask(0);
		model_inst->set_option_flags(GMO_OPT_NO_MATERIALS);
		model_inst->draw();
		model_inst->clear_option_flags(GMO_OPT_NO_MATERIALS);
		g.ColorMask(option_state.GetColorMask());
	}

	if (face_mode) {
		int save = g.fFlagsE;
		int frontBits = save | GM_ENABLE_CULL_FACE;
		if (face_mode & FACE_BACK) {
			g.SetEnableBits(frontBits & ~GM_ENABLE_FRONT_FACE);
			model_inst->draw();
		}
		if (face_mode & FACE_FRONT) {
			g.SetEnableBits(save | GM_ENABLE_CULL_FACE | GM_ENABLE_FRONT_FACE);
			model_inst->draw();
		}
		g.SetEnableBits(save);
	} else {
		model_inst->draw();
	}

	model_inst->update(0.016666668f);
}

/* 'DISP' applies to every bone; 'TEXC'/'TXCn' also mark layer n wrap as set
   (MODEL_LAYER0_WRAP << n); bare 'MOT' clears items 1..3 (value -1) and then sets
   item 0. */
int Model::SetOption(int option, const int &value, unsigned int size)
{
	switch (option) {
	case OPT_FACE:
		face_mode = value;
		return 1;
	case OPT_FADE:
		fade_mode = value;
		return 1;
	case OPT_ANTI:
		anti_mode = value;
		return 1;
	case OPT_DISP: {
		if (!model_inst)
			return 0;
		int count = model_inst->get_bone_count();
		if (count > 0) {
			int i = 0;
			do {
				SetItemOption(option, i, value, size);
				i++;
			} while (i < model_inst->get_bone_count());
		}
		return 1;
	}
	case OPT_LAY:
		option = OPT_LAY0;
		/* fallthrough */
	case OPT_LAY0:
	case OPT_LAY1:
	case OPT_LAY2:
	case OPT_LAY3: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		int no = ((option >> 24) - '0') & 3;
		return SetLayerMode(mat, no, value);
	}
	case OPT_ALPH:
		option = OPT_ALP0;
		/* fallthrough */
	case OPT_ALP0:
	case OPT_ALP1:
	case OPT_ALP2:
	case OPT_ALP3: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		if (!mat)
			return 0;
		int no = ((option >> 24) - '0') & 3;
		mat->set_layer_blend_func(no, value);
		return 1;
	}
	case OPT_TEXM:
		option = OPT_TXM0;
		/* fallthrough */
	case OPT_TXM0:
	case OPT_TXM1:
	case OPT_TXM2:
	case OPT_TXM3: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		int no = ((option >> 24) - '0') & 3;
		return SetTextureMode(mat, no, value);
	}
	case OPT_TEXC:
		option = OPT_TXC0;
		/* fallthrough */
	case OPT_TXC0:
	case OPT_TXC1:
	case OPT_TXC2:
	case OPT_TXC3: {
		if (!model_inst)
			return 0;
		int no = ((option >> 24) - '0') & 3;
		flags |= MODEL_LAYER0_WRAP << no;
		gmo_material *mat = model_inst->global_material;
		return SetTextureClamp(mat, no, value);
	}
	case OPT_TEXF:
		option = OPT_TXF0;
		/* fallthrough */
	case OPT_TXF0:
	case OPT_TXF1:
	case OPT_TXF2:
	case OPT_TXF3: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		int no = ((option >> 24) - '0') & 3;
		return SetTextureFilter(mat, no, value);
	}
	case OPT_MOT: {
		if (!model_inst)
			return 0;
		SetItemOption(option, 1, -1, 1);
		SetItemOption(option, 2, -1, 1);
		SetItemOption(option, 3, -1, 1);
		return SetItemOption(option, 0, value, size);
	}
	}
	return Object::SetOption(option, value, size);
}

/* The motion options 'MOTI'/'MOTT'/'MOTS'/'MOTF' apply to all four items. */
int Model::SetOption(int option, const float &value, unsigned int size)
{
	switch (option) {
	case OPT_SIZE:
		particle_size = value;
		return 1;
	case OPT_DIVL:
		div_level = value;
		return 1;
	case OPT_MOTI:
	case OPT_MOTT:
	case OPT_MOTS:
	case OPT_MOTF:
		if (!model_inst)
			return 0;
		for (int i = 0; i < 4; i++)
			SetItemOption(option, i, value, size);
		return 1;
	case OPT_CROP:
	case OPT_FACT:
	case OPT_MWGT:
	case OPT_EMIS:
	case OPT_SCAL:
	case OPT_MOTL:
	case OPT_CRP2:
	case OPT_SPEC:
	case OPT_DIFF:
	case OPT_CRP3:
	case OPT_ROT:
	case OPT_CRP0:
	case OPT_CRP1:
	case OPT_POS:
		return SetOption(option, &value, size * 4);
	}
	return Object::SetOption(option, value, size);
}

/* The vec4 argument is parsed once, before the dispatch (missing components
   default to {0, 0, 0, 1}). 'MWGT'/'SCAL'/'ROT'/'POS' apply to every bone;
   bare 'CROP' is an alias of 'CRP0'. */
int Model::SetOption(int option, const void *data, unsigned int size)
{
	int n = size >> 2;
	const float *f = (const float *)data;
	vec4 tmp;
	if (n < 4) {
		float *p = &tmp.x;
		p[0] = 0.0f;
		p[1] = 0.0f;
		p[2] = 0.0f;
		p[3] = 1.0f;
	}
	ParseVec4(tmp, f, n);

	switch (option) {
	case OPT_MWGT:
	case OPT_SCAL:
	case OPT_ROT:
	case OPT_POS: {
		if (!model_inst)
			return 0;
		for (int i = 0; i < model_inst->get_bone_count(); i++)
			SetItemOption(option, i, data, size);
		return 1;
	}
	case OPT_DIFF: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		if (!mat)
			return 0;
		mat->set_diffuse(tmp);
		return 1;
	}
	case OPT_SPEC: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		if (!mat)
			return 0;
		float p = 0.0f;
		if (n >= 5)
			p = f[4];
		mat->set_specular(tmp, p);
		return 1;
	}
	case OPT_EMIS: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		if (!mat)
			return 0;
		mat->set_emission(tmp);
		return 1;
	}
	case OPT_FACT: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		if (!mat)
			return 0;
		float a = 0.0f;
		if (n >= 5)
			a = f[4];
		float b = 1.0f;
		if (n >= 6)
			b = f[5];
		mat->set_tex_factor(tmp, a, b);
		return 1;
	}
	case OPT_CROP:
		option = OPT_CRP0;
		/* fallthrough */
	case OPT_CRP0:
	case OPT_CRP1:
	case OPT_CRP2:
	case OPT_CRP3: {
		if (!model_inst)
			return 0;
		if (n < 3)
			tmp.z = 1.0f;
		gmo_material *mat = model_inst->global_material;
		if (!mat)
			return 0;
		int no = ((option >> 24) - '0') & 3;
		mat->set_layer_tex_crop(no, tmp);
		return 1;
	}
	case OPT_MOTL:
		if (!model_inst)
			return 0;
		return SetItemOption(OPT_MOTL, 0, data, size);
	}
	return Object::SetOption(option, data, size);
}

/* Per-item options on material or bone `item`. 'LIT'/'FOG' enable lighting
   (GM_ENABLE_LIGHTING) or fog (GM_ENABLE_FOG) on the material; 'MOT' restores the saved
   frame rate of the current motion, saves the frame rate of motion `value`
   and makes it current. */
int Model::SetItemOption(int option, int item, const int &value, unsigned int size)
{
	if (!model_inst)
		return 0;
	switch (option) {
	case OPT_DISP: {
		gmo_bone *bone = model_inst->get_bone(item);
		if (!bone)
			return 0;
		bone->visible_parts = value;
		return 1;
	}
	case OPT_LIT: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		mat->enable(GM_ENABLE_LIGHTING, value & 1);
		return 1;
	}
	case OPT_FOG: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		mat->enable(GM_ENABLE_FOG, value & 1);
		return 1;
	}
	case OPT_LAY:
		option = OPT_LAY0;
		/* fallthrough */
	case OPT_LAY0:
	case OPT_LAY1:
	case OPT_LAY2:
	case OPT_LAY3: {
		gmo_material *mat = model_inst->get_material(item);
		int no = ((option >> 24) - '0') & 3;
		return SetLayerMode(mat, no, value);
	}
	case OPT_ALPH:
		option = OPT_ALP0;
		/* fallthrough */
	case OPT_ALP0:
	case OPT_ALP1:
	case OPT_ALP2:
	case OPT_ALP3: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		int no = ((option >> 24) - '0') & 3;
		mat->set_layer_blend_func(no, value);
		return 1;
	}
	case OPT_TEXM:
		option = OPT_TXM0;
		/* fallthrough */
	case OPT_TXM0:
	case OPT_TXM1:
	case OPT_TXM2:
	case OPT_TXM3: {
		gmo_material *mat = model_inst->get_material(item);
		int no = ((option >> 24) - '0') & 3;
		return SetTextureMode(mat, no, value);
	}
	case OPT_TEXC:
		option = OPT_TXC0;
		/* fallthrough */
	case OPT_TXC0:
	case OPT_TXC1:
	case OPT_TXC2:
	case OPT_TXC3: {
		gmo_material *mat = model_inst->get_material(item);
		int no = ((option >> 24) - '0') & 3;
		return SetTextureClamp(mat, no, value);
	}
	case OPT_TEXF:
		option = OPT_TXF0;
		/* fallthrough */
	case OPT_TXF0:
	case OPT_TXF1:
	case OPT_TXF2:
	case OPT_TXF3: {
		gmo_material *mat = model_inst->get_material(item);
		int no = ((option >> 24) - '0') & 3;
		return SetTextureFilter(mat, no, value);
	}
	case OPT_MOT: {
		if ((unsigned int)item >= 4)
			return 0;
		int id = model_inst->get_current_motion(item);
		gmo_motion *inst = model_inst->get_motion(id);
		if (inst)
			inst->set_frame_rate(item_angle[item]);

		gmo_motion *next = model_inst->get_motion(value);
		if (next)
			item_angle[item] = next->frame_rate;

		model_inst->set_current_motion(item, value, 0.0f, item_statef[item]);
		return 1;
	}
	}
	return Object::SetItemOption(option, item, value, size);
}

/* Motion options act on the gmo_motion of the item: 'MOTF' frame, 'MOTT'
   time, 'MOTS' frame rate; 'MOTI' always stores item_statef[item] and also
   sets the interpolation when the motion exists. */
int Model::SetItemOption(int option, int item, const float &value, unsigned int size)
{
	if (!model_inst)
		return 0;

	switch (option) {
	case OPT_MOTF: {
		if ((unsigned int)item >= 4)
			return 0;
		int id = model_inst->get_current_motion(item);
		gmo_motion *inst = model_inst->get_motion(id);
		if (!inst)
			return 0;
		inst->set_frame(value);
		return 1;
	}
	case OPT_MOTT: {
		if ((unsigned int)item >= 4)
			return 0;
		int id = model_inst->get_current_motion(item);
		gmo_motion *inst = model_inst->get_motion(id);
		if (!inst)
			return 0;
		inst->set_time(value);
		return 1;
	}
	case OPT_MOTS: {
		if ((unsigned int)item >= 4)
			return 0;
		int id = model_inst->get_current_motion(item);
		gmo_motion *inst = model_inst->get_motion(id);
		if (!inst)
			return 0;
		inst->set_frame_rate(value);
		return 1;
	}
	case OPT_MOTI: {
		if ((unsigned int)item >= 4)
			return 0;
		item_statef[item] = value;

		int id = model_inst->get_current_motion(item);
		gmo_motion *inst = model_inst->get_motion(id);
		if (!inst)
			return 0;
		inst->set_interp(value);
		return 1;
	}
	case OPT_CRP0:
	case OPT_CRP1:
	case OPT_CRP2:
	case OPT_CRP3:
	case OPT_ROT:
	case OPT_SPEC:
	case OPT_SCAL:
	case OPT_CROP:
	case OPT_MWGT:
	case OPT_POS:
	case OPT_DIFF:
	case OPT_EMIS:
	case OPT_FACT:
	case OPT_MOTL:
		return SetItemOption(option, item, &value, size * 4);
	}
	return Object::SetItemOption(option, item, value, size);
}

/* Per-item vec4 options on bone or material `item`. 'ROT' converts Euler
   angles (radians) to a quaternion with the integer sine/cosine tables;
   'MOTL' sets the loop start and end of the current motion. */
int Model::SetItemOption(int option, int item, const void *data, unsigned int size)
{
	if (!model_inst)
		return 0;

	int n = size >> 2;
	const float *f = (const float *)data;
	vec4 tmp;
	if (n < 4) {
		float *p = &tmp.x;
		p[0] = 0.0f;
		p[1] = 0.0f;
		p[2] = 0.0f;
		p[3] = 1.0f;
	}
	ParseVec4(tmp, f, n);

	switch (option) {
	case OPT_POS: {
		gmo_bone *bone = model_inst->get_bone(item);
		if (!bone)
			return 0;
		bone->set_translate(tmp);
		return 1;
	}
	case OPT_ROT: {
		gmo_bone *bone = model_inst->get_bone(item);
		if (!bone)
			return 0;
		const vec4 &e = tmp;
		float hx = e.x * 0.5f;
		float hy = e.y * 0.5f;
		float hz = e.z * 0.5f;
		int ax = (int)(hx * 32768.0f / 3.1415927f);
		float cx = gmGetTableCos(ax);
		float sx = gmGetTableSin(ax);
		int ay = (int)(hy * 32768.0f / 3.1415927f);
		float cy = gmGetTableCos(ay);
		float sy = gmGetTableSin(ay);
		int az = (int)(hz * 32768.0f / 3.1415927f);
		float cz = gmGetTableCos(az);
		float sz = gmGetTableSin(az);
		vec4 q;
		q.x = cy * cz * sx + sy * cx * sz;
		q.y = sy * cx * cz - cy * sx * sz;
		q.z = cy * cx * sz - sy * cz * sx;
		q.w = cy * cx * cz + sy * sx * sz;
		bone->set_rotate(q);
		return 1;
	}
	case OPT_SCAL: {
		gmo_bone *bone = model_inst->get_bone(item);
		if (!bone)
			return 0;
		bone->set_scale(tmp);
		return 1;
	}
	case OPT_MWGT: {
		gmo_bone *bone = model_inst->get_bone(item);
		if (!bone)
			return 0;
		bone->set_morph_weights(f, n);
		return 1;
	}
	case OPT_DIFF: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		mat->set_diffuse(tmp);
		return 1;
	}
	case OPT_SPEC: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		float pw = 0.0f;
		if (n >= 5)
			pw = f[4];
		mat->set_specular(tmp, pw);
		return 1;
	}
	case OPT_EMIS: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		mat->set_emission(tmp);
		return 1;
	}
	case OPT_FACT: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		float a = 0.0f;
		if (n >= 5)
			a = f[4];
		float b = 1.0f;
		if (n >= 6)
			b = f[5];
		mat->set_tex_factor(tmp, a, b);
		return 1;
	}
	case OPT_CROP:
		option = OPT_CRP0;
		/* fallthrough */
	case OPT_CRP0:
	case OPT_CRP1:
	case OPT_CRP2:
	case OPT_CRP3: {
		if (n < 3)
			tmp.z = 1.0f;
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		int no = ((option >> 24) - '0') & 3;
		mat->set_layer_tex_crop(no, tmp);
		return 1;
	}
	case OPT_MOTL: {
		if ((unsigned int)item >= 4)
			return 0;
		int id = model_inst->get_current_motion(item);
		gmo_motion *inst = model_inst->get_motion(id);
		if (!inst)
			return 0;
		inst->set_loop_start(f[0]);
		float e;
		if (n < 2)
			e = f[0];
		else
			e = f[1];
		inst->set_loop_end(e);
		return 1;
	}
	}
	return Object::SetItemOption(option, item, data, size);
}

/* 'FACE'/'FADE'/'ANTI' are fields of Model; 'DISP' and bare 'MOT' read item
   0; the layer options act on the global material, layer
   ((option >> 24) - '0') & 3. Bare 'LAY'/'ALPH'/'TEXM'/'TEXC'/'TEXF' are
   aliases of layer 0. */
int Model::GetOption(int option, int &value, unsigned int size) const
{
	switch (option) {
	case OPT_FACE:
		value = face_mode;
		return 1;
	case OPT_FADE:
		value = fade_mode;
		return 1;
	case OPT_ANTI:
		value = anti_mode;
		return 1;
	case OPT_DISP:
		if (!model_inst)
			return 0;
		return GetItemOption(option, 0, value, size);
	case OPT_LAY:
		option = OPT_LAY0;
		/* fallthrough */
	case OPT_LAY0:
	case OPT_LAY1:
	case OPT_LAY2:
	case OPT_LAY3: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		int no = ((option >> 24) - '0') & 3;
		return GetLayerMode(mat, no, value);
	}
	case OPT_ALPH:
		option = OPT_ALP0;
		/* fallthrough */
	case OPT_ALP0:
	case OPT_ALP1:
	case OPT_ALP2:
	case OPT_ALP3: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		if (!mat)
			return 0;
		int no = ((option >> 24) - '0') & 3;
		mat->get_layer_blend_func(no, value);
		return 1;
	}
	case OPT_TEXM:
		option = OPT_TXM0;
		/* fallthrough */
	case OPT_TXM0:
	case OPT_TXM1:
	case OPT_TXM2:
	case OPT_TXM3: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		int no = ((option >> 24) - '0') & 3;
		return GetTextureMode(mat, no, value);
	}
	case OPT_TEXC:
		option = OPT_TXC0;
		/* fallthrough */
	case OPT_TXC0:
	case OPT_TXC1:
	case OPT_TXC2:
	case OPT_TXC3: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		int no = ((option >> 24) - '0') & 3;
		return GetTextureClamp(mat, no, value);
	}
	case OPT_TEXF:
		option = OPT_TXF0;
		/* fallthrough */
	case OPT_TXF0:
	case OPT_TXF1:
	case OPT_TXF2:
	case OPT_TXF3: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		int no = ((option >> 24) - '0') & 3;
		return GetTextureFilter(mat, no, value);
	}
	case OPT_MOT:
		if (!model_inst)
			return 0;
		return GetItemOption(option, 0, value, size);
	}
	return Object::GetOption(option, value, size);
}
int Model::GetOption(int option, float &value, unsigned int size) const
{
	switch (option) {
	case OPT_SIZE:
		value = particle_size;
		return 1;
	case OPT_DIVL:
		value = div_level;
		return 1;
	case OPT_MOTI:
	case OPT_MOTT:
	case OPT_MOTS:
	case OPT_MOTF:
		if (!model_inst)
			return 0;
		return GetItemOption(option, 0, value, size);
	case OPT_EMIS:
	case OPT_MWGT:
	case OPT_BBOX:
	case OPT_FACT:
	case OPT_MOTL:
	case OPT_CROP:
	case OPT_SCAL:
	case OPT_CRP2:
	case OPT_DIFF:
	case OPT_CRP3:
	case OPT_SPEC:
	case OPT_ROT:
	case OPT_CRP0:
	case OPT_CRP1:
	case OPT_POS:
		return GetOption(option, &value, size * 4);
	}
	return Object::GetOption(option, value, size);
}

/* 'BBOX' returns the 8 values of the model bounding box; bare 'CROP' is an
   alias of 'CRP0'. */
int Model::GetOption(int option, void *data, unsigned int size) const
{
	int n = size >> 2;
	float *f = (float *)data;
	switch (option) {
	case OPT_BBOX: {
		if (!model_inst)
			return 0;
		GmoBoundingBox *box = model_inst->bounding_box;
		if (!box)
			return 0;
		ResultFloatArray((const float *)box, 8, f, n);
		return 1;
	}
	case OPT_MWGT:
	case OPT_SCAL:
	case OPT_ROT:
	case OPT_POS:
		if (!model_inst)
			return 0;
		return GetItemOption(option, 0, data, size);
	case OPT_DIFF: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		if (!mat)
			return 0;
		vec4 v;
		mat->get_diffuse(v);
		ResultVec4(v, f, n);
		return 1;
	}
	case OPT_SPEC: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		if (!mat)
			return 0;
		vec4 v;
		float p;
		mat->get_specular(v, p);
		ResultVec4(v, f, n);
		if (n >= 5)
			f[4] = p;
		return 1;
	}
	case OPT_EMIS: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		if (!mat)
			return 0;
		vec4 v;
		mat->get_emission(v);
		ResultVec4(v, f, n);
		return 1;
	}
	case OPT_FACT: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		if (!mat)
			return 0;
		vec4 v;
		float a, b;
		mat->get_tex_factor(v, a, b);
		ResultVec4(v, f, n);
		if (n >= 5)
			f[4] = a;
		if (n >= 6)
			f[5] = b;
		return 1;
	}
	case OPT_CROP:
		option = OPT_CRP0;
		/* fallthrough */
	case OPT_CRP0:
	case OPT_CRP1:
	case OPT_CRP2:
	case OPT_CRP3: {
		if (!model_inst)
			return 0;
		gmo_material *mat = model_inst->global_material;
		if (!mat)
			return 0;
		int no = ((option >> 24) - '0') & 3;
		vec4 v;
		mat->get_layer_tex_crop(no, v);
		ResultVec4(v, f, n);
		return 1;
	}
	case OPT_MOTL:
		if (!model_inst)
			return 0;
		return GetItemOption(OPT_MOTL, 0, data, size);
	}
	return Object::GetOption(option, data, size);
}

/* Per-item options on material or bone `item`; bare 'MOT' returns the
   current motion of the item. */
int Model::GetItemOption(int option, int item, int &value, unsigned int size) const
{
	if (!model_inst)
		return 0;
	switch (option) {
	case OPT_DISP: {
		gmo_bone *bone = model_inst->get_bone(item);
		if (!bone)
			return 0;
		value = bone->visible_parts;
		return 1;
	}
	case OPT_LIT: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		int v;
		if (mat->enable_mask & GM_ENABLE_LIGHTING)
			v = mat->enable_value;
		else
			v = option_state.GetLightingMode();
		value = v & 1;
		return 1;
	}
	case OPT_FOG: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		int v;
		if (mat->enable_mask & GM_ENABLE_FOG)
			v = (int)((unsigned int)mat->enable_value >> 18); /* GM_ENABLE_FOG */
		else
			v = option_state
			        .GetLightingMode(); /* NOTE: bug: reads the lighting mode instead of the fog mode */
		value = v & 1;
		return 1;
	}
	case OPT_LAY:
		option = OPT_LAY0;
		/* fallthrough */
	case OPT_LAY0:
	case OPT_LAY1:
	case OPT_LAY2:
	case OPT_LAY3: {
		gmo_material *mat = model_inst->get_material(item);
		int no = ((option >> 24) - '0') & 3;
		return GetLayerMode(mat, no, value);
	}
	case OPT_ALPH:
		option = OPT_ALP0;
		/* fallthrough */
	case OPT_ALP0:
	case OPT_ALP1:
	case OPT_ALP2:
	case OPT_ALP3: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		int no = ((option >> 24) - '0') & 3;
		mat->get_layer_blend_func(no, value);
		return 1;
	}
	case OPT_TEXM:
		option = OPT_TXM0;
		/* fallthrough */
	case OPT_TXM0:
	case OPT_TXM1:
	case OPT_TXM2:
	case OPT_TXM3: {
		gmo_material *mat = model_inst->get_material(item);
		int no = ((option >> 24) - '0') & 3;
		return GetTextureMode(mat, no, value);
	}
	case OPT_TEXC:
		option = OPT_TXC0;
		/* fallthrough */
	case OPT_TXC0:
	case OPT_TXC1:
	case OPT_TXC2:
	case OPT_TXC3: {
		gmo_material *mat = model_inst->get_material(item);
		int no = ((option >> 24) - '0') & 3;
		return GetTextureClamp(mat, no, value);
	}
	case OPT_TEXF:
		option = OPT_TXF0;
		/* fallthrough */
	case OPT_TXF0:
	case OPT_TXF1:
	case OPT_TXF2:
	case OPT_TXF3: {
		gmo_material *mat = model_inst->get_material(item);
		int no = ((option >> 24) - '0') & 3;
		return GetTextureFilter(mat, no, value);
	}
	case OPT_MOT:
		if ((unsigned int)item >= 4)
			return 0;
		value = model_inst->get_current_motion(item);
		return 1;
	}
	return Object::GetItemOption(option, item, value, size);
}

/* 'MOTF'/'MOTT'/'MOTS' read the current motion of the item, 'MOTI' the
   interpolation stored in item_statef[item]. */
int Model::GetItemOption(int option, int item, float &value, unsigned int size) const
{
	if (!model_inst)
		return 0;

	switch (option) {
	case OPT_MOTF: {
		if ((unsigned int)item >= 4)
			return 0;
		int id = model_inst->get_current_motion(item);
		gmo_motion *inst = model_inst->get_motion(id);
		if (!inst)
			return 0;
		value = inst->current_frame;
		return 1;
	}
	case OPT_MOTT: {
		if ((unsigned int)item >= 4)
			return 0;
		int id = model_inst->get_current_motion(item);
		gmo_motion *inst = model_inst->get_motion(id);
		if (!inst)
			return 0;
		value = inst->time;
		return 1;
	}
	case OPT_MOTS: {
		if ((unsigned int)item >= 4)
			return 0;
		int id = model_inst->get_current_motion(item);
		gmo_motion *inst = model_inst->get_motion(id);
		if (!inst)
			return 0;
		value = inst->frame_rate;
		return 1;
	}
	case OPT_MOTI:
		if ((unsigned int)item >= 4)
			return 0;
		{
			const int *q = item_state;
			q += item;
			value = *(float *)q;
		}
		return 1;
	case OPT_CRP0:
	case OPT_CRP1:
	case OPT_CRP2:
	case OPT_CRP3:
	case OPT_ROT:
	case OPT_SPEC:
	case OPT_SCAL:
	case OPT_CROP:
	case OPT_MWGT:
	case OPT_POS:
	case OPT_DIFF:
	case OPT_EMIS:
	case OPT_FACT:
	case OPT_BBOX:
		return GetItemOption(option, item, &value, size * 4);
	}
	return Object::GetItemOption(option, item, value, size);
}

/* Local copy of quat_to_rotyxz(). */
static inline vec4 l_quat_to_rotyxz(const vec4 &q)
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

/* Per-item vec4 options, the inverse of SetItemOption(void *): 'BBOX' is the
   bone bounding box, 'ROT' converts the quaternion back to Euler angles,
   'MOTL' returns the loop start and end of the current motion. */
int Model::GetItemOption(int option, int item, void *data, unsigned int size) const
{
	if (!model_inst)
		return 0;

	int n = size >> 2;
	float *f = (float *)data;

	switch (option) {
	case OPT_BBOX: {
		gmo_bone *bone = model_inst->get_bone(item);
		if (!bone)
			return 0;
		GmoBoundingBox *box = bone->bounding_box;
		if (!box)
			return 0;
		ResultFloatArray((const float *)box, 8, f, n);
		return 1;
	}
	case OPT_POS: {
		gmo_bone *bone = model_inst->get_bone(item);
		if (!bone)
			return 0;
		vec4 v;
		bone->get_translate(v);
		ResultVec4(v, f, n);
		return 1;
	}
	case OPT_ROT: {
		gmo_bone *bone = model_inst->get_bone(item);
		if (!bone)
			return 0;
		vec4 v;
		bone->get_rotate(v);
		v = l_quat_to_rotyxz(v);
		ResultVec4(v, f, n);
		return 1;
	}
	case OPT_SCAL: {
		gmo_bone *bone = model_inst->get_bone(item);
		if (!bone)
			return 0;
		vec4 v;
		bone->get_scale(v);
		ResultVec4(v, f, n);
		return 1;
	}
	case OPT_MWGT: {
		gmo_bone *bone = model_inst->get_bone(item);
		if (!bone)
			return 0;
		ResultFloatArray(bone->get_morph_weights(), f, n);
		return 1;
	}
	case OPT_DIFF: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		vec4 v;
		mat->get_diffuse(v);
		ResultVec4(v, f, n);
		return 1;
	}
	case OPT_SPEC: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		vec4 v;
		float p;
		mat->get_specular(v, p);
		ResultVec4(v, f, n);
		if (n >= 5)
			f[4] = p;
		return 1;
	}
	case OPT_EMIS: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		vec4 v;
		mat->get_emission(v);
		ResultVec4(v, f, n);
		return 1;
	}
	case OPT_FACT: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		vec4 v;
		float a, b;
		mat->get_tex_factor(v, a, b);
		ResultVec4(v, f, n);
		if (n >= 5)
			f[4] = a;
		if (n >= 6)
			f[5] = b;
		return 1;
	}
	case OPT_CROP:
		option = OPT_CRP0;
		/* fallthrough */
	case OPT_CRP0:
	case OPT_CRP1:
	case OPT_CRP2:
	case OPT_CRP3: {
		gmo_material *mat = model_inst->get_material(item);
		if (!mat)
			return 0;
		int no = ((option >> 24) - '0') & 3;
		vec4 v;
		mat->get_layer_tex_crop(no, v);
		ResultVec4(v, f, n);
		return 1;
	}
	case OPT_MOTL: {
		if ((unsigned int)item >= 4)
			return 0;
		int id = model_inst->get_current_motion(item);
		gmo_motion *inst = model_inst->get_motion(id);
		if (!inst)
			return 0;
		if (n > 0)
			f[0] = inst->loop_start;
		if (n >= 2)
			f[1] = inst->loop_end;
		return 1;
	}
	}
	return Object::GetItemOption(option, item, data, size);
}

/* --- SetLayerMode: the packed layer mode of EffOptionState::ActivateLayerState. --- */

static int layer_color_gen_table[4] = { 2, 0, 1, 2 };
static int layer_tex_gen_table[4] = { 0, 1, 2, 3 };
static int layer_tex_gen_param[8] = {
	GM_TEXGEN_TEXCOORD,
	GM_TEXGEN_TEXCOORD,
	GM_TEXGEN_TEXCOORD,
	GM_TEXGEN_TEXCOORD,
	GM_TEXGEN_POSITION,
	GM_TEXGEN_NORMAL,
	GM_TEXGEN_REFLECTION,
	GM_TEXGEN_TEXCOORD,
};
static int layer_map_coord_table[4] = { 0, 1, 2, 2 };

int Model::SetLayerMode(gmo_material *mat, int no, int mode)
{
	if (!mat)
		return 0;

	if (no != 0) {
		int blend = mode & 0xffff;
		if (blend == 0xffff) {
			mat->set_layer_blend_func(no, -1);
			return 1;
		}
		mat->set_layer_blend_func(no, blend);
	}
	int cgb = (mode & 0x00080000) != 0;
	mat->set_layer_color_gen(no, layer_color_gen_table[(mode >> 16) & 3], cgb);

	if (!((MODEL_LAYER0_WRAP << no) & flags)) {
		int wrap = (mode & 0x08000000) != 0;
		mat->set_layer_tex_wrap(no, wrap, wrap);
	}

	int gen = layer_tex_gen_table[(mode >> 24) & 3];
	int gen_param = layer_tex_gen_param[(mode >> 20) & 7];

	if (gen == 0) {
		int cur_mode, cur_arg;
		mat->get_layer_tex_gen(no, cur_mode, cur_arg);
		if (cur_mode != 0) {
			static vec4 crop1(0.0f, 0.0f, 1.0f, 1.0f);
			mat->set_layer_tex_crop(no, crop1);
		}
	} else {
		static vec4 crop2(0.5f, 0.5f, -1.0f, -1.0f);
		static vec4 crop3(0.5f, 0.5f, -0.5f, -0.5f);
		mat->set_layer_tex_crop(no, (mode & 0x00800000) ? crop3 : crop2);
	}

	mat->set_layer_tex_gen(no, gen, gen_param);
	mat->set_layer_map_coord(no, layer_map_coord_table[(mode >> 28) & 3], (mode & 0x00800000) ? 16 : 0);
	return 1;
}

/* --- Texture mode, clamp and filter: translate the packed mode with small tables. --- */

static int tex_func_table[2] = { GM_TEXFUNC_MODULATE, GM_TEXFUNC_DECAL };
static int tex_func_mode[2] = { GM_TEXFUNC_RGBA, GM_TEXFUNC_RGB };

int Model::SetTextureMode(gmo_material *mat, int no, int mode)
{
	if (!mat)
		return 0;
	mat->set_layer_tex_func(no, tex_func_table[mode & 1], tex_func_mode[(mode >> 4) & 1]);
	return 1;
}

static int tex_wrap_table[2] = { GM_WRAP_REPEAT, GM_WRAP_CLAMP };
static inline int tex_wrap(int mode)
{
	return tex_wrap_table[mode & 1];
}

int Model::SetTextureClamp(gmo_material *mat, int no, int mode)
{
	if (!mat)
		return 0;
	mat->set_layer_tex_wrap(no, tex_wrap(mode), tex_wrap(mode >> 4));
	return 1;
}

static int tex_filter_mag[2] = { GM_FILTER_NEAREST, GM_FILTER_LINEAR };
static int tex_filter_min[8] = {
	GM_FILTER_NEAREST_MIPMAP_NEAREST,
	GM_FILTER_LINEAR_MIPMAP_NEAREST,
	GM_FILTER_NEAREST_MIPMAP_LINEAR,
	GM_FILTER_LINEAR_MIPMAP_LINEAR,
	GM_FILTER_NEAREST,
	GM_FILTER_LINEAR,
	GM_FILTER_NEAREST,
	GM_FILTER_LINEAR,
};

int Model::SetTextureFilter(gmo_material *mat, int no, int mode)
{
	if (!mat)
		return 0;
	mat->set_layer_tex_filter(no, tex_filter_mag[mode & 1], tex_filter_min[((mode >> 4) & 1) | ((mode >> 7) & 6)]);
	return 1;
}

/* --- GetLayerMode and the texture getters: the inverse mappings. --- */
static int rev_color_gen_a[4] = { 0x10000, 0x20000, 0, 0 };
static int rev_color_gen_b[2] = { 0, 0x80000 };
static int rev_gen_table[4] = { 0, 0x1000000, 0x2000000, 0x3000000 };
static int rev_map_a[4] = { 0, 0x10000000, 0x20000000, 0x20000000 };
static int rev_map_b[2] = { 0, 0x800000 };

int Model::GetLayerMode(gmo_material *mat, int no, int &mode) const
{
	if (!mat)
		return 0;

	int a, b;

	mat->get_layer_blend_func(no, a);
	mode = (unsigned short)a;

	mat->get_layer_color_gen(no, a, b);
	mode |= rev_color_gen_a[a & 3] | rev_color_gen_b[b & 1];

	mat->get_layer_tex_wrap(no, a, b);
	if (a == 1 || b == 1)
		mode |= 0x08000000;

	mat->get_layer_tex_gen(no, a, b);
	switch (b) {
	case 0x1000:
		b = 0x00e00000;
		break;
	case 1:
		b = 0x00400000;
		break;
	case 2:
		b = 0x00d00000;
		break;
	default:
		b = 0;
		break;
	}
	a = (rev_gen_table[a & 3] | b) & ~0x08800000;
	mode |= a;

	/* NOTE: bug: rev_map_b is indexed with the first output, not the second. */
	mat->get_layer_map_coord(no, a, b);
	mode |= rev_map_a[a & 3] | rev_map_b[a & 1];

	return 1;
}

static int tex_func_table_r[4] = { 0, 1, 0, 0 };
static int tex_func_mode_r[2] = { 0, 0x10 };

int Model::GetTextureMode(gmo_material *mat, int no, int &mode) const
{
	if (!mat)
		return 0;
	int func, arg;
	mat->get_layer_tex_func(no, func, arg);
	mode = tex_func_table_r[func & 3] | tex_func_mode_r[arg & 1];
	return 1;
}

int Model::GetTextureClamp(gmo_material *mat, int no, int &mode) const
{
	if (!mat)
		return 0;
	int s, t;
	mat->get_layer_tex_wrap(no, s, t);
	mode = 0;
	if ((unsigned int)(s - 1) < 2)
		mode = 1;
	if ((unsigned int)(t - 1) < 2)
		mode |= 0x10;
	return 1;
}

static int tex_filter_mag_r[2] = { 0, 1 };
static int tex_filter_min_r[8] = { 0x200, 0x210, 0, 0x100, 0x10, 0x110, 0x110, 0x110 };

int Model::GetTextureFilter(gmo_material *mat, int no, int &mode) const
{
	if (!mat)
		return 0;
	int mag, min;
	mat->get_layer_tex_filter(no, mag, min);
	mode = tex_filter_mag_r[mag & 1] | tex_filter_min_r[min & 7];
	return 1;
}
