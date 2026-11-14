/*
 * ImgOpZoomOut: reduces an ImageRGBA (box average, through ImgOpAverageZoom).
 */

#ifndef IMGOPZOOMOUT_H
#define IMGOPZOOMOUT_H

class ImageRGBA;

class ImgOpZoomOut {
public:
	static int OperateImp(ImageRGBA *src, ImageRGBA *dst, const void *param);
};

#endif
