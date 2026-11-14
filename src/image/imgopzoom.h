/*
 * ImgOpResize: resizes an ImageRGBA to a target size (param of OperateImp is
 * an EiclSize).
 */

#ifndef IMGOPZOOM_H
#define IMGOPZOOM_H

class ImageRGBA;
class EiclSize;

enum RESIZE_METHOD {
	RESIZE_DEFAULT = 4
	/* other values unknown */
};

class ImgOpResize {
public:
	static int Operate(ImageRGBA *img, const EiclSize *sz, RESIZE_METHOD m);
	static int Operate(ImageRGBA *img, int w, int h, RESIZE_METHOD m);
	static int OperateImp(ImageRGBA *src, ImageRGBA *dst, const void *param);
};

#endif
