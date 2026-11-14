/*
 * ImgOpQuadPointZoom: scales an ImageRGBA by bilinear (four-sample)
 * interpolation.
 */

#ifndef IMGOPQUADPOINTZOOM_H
#define IMGOPQUADPOINTZOOM_H

class ImageRGBA;

class ImgOpQuadPointZoom {
public:
	static int OperateImp(ImageRGBA *src, ImageRGBA *dst, const void *param);
	static int QuadPointZoom(ImageRGBA *src, ImageRGBA *dst, const void *param);
};

#endif
