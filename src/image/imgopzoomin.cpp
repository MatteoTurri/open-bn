/*
 * ImgOpZoomIn: enlarging through ImgOpQuadPointZoom.
 */

#include "imagergba.h"
#include "imgopquadpointzoom.h"
#include "imgopzoomin.h"

int ImgOpZoomIn::OperateImp(ImageRGBA *src, ImageRGBA *dst, const void *param)
{
	return ImgOpQuadPointZoom::OperateImp(src, dst, param);
}
