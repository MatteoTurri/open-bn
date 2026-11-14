/*
 * Plane: Object drawing a list of textured quads (flat, gouraud, edge-faded
 * or 9-slice), recorded in display lists of up to 16 items.
 */

#ifndef PLANE_H
#define PLANE_H

#include <vector>
#include "gfx/vec_types.h"
#include "object.h"

class Object;
class gm_graph;

class gm_graph;

/* Plane::flags. */
#define PLANE_LISTS_DIRTY   0x00000001  /* rebuild the dirty groups of items */
#define PLANE_REBUILD_ALL   0x00000002  /* rebuild every group */
#define PLANE_TEX_NONPOW2   0x00000100  /* the texture is not a power of two */
#define PLANE_GOURAUD       0x00000200  /* gouraud quads, corner colors 'COL4' */
#define PLANE_EDGE_FADE     0x00000400  /* edge-faded quads, option 'FDEG' */
#define PLANE_MARGINS       0x00000800  /* 9-slice quads, options 'MGN'/'MGNT' */
#define PLANE_STYLE_MASK    0x00000e00
#define PLANE_AUTO_CROP     0x00010000  /* auto-crop state of the last DrawThis */
#define PLANE_DEBUG_TEXTURE 0x80000000  /* option 'DBG': show the debug texture */

/* Plane::item_info::flags, besides ITEM_*. */
#define PLANE_ITEM_QUAD    0x0100  /* own quad, option 'CLIP' */
#define PLANE_ITEM_CROP    0x0200  /* own texture crop */
#define PLANE_ITEM_DIFFUSE 0x4000  /* own diffuse color */

/* Items per display list group (one dirty bit each). */
#define PLANE_GROUP_SIZE 16

class Plane : public Object {
public:
	class item_info {
	public:
		/* 0x00 */ int flags;  /* ITEM_*, PLANE_ITEM_* */
		/* 0x04 */ int display_list;
		/* 0x08 */ int list_count;    /* number of items drawn by this display list */
		/* 0x0C */ int disp;  /* option 'DISP', DISP_* */
		/* 0x10 */ int blend;  /* blend function, option 'ALPH' */
		/* 0x14 */ int pad_14[3];
		/* 0x20 */ vec4 diffuse;  /* diffuse color ('DIFF', 'COLR') */
		/* 0x30 */ vec4 quad;  /* quad center x, y and size z, w ('CLIP') */
		/* 0x40 */ vec4 crop;  /* texture crop ('CROP') */
		/* 0x50 */ vec4 pos;  /* position ('POS') */
		/* 0x60 */ vec4 rot;  /* rotation ('ROT') */
		/* 0x70 */ vec4 scale;  /* scale ('SCAL') */

		item_info();
		item_info &operator=(const item_info &o);
	};

	/* 0x250 */ int flags;  /* PLANE_* */
	/* 0x254 */ int tex_handle;
	/* 0x258 */ int pad_258[2];
	/* 0x260 */ vec4 def_diffuse;  /* default diffuse color */
	/* 0x270 */ vec4 def_crop;  /* default texture crop */
	/* 0x280 */ vector<item_info> items;
	/* 0x28C: one dirty bit per PLANE_GROUP_SIZE items */
	std::vector<bool> dirty;
	/* 0x2A8 */ int corner_colors[4];  /* corner colors for the gouraud quad, option 'COL4' */
	/* 0x2B8 */ float margins[4];  /* 9-slice margins (left, right, top, bottom), option 'MGN' */
	/* 0x2C8 */ float margins_uv[4];  /* 9-slice margins in uv, option 'MGNT' */

	Plane(Object *parent);
	virtual ~Plane();

	virtual void DrawThis(gm_graph &g);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const String &value);
	virtual int SetOption(int option, const void *data, unsigned int size);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, String &value) const;
	virtual int GetOption(int option, void *data, unsigned int size) const;
	virtual int SetItemOption(int option, int item, const int &value, unsigned int size);
	virtual int SetItemOption(int option, int item, const float &value, unsigned int size);
	virtual int SetItemOption(int option, int item, const void *data, unsigned int size);
	virtual int GetItemOption(int option, int item, int &value, unsigned int size) const;
	virtual int GetItemOption(int option, int item, float &value, unsigned int size) const;
	virtual int GetItemOption(int option, int item, void *data, unsigned int size) const;
	virtual void SetTexture(int texture, int option, int mode);

	void BuildShape(gm_graph &g, int start, int count);
	void DrawItem(gm_graph &g, item_info &item);
	void DrawItemShape(gm_graph &g, item_info &item);
	void DrawFlatQuad(gm_graph &g, item_info &item);
	void DrawGouraudQuad(gm_graph &g, item_info &item);
	void DrawEdgeFadedQuad(gm_graph &g, item_info &item);
	void DrawMarginedQuad(gm_graph &g, item_info &item);
	void UpdateDebugTexture(gm_graph &g);
};

#endif
