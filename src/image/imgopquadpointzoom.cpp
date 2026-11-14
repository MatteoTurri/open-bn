/*
 * ImgOpQuadPointZoom: bilinear (four-sample) scaling of an ImageRGBA.
 * OperateImp doubles or quarters a clone of the source until it is near the
 * target, then does the final step to the exact size.
 */

#include "imagergba.h"
#include "eiclsize.h"

#include <stdlib.h>
#include <math.h>

#include "imgopquadpointzoom.h"

int ImgOpQuadPointZoom::OperateImp(ImageRGBA *src, ImageRGBA *dst, const void *param)
{
	const EiclSize *req = (const EiclSize *)param;
	EiclSize sz(req->w, req->h);
	int result;
	ImageRGBA *clone = src->CreateClone();

	if (clone) {
		EiclSize cur;

		cur.w = src->Width();
		cur.h = src->Height();
		result = 0;
		if (clone->Width() < (unsigned)sz.GetWidth() && clone->Height() < (unsigned)sz.GetHeight()) {
			while (!result && clone->Width() * 2 < (unsigned)sz.GetWidth()
			    && clone->Height() * 2 < (unsigned)sz.GetHeight()) {
				cur.w *= 2;
				cur.h *= 2;
				result = QuadPointZoom(clone, dst, &cur);
				if (!result)
					clone->Duplicate(dst);
			}
		} else if (clone->Width() > (unsigned)sz.GetWidth() && clone->Height() > (unsigned)sz.GetHeight()) {
			while (!result && clone->Width() / 4 > (unsigned)sz.GetWidth()
			    && clone->Height() / 4 > (unsigned)sz.GetHeight()) {
				cur.w /= 4;
				cur.h /= 4;
				result = QuadPointZoom(clone, dst, &cur);
				if (!result)
					clone->Duplicate(dst);
			}
		}
		if (!result)
			result = QuadPointZoom(clone, dst, param);
		delete clone;
	} else {
		result = 16;
	}
	return result;
}

int ImgOpQuadPointZoom::QuadPointZoom(ImageRGBA *src, ImageRGBA *dst, const void *param)
{
	const EiclSize *req = (const EiclSize *)param;
	EiclSize sz(req->w, req->h);
	int xs[4];
	int ys[4];
	int bb[4];
	int gg[4];
	int rr[4];
	int aa[4];
	int wx16, wy16;
	unsigned x;
	int sb, sg, sr, sa;
	unsigned srcW, srcH, dstW, dstH;
	unsigned y;
	unsigned x0, y0;
	double fx, fy;
	unsigned i;
	int w;

	dst->Reconstruct(sz.GetWidth(), sz.GetHeight(), 255, ImageRGBA::RGBA8888);

	srcW = src->Width();
	srcH = src->Height();
	dstW = dst->Width();
	dstH = dst->Height();

	for (y = 0; y < dstH; y++) {
		fy = (double)(y * srcH) / (double)dstH;
		wy16 = (int)(fy * 16.0);
		y0 = (unsigned)floor(fy);
		ys[0] = ys[1] = y0;
		ys[2] = ys[3] = y0 + 1;

		for (x = 0; x < dstW; x++) {
			sb = sg = sr = sa = 0;
			fx = (double)(x * srcW) / (double)dstW;
			wx16 = (int)(fx * 16.0);
			x0 = (unsigned)floor(fx);
			xs[0] = xs[2] = x0;
			xs[1] = xs[3] = x0 + 1;

			for (i = 0; i < 4; i++) {
				src->GetPixel(xs[i] < srcW ? xs[i] : srcW - 1, ys[i] < srcH ? ys[i] : srcH - 1, rr[i],
				    gg[i], bb[i], aa[i]);
				w = (16 - abs(xs[i] * 16 - wx16)) * (16 - abs(ys[i] * 16 - wy16));
				sr += (rr[i] * w) >> 8;
				sg += (gg[i] * w) >> 8;
				sb += (bb[i] * w) >> 8;
				sa += (aa[i] * w) >> 8;
			}
			dst->SetPixel(x, y, sr & 0xff, sg & 0xff, sb & 0xff, sa & 0xff);
		}
	}
	return 0;
}
