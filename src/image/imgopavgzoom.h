/*
 * ImgOpAverageZoom: reduces an ImageRGBA by box averaging.
 */

#ifndef IMGOPAVGZOOM_H
#define IMGOPAVGZOOM_H

class ImageRGBA;
class EiclSize;

class ImgOpAverageZoom {
public:
	static int OperateImp(ImageRGBA *src, ImageRGBA *dst, const void *param);
	static int AverageZoom(ImageRGBA *src, ImageRGBA *dst, EiclSize *size);
};

#endif
