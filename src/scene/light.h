/*
 * Light: light object of the scene graph.
 */

#ifndef LIGHT_H
#define LIGHT_H

#include "object.h"

/* A light of type 0 directional, 1 point or 2 spot, bound to the gm_graph light slot `no`. */
class Light : public Object {
public:
	/* 0x250 */ int no;  /* gm_graph light slot */
	/* 0x254 */ int type;  /* GM_LIGHT_* */
	/* 0x258 */ float range;  /* maximum distance */
	/* 0x25C */ float atten_const;  /* constant attenuation */
	/* 0x260 */ float atten_linear;  /* linear attenuation */
	/* 0x264 */ float atten_quad;  /* quadratic attenuation */
	/* 0x268 */ float cone_angle;
	/* 0x26C */ float cone_falloff;
	/* 0x270 */ float cone_scale;
	/* 0x274 */ float cone_bias;

	Light(Object *parent, int no);
	~Light();

	void ActivatePseudoPointLightState(gm_graph &g, int no, const vec4 &pos);
	int GetType() const;
	virtual void DrawThis(gm_graph &g);
	void ActivateLightState(gm_graph &g, int no);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const void *data, unsigned int size);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, void *data, unsigned int size) const;
	void SetType(int type);
	void SetRange(float maxDist, float constant, float linear, float quadratic);
	void SetCone(float angle, float falloff);
	void GetType(int &type) const;
	void GetRange(float &maxDist, float &constant, float &linear, float &quadratic) const;
	void GetCone(float &angle, float &falloff) const;
}; /* size = 0x278 */

#endif
