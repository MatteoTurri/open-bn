/*
 * Model: Object that draws a GMO model (ModelData) with up to four items,
 * each with its own motion, referenced Object and material overrides.
 */

#ifndef MODEL_H
#define MODEL_H

#include "gfx/gmo.h"
#include "object.h"

/* A ModelData holds a gmo_model at offset 0x20. */
class ModelData;

/* Model::flags. */
#define MODEL_LAYER0_WRAP  0x0100  /* the wrap of layer n (MODEL_LAYER0_WRAP << n) was set with 'TXC0'..'TXC3' */
#define MODEL_LAYER_WRAPS  0x0f00
#define MODEL_ITEM0_OBJECT 0x1000  /* item n has an Object (MODEL_ITEM0_OBJECT << n), bound as its mapper */
#define MODEL_ITEM_OBJECTS 0xf000

class Model : public Object {
public:
	/* 0x250 */ int flags;  /* MODEL_* */
	/* 0x254 */ int item_ref[4];  /* per-item Object reference, -1 for none */
	/* 0x264: per-item motion state, read as int or float. */
	union {
		int item_state[4];
		float item_statef[4];
	};
	/* 0x274 */ float item_angle[4];  /* per-item motion frame rate, 60 at construction */
	/* 0x284 */ ModelData *model_inst;  /* private copy of the model */
	/* 0x288 */ ModelData *model_data;  /* shared, reference-counted model */
	/* 0x28C */ int face_mode;  /* option 'FACE' */
	/* 0x290 */ int fade_mode;  /* option 'FADE' */
	/* 0x294 */ int anti_mode;  /* option 'ANTI' */
	/* 0x298 */ float particle_size;  /* option 'SIZE' */
	/* 0x29C */ float div_level;      /* option 'DIVL' */

	Model(Object *parent);
	~Model();

	virtual void DrawThis(gm_graph &g);
	void SetModel(ModelData *data);
	void SetObject(Object *obj, int option, int mode);
	void SetTexture(int texture, int option, int mode);

	int SetLayerMode(gmo_material *mat, int no, int mode);
	int GetLayerMode(gmo_material *mat, int no, int &mode) const;
	int SetTextureMode(gmo_material *mat, int no, int mode);
	int SetTextureClamp(gmo_material *mat, int no, int mode);
	int SetTextureFilter(gmo_material *mat, int no, int mode);
	int GetTextureMode(gmo_material *mat, int no, int &mode) const;
	int GetTextureClamp(gmo_material *mat, int no, int &mode) const;
	int GetTextureFilter(gmo_material *mat, int no, int &mode) const;

	int SetOption(int option, const int &value, unsigned int size);
	int SetOption(int option, const float &value, unsigned int size);
	int SetOption(int option, const void *data, unsigned int size);
	int GetOption(int option, int &value, unsigned int size) const;
	int GetOption(int option, float &value, unsigned int size) const;
	int GetOption(int option, void *data, unsigned int size) const;
	int SetItemOption(int option, int item, const int &value, unsigned int size);
	int SetItemOption(int option, int item, const float &value, unsigned int size);
	int SetItemOption(int option, int item, const void *data, unsigned int size);
	int GetItemOption(int option, int item, int &value, unsigned int size) const;
	int GetItemOption(int option, int item, float &value, unsigned int size) const;
	int GetItemOption(int option, int item, void *data, unsigned int size) const;
};

#endif
