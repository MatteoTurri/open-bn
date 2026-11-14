/*
 * GIF decoder (InputDataFilterGIF) on top of giflib: decodes the first frame
 * into an ImageRGBA or into an indexed buffer plus RGBA palette.
 */

#include "inputdatafilter.h"
#include "eiclerror.h"
#include "pgprops.h"
#include "imagergba.h"
#include <stdlib.h>
#include <string.h>
extern "C" {
#include "gif_lib.h"
}

#include "iofilter.h"
#include "inputdatafiltergif.h"
#include "theimagedecoder.h"

class InputDataFilterGIF::PrivateGIFReader {
public:
	InputDataFilterGIF *owner;

	PrivateGIFReader(InputDataFilterGIF *o) : owner(o) {}

	static int read(GifFileType *gif, unsigned char *buf, int len)
	{
		InputDataFilterGIF *owner = (InputDataFilterGIF *)gif->UserData;
		return owner->m_ioFilter->Read(len, buf);
	}
};

/* Interlaced GIF passes: start row and row step of each of the four passes. */
static int InterlacedOffset[4] = { 0, 4, 2, 1 };
static int InterlacedJumps[4] = { 8, 8, 4, 2 };

InputDataFilterGIF::InputDataFilterGIF() : InputDataFilter()
{
	m_ioFilter = 0;
	m_reader = new PrivateGIFReader(this);
}

InputDataFilterGIF::InputDataFilterGIF(InputIoFilter *ioFilter) : InputDataFilter()
{
	m_ioFilter = ioFilter;
	m_reader = new PrivateGIFReader(this);
}

InputDataFilterGIF::~InputDataFilterGIF()
{
	delete m_reader;
}

int InputDataFilterGIF::IsSupported()
{
	int ok;
	if (m_ioFilter) {
		PrivateGIFReader *reader = m_reader;
		reader->owner->m_ioFilter->Seek(0);
		GifFileType *gif = DGifOpen(reader->owner, PrivateGIFReader::read);
		if (gif) {
			ok = 1;
			DGifCloseFile(gif);
		} else {
			ok = 0;
		}
	} else {
		ok = 0;
	}
	return ok;
}

/* Decodes the first frame straight into an ImageRGBA, resolving the palette;
   the transparent index of the Graphics Control Extension gets alpha 0. Unlike
   the raw-buffer variant there is no progress context. */
