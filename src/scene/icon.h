/*
 * Icon: Object that draws a PS2 save-data icon (icon.sys plus model, motion
 * and texture), with optional shadow pass and an icon cache shared by name.
 */

#ifndef ICON_H
#define ICON_H

#include "gfx/vec_types.h"
#include <vector>
#include "object.h"

class Object;
class gm_graph;

/* ===== save-icon file loaders ===== */

#define ICO_PS1_MAGIC 0x4353  /* "SC": a PS1 save file header */
#define ICO_SYS_MAGIC 0x44325350  /* "PS2D": icon.sys */
#define ICO_FILE_MAGIC 0x10000  /* first word of an icon file */

/* Parts of an ico_file (load, unload and is_loaded masks). */
#define ICO_PART_MODEL 1
#define ICO_PART_MOTION 2
#define ICO_PART_TEXTURE 4
#define ICO_PART_ALL 7

/* Bits of ico_model::attrib. */
#define ICO_ATTR_TEXTURE 4  /* a texture follows the motion */
#define ICO_ATTR_RLE 8  /* the texture is run-length encoded */
#define ICO_ATTR_PACKED 0x10  /* packed vertices */

class ico_texture {
public:
	int size;
	void *buffer;

	ico_texture();
	~ico_texture();

	ico_texture *copy(const ico_texture &other);
	int *load(int *p, int *end, int mask);
	int *decode_runlength(short *dst, int *src);
};

class ico_model {
public:
	/* 0x00 */ int attrib;       /* ICO_ATTR_* */
	/* 0x04 */ float unk_04;
	/* 0x08 */ int count;        /* morph shapes, 1..8 */
	/* 0x0C */ int vertex_count;
	/* 0x10 */ void *buffer;   /* float vertices of shape 0, then the other shapes */

	ico_model();
	~ico_model();

	ico_model *copy(const ico_model &other);
	int *load(int *p, int *end);
};

class ico_motion {
public:
	struct Entry {
		int shape;  /* weight index, < 0 = unused */
		int key_count;
		void *keys;  /* key_count (time, weight) pairs */
	};

	/* 0x00 */ int tag;
	/* 0x04 */ int frame_length;
	/* 0x08 */ float speed;
	/* 0x0C */ int play_offset;
	/* 0x10 */ int valid_count;
	/* 0x14 */ Entry entries[8];

	ico_motion();
	~ico_motion();

	ico_motion *copy(const ico_motion &other);
	int *load(int *p, int *end);
	void get_weights(float *out, int shapeCount, float t) const;
};

class ico_file {
public:
	/* 0x00 */ int unk_00;
	/* 0x04 */ ico_model *model;
	/* 0x08 */ ico_motion *motion;
	/* 0x0C */ ico_texture *texture;

	ico_file();
	~ico_file();

	ico_file *copy(const ico_file &other);
	int *load(int *p, int *end, int mask);
	void unload(int mask);
	bool is_loaded(int mask) const;
};

/* Parsed icon.sys (964-byte record, see ico_conf::load). */
class ico_conf {
public:
	/* 0x00 */ int magic;
	/* 0x04 */ String title;
	/* 0x08 */ int title_break;      /* line-break offset in the title */
	/* 0x0C */ String icon_names[3]; /* icon file names (list, copy, delete) */
	/* 0x18 */ char pad_18[8];
	/* 0x20 */ vec4 light_dir[3];
	/* 0x50 */ vec4 light_color[3];
	/* 0x80 */ vec4 ambient;
	/* 0x90 */ int color[4];  /* background corner colors, packed RGBA */

	ico_conf();
	~ico_conf();

	ico_conf *copy(const ico_conf &other);
	int *load(int *p, int *end);
	void unload();
	bool is_loaded() const;
};

/* Bits of Icon::flags. */
#define ICON_REBUILD 0x2  /* RebuildRun pending */
#define ICON_RELOAD 0x4  /* ReloadRun pending */
#define ICON_BUILT 0x100  /* texture and display list ready */
#define ICON_LOADED 0x200
#define ICON_PS1 0x400  /* a PS1 save icon ("SC" header), drawn on the default shape */
#define ICON_TRANSLUCENT 0x800  /* fading in or out */
#define ICON_HIDDEN 0x1000  /* invisible or fully faded */
#define ICON_MODEL 0x10000
#define ICON_MORPH 0x20000  /* animated: more than one shape */
#define ICON_TEXTURED 0x40000
#define ICON_SYNC 0x01000000  /* option 'SYNC': load and build when 'FILE' is set */
#define ICON_AMBIENT 0x02000000  /* option 'AMBS': the ambient of icon.sys */
#define ICON_LIGHTS 0x04000000  /* option 'LITS': the lights of icon.sys */
#define ICON_SHADOW 0x08000000  /* option 'FBUF': shadow pass while translucent */
#define ICON_SHADOW_ONCE 0x10000000  /* 'FBUF' >= 2: the shadow ends when the icon is opaque */
#define ICON_PERSPECTIVE 0x20000000  /* option 'PERS' != 1 */

