/*
 * Defocus: depth of field (screen blur) effect of the scene.
 */

#ifndef DEFOCUS_H
#define DEFOCUS_H

#include "gfx/vec_types.h"
#include "object.h"

class Object;
class gm_graph;

/* Blurs the screen with `level` ping-pong passes through an offscreen color buffer. */
class Defocus : public Object {
public:
	/* 0x250 */ int render_target_id;  /* offscreen color buffer, 320 x 224 */
	/* 0x254 */ int level;  /* option LEVL: number of blur passes, at most 16 */
	/* 0x258 */ int pad_258[2];
	/* 0x260 */ vec4 kernel[16];  /* color weight of each pass */

	Defocus(Object *parent);
	virtual ~Defocus();

	virtual void DrawThis(gm_graph &g);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const String &value);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, String &value) const;
};

#endif