int InputDataFilterGIF::GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha)
{
	tagRECT rect;
	int extCode;
	GifByteType *extData;
	GifRecordType recordType;
	int ret;
	int transparentIndex;
	unsigned char *rawBuf;
	int imgTop, imgLeft, imgWidth, imgHeight;
	int imageDone;
	GifFileType *gif;
	int ok;
	ColorMapObject *colorMap;
	int x, y;

	if (!m_ioFilter)
		goto noIoFilter;

	{
		PrivateGIFReader *reader = m_reader;

		reader->owner->m_ioFilter->Seek(0);
		gif = DGifOpen(reader->owner, PrivateGIFReader::read);
	}
	if (gif == 0)
		goto openFail;

	*outWidth = gif->SWidth;
	*outHeight = gif->SHeight;
	imageDone = 0;
	transparentIndex = -1;
	rawBuf = (unsigned char *)malloc(gif->SHeight * gif->SWidth);
	if (rawBuf == 0)
		goto finish;
	memset(rawBuf, gif->SBackGroundColor, gif->SHeight * gif->SWidth);

	ok = 1;
	while (DGifGetRecordType(gif, &recordType)) {
		switch (recordType) {
		case IMAGE_DESC_RECORD_TYPE: {
			unsigned char *buf;
			int i, j;
			int width, left, top, height;
			int lineOk;

			if (!DGifGetImageDesc(gif))
				goto errStop;
			left = gif->Image.Left;
			width = gif->Image.Width;
			top = gif->Image.Top;
			height = gif->Image.Height;
			if (gif->SWidth < left + width)
				goto errStop;
			if (gif->SHeight < top + height)
				goto errStop;
			buf = (unsigned char *)malloc(width);
			*out = new ImageRGBA(gif->SWidth, gif->SHeight, alpha, ImageRGBA::RGBA8888);
			if (buf == 0)
				goto allocFail;
			if (*out == 0)
				goto allocFail;
			rect.left = rect.right = 0;
			rect.top = rect.bottom = 0;
			if (gif->Image.Interlace) {
				for (i = 0; i < 4; i++) {
					for (j = top + InterlacedOffset[i]; j < top + height; j += InterlacedJumps[i]) {
						lineOk = DGifGetLine(gif, buf, width);
						if (lineOk) {
							if (imageDone == 0)
								memcpy(rawBuf + j * gif->SWidth + left, buf, width);
						} else
							break;
					}
					if (lineOk == 0)
						break;
				}
			} else {
				/* NOTE: bug: the non-interlaced loop stops at height instead of top + height. */
				for (i = top; i < height; i++) {
					lineOk = DGifGetLine(gif, buf, width);
					if (lineOk) {
						if (imageDone == 0)
							memcpy(rawBuf + i * gif->SWidth + left, buf, width);
					} else
						break;
				}
			}
			if (imageDone == 0) {
				imgTop = top;
				imgLeft = left;
				imgWidth = width;
				imgHeight = height;
			}
			imageDone = 1;
			ok = 0;
			free(buf);
			break;
allocFail:
			imageDone = 0;
			ok = 0;
			break;
		}
		case EXTENSION_RECORD_TYPE:
			if (!DGifGetExtension(gif, &extCode, &extData))
				goto errStop;
			if (extCode == 249) {
				if (extData[1] & 1)
					transparentIndex = extData[4];
			}
			while (extData != 0) {
				if (!DGifGetExtensionNext(gif, &extData)) {
					ok = 0;
					break;
				}
			}
			break;
		case TERMINATE_RECORD_TYPE:
			break;
		default:
			break;
errStop:
			ok = 0;
			break;
		}
		if (recordType == TERMINATE_RECORD_TYPE)
			break;
		if (!ok)
			break;
	}

finish:
	/* NOTE: bug: ret is left uninitialized when no image was decoded. */
	if (imageDone) {
		colorMap = gif->Image.ColorMap;
		if (colorMap == 0)
			colorMap = gif->SColorMap;
		if (*out != 0) {
			for (y = imgTop; y < imgTop + imgHeight; y++) {
				unsigned char *line = rawBuf + y * gif->SWidth;

				for (x = imgLeft; x < imgLeft + imgWidth; x++) {
					unsigned char idx = line[x];
					GifColorType *col = &colorMap->Colors[idx];

					if (idx == transparentIndex)
						(*out)->SetPixel(x, y, col->Red, col->Green, col->Blue, 0);
					else
						(*out)->SetPixel(x, y, col->Red, col->Green, col->Blue);
				}
			}
			ret = 0;
		} else
			ret = EICL_ERR_NO_MEMORY;
	}
	free(rawBuf);
	DGifCloseFile(gif);
	goto out;
openFail:
	ret = EICL_ERR_STREAM;
	goto out;
noIoFilter:
	ret = EICL_ERR_STREAM;
out:
	return ret;
}

/* Fills in the properties by decoding the first frame into a scratch buffer
   that is then discarded. Also reports whether the GIF is animated: a Graphics
   Control Extension with a nonzero delay, or a NETSCAPE application block. */
