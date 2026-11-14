/*
 * ImgOpZoomIn: enlarges an ImageRGBA (bilinear, through ImgOpQuadPointZoom).
 */

#ifndef IMGOPZOOMIN_H
#define IMGOPZOOMIN_H

class ImageRGBA;

class ImgOpZoomIn {
public:
	static int OperateImp(ImageRGBA *src, ImageRGBA *dst, const void *param);
};

#endif