/* Values of the option 'TYPE'. */
#define ICON_TYPE_NONE 0
#define ICON_TYPE_PS1 1
#define ICON_TYPE_PS2 2

/* States of Icon::ReloadRun. */
enum {
	ICON_LOAD_START = 0,  /* from the cache or open the file */
	ICON_LOAD_READ = 1,  /* read the whole file, then tell the format */
	ICON_LOAD_PS1 = 10,  /* PS1 header: frames, CLUT and pixels */
	ICON_LOAD_PS1_DONE = 11,
	ICON_LOAD_SYS = 20,  /* PS2: icon.sys */
	ICON_LOAD_MODEL = 21,
	ICON_LOAD_MOTION = 22,
	ICON_LOAD_TEXTURE = 23,
	ICON_LOAD_DONE = 24
};

/* States of Icon::RebuildRun. */
enum {
	ICON_BUILD_TEXTURE = 0,
	ICON_BUILD_LIST = 1
};

class Icon : public Object {
public:
	/* 0x250 */ Icon *next;            /* in the global list */
	/* 0x254 */ Icon *prev;
	/* 0x258 */ int flags;            /* ICON_* */
	/* 0x25C */ int display_list;
	/* 0x260 */ int shadow_buffer;     /* color buffer, created on demand by "FBUF" */
	/* 0x264 */ int texture;
	/* 0x268 */ int pad_268[2];
	/* 0x270 */ ico_conf conf;
	/* 0x310 */ ico_file file;
	/* 0x320 */ String file_name;      /* also the cache key */
	/* 0x324 */ int reload_state;      /* ICON_LOAD_* */
	/* 0x328 */ int rebuild_state;     /* ICON_BUILD_* */
	/* 0x32C */ int fd;               /* -1 when closed */
	/* 0x330 */ int *load_buf;
	/* 0x334 */ int bytes_read;
	/* 0x338 */ int bytes_expected;
	/* 0x33C */ int *parse_cur;
	/* 0x340 */ int *parse_end;
	/* 0x344 */ int shading;          /* bit 0 selects flat or gouraud shading */
	/* 0x348 */ void *vertices;       /* model vertex buffer (DrawShape) */
	/* 0x34C */ int vertex_count;
	/* 0x350 */ int num_shapes;        /* morph shapes of the model */
	/* 0x354 */ float anim_length;     /* frames */
	/* 0x358 */ float anim_step;       /* frames per tick */
	/* 0x35C */ float frame;          /* option "FRAM" */
	/* 0x360 */ float anim_speed;      /* option "SPED", multiplies anim_step */
	/* 0x364 */ int pad_364[3];
	/* 0x370 */ fvec4 basis_a[4];  /* identity at construction */
	/* 0x3B0 */ int shape_frames;  /* frames of the shape texture (0..3) */
	/* Shape texture built in RAM: 0x180 bytes of pixels plus a 32-byte CLUT,
	   copied as whole structs from a cached twin. */
	struct shape_data {
		int b[0x60];
	};
	struct clut_data {
		int w[8];
	};
	/* 0x3B4 */ shape_data shape_tex;
	/* 0x534 */ clut_data shape_clut;
	/* 0x554 */ char pad_554[12];
	/* 0x560 */ fvec4 basis_b[4];  /* identity at construction */
	/* 0x5A0 */ float perspective;  /* option "PERS" */

	Icon(Object *parent);
	virtual ~Icon();

	virtual void DrawThis(gm_graph &g);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const String &value);
	virtual int SetOption(int option, const void *data, unsigned int size);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, void *data, unsigned int size) const;

	void DrawShape(gm_graph &g);
	int RebuildRun(gm_graph &g);
	void RebuildSet();
	int ReloadRun(gm_graph &g);
	void ReloadSet(const String &name);
	void Unbuild();
	void Unload();
	Icon *FindCache();
	void Animate(gm_graph &g);

	static Icon *g_top;
};

#endif
