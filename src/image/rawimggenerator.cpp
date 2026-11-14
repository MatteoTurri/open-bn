/*
 * RawImgGenerator: factory functions for the RawImg subclasses (type 1 =
 * RawImgBGR, type 2 = RawImgDIB). Generate returns NULL for other types.
 */

#include "rawimgdib.h"
#include "rawimggenerator.h"

RawImg *RawImgGenerator::GenerateCompatible(RawImg *src)
{
	return Generate((RawImg::RawImgType)src->ImgType());
}

RawImg *RawImgGenerator::Generate(RawImg::RawImgType type)
{
	RawImg *result = 0;

	if (type == RawImg::TYPE_BGR)
		result = new RawImgBGR();
	else if (type == RawImg::TYPE_DIB)
		result = new RawImgDIB();
	return result;
}

RawImg *RawImgGenerator::GenerateSizedImage(RawImg::RawImgType type, int w, int h)
{
	RawImg *img = Generate(type);

	if (img) {
		if (img->CreateNullImage(w, h)) {
			delete img;
			img = 0;
		}
	}
	return img;
}

RawImg *RawImgGenerator::GenerateClone(RawImg *src)
{
	RawImg *img = GenerateCompatible(src);

	if (img) {
		if (img->CreateAsClone(src)) {
			delete img;
			img = 0;
		}
	}
	return img;
}
