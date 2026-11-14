/*
 * ImgOpAverageZoom: box-average downscaling of an ImageRGBA. OperateImp is
 * static and takes (src, dst, param).
 */

#include "imagergba.h"
#include "eiclsize.h"
#include "imgopavgzoom.h"

class ImgOp {
public:
	static int OperateImp(ImageRGBA *src, ImageRGBA *dst, const void *param);
};

int ImgOpAverageZoom::OperateImp(ImageRGBA *src, ImageRGBA *dst, const void *param)
{
	const EiclSize *req = (const EiclSize *)param;
	EiclSize sz(req->w, req->h);
	int result;

	if (src->Width() >= (unsigned)sz.GetWidth() && src->Height() >= (unsigned)sz.GetHeight()) {
		result = AverageZoom(src, dst, &sz);
	} else {
		ImageRGBA *clone = src->CreateClone();
		ImageRGBA *compat = clone->CreateCompatible();
		int ret;

		if (compat) {
			ret = ImgOp::OperateImp(clone, compat, param);
			if (!ret)
				clone->Duplicate(compat);
			delete compat;
		} else {
			ret = 16;
		}
		result = ret;
		dst->Duplicate(clone);
		delete clone;
	}
	return result;
}

int ImgOpAverageZoom::AverageZoom(ImageRGBA *src, ImageRGBA *dst, EiclSize *size)
{
	int result;

	dst->Reconstruct(size->GetWidth(), size->GetHeight(), 255, ImageRGBA::RGBA8888);

	result = 0;

	unsigned srcW = src->Width();
	unsigned srcH = src->Height();
	unsigned dstW = size->GetWidth();
	unsigned dstH = size->GetHeight();
	unsigned char *base = (unsigned char *)src->GetImageBufferPtr();

	if (base) {
		unsigned stride = src->Width() * 4;
		unsigned y;

		for (y = 0; y < dstH; y++) {
			unsigned y0 = y * srcH / dstH;
			unsigned y1 = (y + 1) * srcH / dstH;
			unsigned x;

			for (x = 0; x < dstW; x++) {
				unsigned x0 = x * srcW / dstW;
				unsigned x1 = (x + 1) * srcW / dstW;
				unsigned char *p = base + stride * y0 + x0 * 4;
				unsigned colBytes = (x1 - x0) * 4;
				unsigned rowAdvance = stride - colBytes;
				unsigned sum[4];
				unsigned row;
				unsigned n;
				unsigned r, g, b, a;

				sum[3] = 0;
				sum[2] = 0;
				sum[1] = 0;
				sum[0] = 0;
				for (row = y0; row < y1; row++) {
					unsigned char *end = p + colBytes;
					while (p < end) {
						sum[0] += *p++;
						sum[1] += *p++;
						sum[2] += *p++;
						sum[3] += *p++;
					}
					p += rowAdvance;
				}
				n = (y1 - y0) * (x1 - x0);
				r = sum[0] / n;
				g = sum[1] / n;
				b = sum[2] / n;
				a = sum[3] / n;
				dst->SetPixel(x, y, r, g, b, a);
			}
		}
	} else {
		result = 0x100000;
	}
	return result;
}

/* Base implementation: does nothing and returns the "unsupported" code. */
inline int ImgOp::OperateImp(ImageRGBA *src, ImageRGBA *dst, const void *param)
{
	return 0x100000;
}
