/*
 * Disp2d: scene object displayed in screen space.
 */

#ifndef DISP2D_H
#define DISP2D_H

#include "gfx/vec_types.h"
#include "object.h"
#include "camera.h"

class gm_graph;

/* Object aligned with the screen: AdjustCamera puts it on the virtual screen of a camera. */
class Disp2d : public Object {
public:
	Disp2d(Object *parent);
	virtual ~Disp2d();

	virtual void DrawThis(gm_graph &g);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const String &value);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, String &value) const;

	void AdjustCamera(Camera *camera);
};

#endif
