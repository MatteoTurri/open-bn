/*
 * EiclSize: a width/height pair.
 */

#ifndef EICLSIZE_H
#define EICLSIZE_H

class EiclSize {
public:
	EiclSize()
	{
		w = 0;
		h = 0;
	}
	EiclSize(int w, int h);
	EiclSize(const EiclSize *other);

	unsigned Width() const { return (unsigned)w; }
	unsigned Height() const { return (unsigned)h; }
	int GetWidth() const;
	int GetHeight() const;
	void SetWidth(int w);
	void SetHeight(int h);

	void SetSize(int w, int h);
	void SetSize(EiclSize sz);
	void SetSize_(int w, int h);

	int FitTo(int w, int h);
	void FitTo(EiclSize sz);

	EiclSize operator+(EiclSize &other);

	/* 0x00 */ int w, h;
}; /* size = 0x08 */

#endif