int InputDataFilterGIF::GetProps(PgProps *props)
{
	int ret;
	int animated;
	unsigned char *rawBuf;
	int imageDone;
	GifFileType *gif;
	GifRecordType recordType;
	int extCode;
	GifByteType *extData;
	int ok;
	int lineOk;

	if (m_ioFilter) {
		PrivateGIFReader *reader = m_reader;

		reader->owner->m_ioFilter->Seek(0);
		gif = DGifOpen(reader->owner, PrivateGIFReader::read);
		if (gif) {
			props->SetSize(gif->SWidth, gif->SHeight);
			props->SetBitsPerPixel(gif->SColorResolution);
			props->SetFormat((PgProps::PgFormat)128);
			ret = 1;
			rawBuf = (unsigned char *)malloc(gif->SHeight * gif->SWidth);
			if (rawBuf) {
				imageDone = 0;
				animated = 0;
				ok = 1;
				do {
					if (!DGifGetRecordType(gif, &recordType))
						break;
					switch (recordType) {
					case IMAGE_DESC_RECORD_TYPE: {
						int top, width, height;
						unsigned char *buf;
						int i, j;
						int left;

						if (!DGifGetImageDesc(gif))
							goto errStop;
						left = gif->Image.Left;
						width = gif->Image.Width;
						top = gif->Image.Top;
						height = gif->Image.Height;
						if (gif->SWidth < left + width)
							goto errStop;
						if (gif->SHeight < top + height)
							goto errStop;
						buf = (unsigned char *)malloc(width);
						if (buf == 0)
							goto allocFail;
						if (gif->Image.Interlace) {
							for (i = 0; i < 4; i++) {
								for (j = top + InterlacedOffset[i]; j < top + height;
								    j += InterlacedJumps[i]) {
									lineOk = DGifGetLine(gif, buf, width);
									if (lineOk) {
										if (imageDone == 0)
											memcpy(rawBuf + j * gif->SWidth
											        + left,
											    buf, width);
									} else
										break;
								}
								if (lineOk == 0)
									break;
							}
						} else {
							for (i = top; i < top + height; i++) {
								lineOk = DGifGetLine(gif, buf, width);
								if (lineOk) {
									if (imageDone == 0)
										memcpy(rawBuf + i * gif->SWidth + left,
										    buf, width);
								} else
									break;
							}
						}
						imageDone = 1;
						ok = 0;
						free(buf);
						break;
allocFail:
						imageDone = 0;
						ok = 0;
						break;
errStop:
						ok = 0;
						break;
					}
					case EXTENSION_RECORD_TYPE:
						if (!DGifGetExtension(gif, &extCode, &extData))
							goto errStop;
						while (extData != 0) {
							if ((extCode == 249 && (extData[2] || extData[3]))
							    || (extCode == 255 && extData[0] >= 10
							        && strncasecmp((char *)extData + 1, "NETSCAPE", 8)
							            == 0))
								animated = 1;
							if (!DGifGetExtensionNext(gif, &extData))
								goto errStop;
						}
						break;
					case TERMINATE_RECORD_TYPE:
						break;
					default:
						break;
					}
				} while (recordType != TERMINATE_RECORD_TYPE && ok);
				free(rawBuf);
			}
			props->SetImageType((PgProps::ImageType)animated);
			DGifCloseFile(gif);
		} else {
			ret = 0;
		}
	} else {
		ret = 0;
	}
	return ret;
}

/* Raw-buffer variant: decodes the first frame into an 8-bit indexed buffer
   (*f) and a 1024-byte RGBA palette (*g); *h is always 4 (indexed). c must be
   0 (automatic) or 4; d fills the palette, i/j receive the screen size and k
   is the _ImgDecStatus. The transparent index gets alpha 0.
   Returns EICL_ERR_FORMAT (format not 4), EICL_ERR_STREAM (no stream or DGifOpen failed),
   EICL_ERR_NO_MEMORY (out of memory), EICL_ERR_BAD_DATA (giflib error or cancelled), 0 on success. */
