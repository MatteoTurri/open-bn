/*
 * GIF encoder: color reduction with _quant_opt and output through giflib.
 */

#include "outputdatafiltergif.h"
#include "eiclerror.h"
#include "imagergba.h"
#include "iofilter.h"
#include "quant_opt.h"
extern "C" {
#include "gif_lib.h"
/* Not declared by the gif_lib.h of giflib 4.1.0. */
GifFileType *EGifOpen(void *userPtr, OutputFunc writeFunc);
}

OutputDataFilterGIF::OutputDataFilterGIF() : OutputDataFilter()
{
	m_ioFilter = 0;
	PrivateGIFWriter *w = new PrivateGIFWriter;
	w->m_filter = this;
	m_writer = w;
	return;  /* redundant as written */
}

OutputDataFilterGIF::OutputDataFilterGIF(OutputIoFilter *ioFilter) : OutputDataFilter()
{
	m_ioFilter = ioFilter;
	PrivateGIFWriter *w = new PrivateGIFWriter;
	w->m_filter = this;
	m_writer = w;
	return;  /* redundant as written */
}

OutputDataFilterGIF::~OutputDataFilterGIF()
{
	delete m_writer;
}

inline int OutputDataFilterGIF::PrivateGIFWriter::write(GifFileType *gif, const unsigned char *buf, int len)
{
	OutputDataFilterGIF *self = (OutputDataFilterGIF *)gif->UserData;
	return self->m_ioFilter->Write(len, buf);
}

/* Reduces the image to 255 colors with _quant_opt, builds the palette with
   QuantizeBuffer and writes it through the EGif* calls, using
   PrivateGIFWriter::write as output function.
   Returns EICL_ERR_NO_MEMORY (out of memory), EICL_ERR_CODEC (QuantizeBuffer failed), EICL_ERR_STREAM_WRITE (giflib
   write error), 0 on success. */
int OutputDataFilterGIF::SetRawData(const ImageRGBA *img)
{
	int result;
	unsigned int size;
	GifByteType *red, *green, *blue;

	size = img->Width() * img->Height();
	red = (GifByteType *)malloc(size);
	green = (GifByteType *)malloc(size);
	blue = (GifByteType *)malloc(size);
	if (red != 0 && green != 0 && blue != 0) {
		GifByteType *rgb = (GifByteType *)malloc(img->Width() * img->Height() * 3);

		if (rgb != 0) {
			GifByteType *quant;

			img->GetImage(rgb, ImageRGBA::BGR888);
			quant = (GifByteType *)malloc(img->Width() * img->Height() * 3);
			if (quant != 0) {
				int colorMapSize;
				ColorMapObject *colorMap;
				GifByteType *out;
				GifByteType *p, *rp, *gp, *bp;
				unsigned int x, y;
				unsigned int row;

				_quant_opt(quant, rgb, 255, img->Width(), img->Height(), false);
				rp = red;
				p = quant;
				gp = green;
				bp = blue;
				for (y = 0; y < img->Height(); y++)
					for (x = 0; x < img->Width(); x++) {
						*bp++ = *p++;
						*gp++ = *p++;
						*rp++ = *p++;
					}
				colorMapSize = 256;
				colorMap = MakeMapObject(256, 0);
				out = (GifByteType *)malloc(img->Width() * img->Height());
				if (colorMap != 0 && out != 0) {
					if (QuantizeBuffer(img->Width(), img->Height(), &colorMapSize, red, green, blue,
					        out, colorMap->Colors)) {
						GifFileType *gif = EGifOpen(m_writer->m_filter,
						    OutputDataFilterGIF::PrivateGIFWriter::write);

						if (gif != 0
						    && EGifPutScreenDesc(gif, img->Width(), img->Height(), colorMapSize,
						           0, colorMap)
						        != 0
						    && EGifPutImageDesc(gif, 0, 0, img->Width(), img->Height(), 0, 0)
						        != 0) {
							GifByteType *line;

							result = 0;
							for (row = 0, line = out; row < img->Height(); row++) {
								if (EGifPutLine(gif, line, img->Width()) == 0) {
									result = EICL_ERR_STREAM_WRITE;
									break;
								}
								line += img->Width();
							}
							EGifCloseFile(gif);
						} else {
							result = EICL_ERR_STREAM_WRITE;
						}
					} else {
						result = EICL_ERR_CODEC;
					}
				} else {
					result = EICL_ERR_NO_MEMORY;
				}
				if (out)
					free(out);
				if (colorMap)
					FreeMapObject(colorMap);
				free(quant);
			} else
				result = EICL_ERR_NO_MEMORY;
			free(rgb);
		} else {
			result = EICL_ERR_NO_MEMORY;
		}
	} else {
		result = EICL_ERR_NO_MEMORY;
	}
	if (red)
		free(red);
	if (green)
		free(green);
	if (blue)
		free(blue);
	return result;
}
