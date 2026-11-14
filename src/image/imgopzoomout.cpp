/*
 * ImgOpZoomOut: reducing through ImgOpAverageZoom.
 */

#include "imagergba.h"
#include "imgopavgzoom.h"
#include "imgopzoomout.h"

int ImgOpZoomOut::OperateImp(ImageRGBA *src, ImageRGBA *dst, const void *param)
{
	return ImgOpAverageZoom::OperateImp(src, dst, param);
}
