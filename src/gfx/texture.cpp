/*
 * Texture: thin wrapper over the tm texture manager, with one shared manager
 * and a budget for textures generated at run time from strings/fonts
 * (at most 256 alive, 2 MB in total).
 */

#include "texture.h"
#include "lib/mymalloc.h"

static TmHandle s_manager;    /* the shared manager */
static int s_usedMemory;      /* bytes used by string textures */
static int s_stringTexCount;  /* string textures alive */

void Texture::Init()
{
	TmInitInfo funcs;
	funcs.allocFn = _stdMalloc;
	funcs.freeFn = _stdFree;
	tmInitialize(&funcs);
	s_usedMemory = 0;
	s_stringTexCount = 0;
	s_manager = tmCreateManager(11568, 16384);
}

void Texture::Exit()
{
	tmRelease(s_manager);
	s_manager = 0;
}

void Texture::Exit2()
{
	tmShutdown();
}

TmHandle Texture::Manager()
{
	return s_manager;
}

/* Destroy callback of a registered string texture; arg is its size. */
static void DestroyStringTextureCallback(void *size)
{
	s_stringTexCount--;
	s_usedMemory -= (int)size;
}

/* Same, for a CLUT: counts against the memory budget only. */
static void DestroyClutMemCallback(void *size)
{
	s_usedMemory -= (int)size;
}

/* Creates a texture, plus its CLUT for the indexed types 19 and 20.
   With doRegister both count against the string texture budget: -1000 means
   too many textures, -1001 means the 2 MB budget is exceeded. */
int Texture::Create(int width, int height, int type, int unused, int mipLevels, void *data, void *clutData,
    bool swizzleClut, bool doRegister)
{
	int handle;
	int clut = 0;
	int err = 0;

	handle = tmCreateTexture(s_manager, width, height, type | ((mipLevels - 1) << 24));
	if (handle <= 0)
		goto texture_error;
	if (doRegister) {
		int size = tmGetTotalSize(handle);
		unsigned newCount = s_stringTexCount + 1;
		unsigned used;

		if (newCount >= 257) {
			err = -1000;
			goto out;
		}
		used = s_usedMemory + size;
		if (0x200000 < used) {
			err = -1001;
			goto out;
		}
		s_stringTexCount = newCount;
		s_usedMemory = used;
		tmSetDestroyCallback(handle, DestroyStringTextureCallback, (void *)size);
	}
	tmUpdateTexture(handle, width, height, type | ((mipLevels - 1) << 24), data);
	if ((unsigned)(type - 19) >= 2)
		goto out;
	clut = tmCreateClut(s_manager, type, 0);
	if (clut <= 0)
		goto clut_error;
	if (doRegister) {
		int size = tmGetTotalSize(clut);
		unsigned used = s_usedMemory + size;

		if (0x200000 < used) {
			err = -1001;
			goto out;
		}
		s_usedMemory = used;
		tmSetDestroyCallback(clut, DestroyClutMemCallback, (void *)size);
	}
	tmUpdateClut(clut, swizzleClut, clutData);
	tmSetClut(handle, clut);
	tmRelease(clut);
	goto out;
clut_error:
	err = clut;
	goto out;
texture_error:
	err = handle;
out:
	if (err < 0) {
		if (handle > 0) {
			tmRelease(handle);
			handle = 0;
		}
		if (clut > 0)
			tmRelease(clut);
		Texture::PrintTMError(err);
	}
	return handle;
}

/* Charges a string texture against the budget (256 textures, 2 MB) and
   releases it when the texture is destroyed. A no-op when doRegister is false. */
bool Texture::RegistStringTexture(int data, bool doRegister)
{
	if (doRegister) {
		unsigned int size = tmGetTotalSize(data);
		unsigned int newCount = s_stringTexCount + 1;
		if (newCount >= 257)
			return false;
		unsigned int total = s_usedMemory + size;
		if (total > 0x200000)
			return false;
		s_stringTexCount = newCount;
		s_usedMemory = total;
		tmSetDestroyCallback(data, DestroyStringTextureCallback, (void *)size);
	}
	return true;
}

void Texture::PrintTMError(int)
{}

void Texture::Release(int handle)
{
	tmRelease(handle);
}

/* Builds the 64-bit GS TEX0 register: TBP0 0-13, TBW 14-19, PSM 20-25,
   TW 26-29, TH 30-33, TCC 34, CBP 37-50, CLD 61 (always 1). TW/TH are
   ceil(log2()) of the size; TCC is set for every format except 1 and 2. */
void Texture::Use(int handle, unsigned long long *tex0)
{
	int format;
	int tw, th;
	int tcc;
	int base;
	int baseWidth;
	int clutBase;
	int w, h;
	int v;

	tmChainTexture(handle);
	format = tmGetPixelFormat(handle);
	tcc = 0;
	if (format != 1 && format != 2)
		tcc = 1;
	base = tmGetBase(handle);
	baseWidth = tmGetBaseWidth(handle);
	w = tmGetWidth(handle);
	v = 1;
	tw = 0;
	while (v < w) {
		v <<= 1;
		tw++;
	}
	h = tmGetHeight(handle);
	v = 1;
	th = 0;
	while (v < h) {
		v <<= 1;
		th++;
	}
	clutBase = tmGetClutBase(handle);
	*tex0 = (long long)base | ((long long)baseWidth << 14) | ((long long)format << 20) | ((long long)tw << 26)
	    | ((long long)th << 30) | ((long long)tcc << 34) | ((long long)clutBase << 37) | ((long long)1 << 61);
}

/* Byte size of all mip levels. Bytes per pixel: type 1 = 3, type 2 = 2,
   type 19 = 1 (8-bit indexed), type 20 = 1/2 (4-bit indexed), else 4. */
int Texture::CalcSizeofImage(int type, int width, int height, int levels)
{
	int level = 0;
	int total = 0;

	if (levels > 0) {
		do {
			switch (type) {
			case 1:
				total += width * height * 3;
				break;
			case 2:
				total += width * height * 2;
				break;
			case 19:
				total += width * height;
				break;
			case 20:
				total += (width * height) >> 1;
				break;
			case 0:
			default:
				total += width * height * 4;
				break;
			}
			width >>= 1;
			height >>= 1;
			level++;
		} while (level < levels);
	}
	return total;
}

/* Palette size in bytes: 256 entries for type 19, 16 for type 20, 4 bytes
   each; any other type gets 1024. */
int Texture::CalcSizeofClut(int type)
{
	switch (type) {
	case 19:
		return 1024;
	case 20:
		return 64;
	default:
		return 1024;
	}
}

void Texture::FlushGSCache()
{
	TmManagerInfo info;
	info.size = sizeof info;
	tmGetManagerInfo(s_manager, &info);
	info.size = sizeof info;
	tmSetManagerInfo(s_manager, &info);
}
