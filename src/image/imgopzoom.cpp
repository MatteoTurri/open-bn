/*
 * ImgOpResize: resizes an ImageRGBA with ImgOpZoomIn (enlarge, bilinear) or
 * ImgOpZoomOut (reduce, box average).
 */

#include "imagergba.h"
#include "eiclerror.h"
#include "imgopzoomin.h"
#include "imgopzoomout.h"
#include "eiclsize.h"
#include "imgopzoom.h"

int ImgOpResize::Operate(ImageRGBA *img, const EiclSize *sz, RESIZE_METHOD m)
{
	ImageRGBA *tmp = img->CreateCompatible();
	int r;

	if (tmp != 0) {
		r = OperateImp(img, tmp, sz);
		if (r == 0)
			img->Duplicate(tmp);
		delete tmp;
	} else {
		r = EICL_ERR_NO_MEMORY;
	}
	return r;
}

int ImgOpResize::Operate(ImageRGBA *img, int w, int h, RESIZE_METHOD m)
{
	EiclSize sz(w, h);

	return Operate(img, &sz, RESIZE_DEFAULT);
}

/* param is the target size: smaller than src in both dimensions -> zoom
 * out, same size -> plain copy, otherwise zoom in. */
int ImgOpResize::OperateImp(ImageRGBA *src, ImageRGBA *dst, const void *param)
{
	const EiclSize *target = (const EiclSize *)param;
	EiclSize sz(target->w, target->h);

	if (sz.GetWidth() < src->Width()) {
		if (sz.GetHeight() < src->Height())
			return ImgOpZoomOut::OperateImp(src, dst, param);
	}
	if (sz.GetWidth() == src->Width()) {
		if (sz.GetHeight() == src->Height()) {
			dst->Duplicate(src);
			return 0;
		}
	}
	return ImgOpZoomIn::OperateImp(src, dst, param);
}