int InputDataFilterGIF::GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
    unsigned char **images, unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status)
{
	int ret;
	int transparentIndex;
	unsigned char *rawBuf;
	int imageDone;
	GifFileType *gif;
	GifRecordType recordType;
	int extCode;
	GifByteType *extData;
	int ok;
	int lineCount;
	int left;
	int lineOk;
	tagRECT rect;

	(void)flags;

	if (format != 0 && format != 4)
		goto badSize;
	ret = 0;
	if (!m_ioFilter)
		goto noIoFilter;

	{
		PrivateGIFReader *reader = m_reader;

		reader->owner->m_ioFilter->Seek(0);
		gif = DGifOpen(reader->owner, PrivateGIFReader::read);
	}
	if (gif == 0)
		goto openFail;

	*srcWidth = gif->SWidth;
	*srcHeight = gif->SHeight;
	transparentIndex = -1;
	if (*dstWidth == 0 || *dstHeight == 0) {
		*dstWidth = gif->SWidth;
		*dstHeight = gif->SHeight;
	} else {
		if (*dstWidth != gif->SWidth)
			goto badSize;
		if (*dstHeight != gif->SHeight)
			goto badSize;
	}

	imageDone = 0;
	rawBuf = new unsigned char[gif->SHeight * gif->SWidth];
	if (rawBuf == 0)
		goto finish;
	memset(rawBuf, gif->SBackGroundColor, gif->SHeight * gif->SWidth);

	ok = 1;
	do {
		if (!DGifGetRecordType(gif, &recordType)) {
			ret = EICL_ERR_BAD_DATA;
			break;
		}
		switch (recordType) {
		case IMAGE_DESC_RECORD_TYPE: {
			int top, width, height, bottom;
			unsigned char *buf;
			int i, j;

			if (!DGifGetImageDesc(gif)) {
				ret = EICL_ERR_BAD_DATA;
				ok = 0;
				break;
			}
			left = gif->Image.Left;
			width = gif->Image.Width;
			top = gif->Image.Top;
			height = gif->Image.Height;
			if (gif->SWidth < left + width)
				goto errStop;
			if (gif->SHeight < top + height)
				goto errStop;
			bottom = top + height;
			buf = (unsigned char *)malloc(width);
			if (buf == 0)
				goto allocFail;
			rect.left = rect.right = 0;
			rect.top = rect.bottom = 0;
			lineCount = 0;
			if (gif->Image.Interlace) {
				for (i = 0; i < 4; i++) {
					for (j = top + InterlacedOffset[i]; j < bottom; j += InterlacedJumps[i]) {
						if (((_ImgDecStatus *)status)->cancel == 1) {
							ret = EICL_ERR_BAD_DATA;
							break;
						}
						lineOk = DGifGetLine(gif, buf, width);
						if (lineOk) {
							if (imageDone == 0)
								memcpy(rawBuf + j * gif->SWidth + left, buf, width);
						} else
							break;
						lineCount++;
						((_ImgDecStatus *)status)->percent = lineCount * 100 / height;
					}
					if (lineOk == 0)
						break;
					if (ret == EICL_ERR_BAD_DATA)
						break;
				}
			} else {
				for (i = top; i < bottom; i++) {
					if (((_ImgDecStatus *)status)->cancel == 1) {
						ret = EICL_ERR_BAD_DATA;
						break;
					}
					lineOk = DGifGetLine(gif, buf, width);
					if (lineOk) {
						if (imageDone == 0)
							memcpy(rawBuf + i * gif->SWidth + left, buf, width);
					} else
						break;
					((_ImgDecStatus *)status)->percent = (i - top + 1) * 100 / height;
				}
			}
			imageDone = 1;
			free(buf);
			ok = 0;
			break;
allocFail:
			ret = EICL_ERR_NO_MEMORY;
			ok = 0;
			imageDone = 0;
			break;
		}
		case EXTENSION_RECORD_TYPE:
			if (!DGifGetExtension(gif, &extCode, &extData)) {
				ret = EICL_ERR_BAD_DATA;
				ok = 0;
				break;
			}
			if (extCode == 249) {
				if (extData[1] & 1)
					transparentIndex = extData[4];
			}
			while (extData != 0) {
				if (!DGifGetExtensionNext(gif, &extData)) {
					ret = EICL_ERR_BAD_DATA;
					ok = 0;
					break;
				}
			}
			break;
		case TERMINATE_RECORD_TYPE:
			break;
		default:
			break;
errStop:
			ok = 0;
			break;
		}
	} while (recordType != TERMINATE_RECORD_TYPE && ok);

finish:
	if (imageDone && ret == 0) {
		ColorMapObject *colorMap;
		unsigned char *clut;
		int colorCount;
		int n;

		colorMap = gif->Image.ColorMap;
		if (colorMap == 0)
			colorMap = gif->SColorMap;
		colorCount = colorMap->ColorCount;
		clut = new unsigned char[1024];
		*cluts = clut;
		if (clut == 0) {
			if (rawBuf)
				delete[] rawBuf;
			DGifCloseFile(gif);
			return EICL_ERR_NO_MEMORY;
		}
		memset(clut, alpha, 1024);
		for (n = 0; n < colorCount; n++) {
			(*cluts)[n * 4] = colorMap->Colors[n].Red;
			(*cluts)[n * 4 + 1] = colorMap->Colors[n].Green;
			(*cluts)[n * 4 + 2] = colorMap->Colors[n].Blue;
		}
		if ((unsigned int)transparentIndex < 256)
			(*cluts)[transparentIndex * 4 + 3] = 0;
		*srcFormat = 4;
		*images = rawBuf;
	}
	DGifCloseFile(gif);
	if (ret != 0) {
		if (rawBuf)
			delete[] rawBuf;
	}
	goto out;
badSize:
	return EICL_ERR_FORMAT;
openFail:
	ret = EICL_ERR_STREAM;
	goto out;
noIoFilter:
	ret = EICL_ERR_STREAM;
out:
	return ret;
}
