/*
 * ImageRGBA: a bitmap stored either as RGBA8888 (4 bytes per pixel, R first)
 * or as INDEX8 with a 256-entry RGBA palette. The other pixel types are only
 * exchange formats for SetImage/GetImage/SetLine/GetLine.
 */

#ifndef IMAGERGBA_H
#define IMAGERGBA_H

class ImageRGBA {
public:
	enum pixel_type {
		PT_NONE = 0,
		RGBA8888 = 1,
		ABGR8888 = 2,
		RGB888 = 3,
		BGR888 = 4,
		ARGB1555 = 5,
		RGB565V = 6,
		GRAY8 = 7,
		MASKED8 = 8,
		INDEX8 = 9
	};

	ImageRGBA(unsigned int width, unsigned int height, unsigned char alpha, pixel_type type);
	virtual ~ImageRGBA();

	/* SetLine/GetLine return the source/destination pointer advanced past the
		   row; SetImage/GetImage chain them row after row. */
	void Construct(unsigned int width, unsigned int height, unsigned char alpha, pixel_type type);
	void Reconstruct(unsigned int width, unsigned int height, unsigned char alpha, pixel_type type);

	ImageRGBA *CreateCompatible() const;
	ImageRGBA *CreateClone() const;
	void Duplicate(const ImageRGBA *other);

	unsigned char *Coord2Address(int x, int y) const;

	void SetPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
	void SetPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);
	void GetPixel(int x, int y, int &r, int &g, int &b, int &a) const;
	void GetPixel(int x, int y, int &r, int &g, int &b) const;

	void *SetImage(const void *image, pixel_type type, const void *clut);
	void *GetImage(void *buf, pixel_type type) const;
	void *SetLine(int row, const void *data, pixel_type type, const void *clut);
	void *GetLine(int row, void *buf, pixel_type type) const;
	void *GetClut(void *buf) const;

	void SetNativePixel(int x, int y, unsigned int value);
	unsigned int GetNativePixel(int x, int y) const;

	void FillAlpha(unsigned char alpha);

	unsigned int Width() const { return width; }
	unsigned int Height() const { return height; }
	int IsClut() { return clut != 0; }
	void *GetImageBufferPtr() const { return image; }
	void *GetClutBufferPtr() const { return clut; }

private:
	/* 0x00 */ unsigned int width;
	/* 0x04 */ unsigned int height;
	/* 0x08 */ unsigned char *image;  /* 1 byte per pixel for INDEX8, else 4 */
	/* 0x0C */ unsigned char *clut;  /* 256 RGBA entries, INDEX8 only */
	/* 0x10 */ pixel_type type;
	/* 0x14 */ unsigned char alpha;  /* used when the source has no alpha */
	/* 0x18: vptr */
}; /* size = 0x1C */

#endif
