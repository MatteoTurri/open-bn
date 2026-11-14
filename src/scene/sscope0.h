/*
 * SScope0: scene object drawing a strip of vertex pairs driven by sample data,
 * over a trail.
 */

#ifndef SSCOPE0_H
#define SSCOPE0_H

#include "gfx/vec_types.h"
#include "gfx/gm_vertex.h"
#include "object.h"

class Object;
class gm_graph;

/* SScope0::flags. */
#define SSCOPE0_TEX_CHANGED 0x001  /* set by SetTexture, never read */
#define SSCOPE0_TEX_NONPOW2 0x100  /* the texture is not a power of two */

class SScope0 : public Object {
public:
	/* 0x250 */ gmVertexPNCT buffer_a[128];  /* vertex format 23; trail, one frame behind buffer_b */
	/* 0x1450 */ gmVertexPNCT buffer_b[128];  /* current strip */
	/* 0x2650 */ int flags;  /* SSCOPE0_* */
	/* 0x2654 */ int tex_handle;
	/* 0x2658 */ int samples;  /* sample buffer, 64 signed bytes (OPT_A) */
	/* 0x265C */ int samples_enabled;  /* samples enabled (OPT_B) */

	SScope0(Object *parent);
	virtual ~SScope0();

	virtual void DrawThis(gm_graph &g);
	virtual void SetTexture(int texture, int option, int mode);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const void *data, unsigned int size);
	virtual int GetOption(int option, int &value, unsigned int size) const;

	/* Not in Object: appended to the vtable. */
	virtual void SetColor(float r, float g, float b, float a);

	void MakeGraphics();
	void InitModel();

	class ClassInitializer {
	public:
		ClassInitializer();
	};
};

#endif
