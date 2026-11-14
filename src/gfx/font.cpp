/*
 * Font: the two system fonts of the PLTM text library (opened at Init and
 * kept in s_fonts) and the kinsoku line-breaking callbacks passed to it.
 */

#include <stdlib.h>
#include "tm.h"
#include "fw/fw_pk.h"
#include "lib/mymalloc2.h"

#include "pltm.h"
#include "font.h"

static void *s_fonts[2];

/* Kinsoku (Japanese line-breaking) tables of UCS-2 characters: line ends,
   wide and ASCII opening brackets, wide and ASCII characters that may not
   start a line. */
static unsigned short s_kinsokuNl[2] = { 0x000A, 0x000D };

/* clang-format off */
static unsigned short s_kinsokuOpenW[12] = {
	0xFF40, 0x2018, 0x201C, 0xFF08, 0x3014, 0xFF3B,
	0xFF5B, 0x3008, 0x300A, 0x300C, 0x300E, 0x3010,
};

static unsigned short s_kinsokuCloseW[32] = {
	0x3000, 0x3001, 0x3002, 0xFF0C, 0xFF0E, 0x30FB, 0xFF1A, 0xFF1B,
	0xFF1F, 0xFF01, 0x309B, 0x309C, 0x00B4, 0x30FD, 0x30FE, 0x309D,
	0x309E, 0x3003, 0x3005, 0x2026, 0x2025, 0x2019, 0x201D, 0xFF09,
	0x3015, 0xFF3D, 0xFF5D, 0x3009, 0x300B, 0x300D, 0x300F, 0x3011,
};
/* clang-format on */

static unsigned short s_kinsokuOpenA[5] = { 0x0028, 0x007B, 0x005B, 0x0024, 0x005C };

/* clang-format off */
static unsigned short s_kinsokuCloseA[28] = {
	0x0029, 0x007D, 0x005D, 0x0021, 0x0040, 0x0023, 0x0025,
	0x005E, 0x0026, 0x002A, 0x002D, 0x003D, 0x005F, 0x002B,
	0x003B, 0x0027, 0x003A, 0x0022, 0x007E, 0x002C, 0x002E,
	0x002F, 0x003C, 0x003E, 0x003F, 0x0009, 0x000A, 0x0020,
};
/* clang-format on */

/* Is `c` a line end? */
static int font_is_line_end(unsigned short c)
{
	unsigned int i;

	for (i = 0; i < 2; i++)
		if (s_kinsokuNl[i] == c)
			return 1;
	return 0;
}

/* May `c` start a line? Below 256 only the ASCII opening brackets do; a wide
   character may unless it is in the closing table. */
static int font_can_start_line(unsigned short c)
{
	unsigned int i;

	for (i = 0; i < 5; i++)
		if (s_kinsokuOpenA[i] == c)
			return 1;
	if (c >= 256) {
		for (i = 0; i < 12; i++)
			if (s_kinsokuOpenW[i] == c)
				return 1;
		for (i = 0; i < 32; i++)
			if (s_kinsokuCloseW[i] == c)
				return 0;
		return 1;
	}
	return 0;
}

/* Must `c` not start a line? */
static int font_cannot_start_line(unsigned short c)
{
	unsigned int i;

	for (i = 0; i < 28; i++)
		if (s_kinsokuCloseA[i] == c)
			return 1;
	if (c >= 256) {
		for (i = 0; i < 32; i++)
			if (s_kinsokuCloseW[i] == c)
				return 1;
		return 0;
	}
	return 0;
}

void Font::Init()
{
	/* The PLTM_Init configuration, then the same memory as the callback table. */
	union {
		struct PLTM_InitInfo init;
		struct PLTM_CallbackBlock cb;
	} config;

	config.init.alloc = (void *(*)(int))malloc;
	config.init.free = free;
	config.init.poolAlloc = _stdMalloc2;
	config.init.poolFree = _stdFree2;
	config.init.sendPacket = (void (*)(void *, int))fwPkAddDmaChain_withRef;
	config.init.tmFrontSize = 11200;
	config.init.tmBackSize = 11568;
	PLTM_Init(&config.init);

	PLTM_SetPolygonPoolSize(400);
	s_fonts[0] = PLTM_OpenS("SGR_SCE.PST", "MBDB_SE1.PST");
	PLTM_SetPolygonPoolSize(0);
	s_fonts[1] = PLTM_OpenS(0, "SGR_SE1.PST");

	config.cb.canStartLine = font_can_start_line;
	config.cb.cannotStartLine = font_cannot_start_line;
	config.cb.isLineEnd = font_is_line_end;
	config.cb.unk_0C[0] = 0;
	config.cb.unk_0C[1] = 0;
	config.cb.unk_0C[2] = 0;
	config.cb.unk_0C[3] = 0;
	config.cb.unk_0C[4] = 0;

	void **p = s_fonts;
	int i = 1;
	do {
		PLTM_SetCallbacks(*p, &config.cb);
		i--;
		p++;
	} while (i >= 0);

	PLTM_SetPolygonPoolSize(256);
}

void Font::Exit()
{
	void **p = s_fonts;
	int i = 1;
	do {
		void *font = *p++;
		i--;
		PLTM_Close(font);
	} while (i >= 0);
	PLTM_Terminate();
}

void Font::Run()
{
	void **p = s_fonts;
	int i = 1;
	do {
		void *font = *p++;
		i--;
		PLTM_UpdateTimeStamp(font);
	} while (i >= 0);
}

void *Font::Get(int index)
{
	return s_fonts[index];
}

void Font::ExpireAll()
{
	/* NOTE: bug: passes the array of fonts, not a font */
	PLTM_ExpireAll(s_fonts);
}

void Font::FlushGSCache()
{
	TmHandle mgr = PLTM_tmID();
	TmManagerInfo info;
	info.size = sizeof info;
	tmGetManagerInfo(mgr, &info);
	info.size = sizeof info;
	tmSetManagerInfo(mgr, &info);
}
