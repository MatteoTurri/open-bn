/*
 * MNG decoder (InputDataFilterMNG) on top of libmng: decodes one frame or all
 * the frames of an animation, with their durations.
 */

#include "inputdatafilter.h"
#include "eiclerror.h"
#include "imagergba.h"
#include "theimagedecoder.h"
#include "libmng.h"
#include <vector>
#include <stdlib.h>

#include "iofilter.h"
#include "inputdatafiltermng.h"

class InputDataFilterMNG::PrivateMNGReader {
public:
	/* One decoded frame: the image and its duration (from m_timer). */
	struct frameTag {
		/* 0x00 */ ImageRGBA *img;
		/* 0x04 */ int delay;
	};
	/* One decoded frame: the pixels (freed with delete[]) and its duration. */
	struct frameImageTag {
		/* 0x00 */ unsigned char *data;
		/* 0x04 */ int delay;
	};

	PrivateMNGReader(InputDataFilterMNG *owner) : m_owner(owner) {}

	static mng_ptr myalloc(mng_size_t size) { return calloc(1, size); }

	static void myfree(mng_ptr p, mng_size_t size) { free(p); }

	static mng_bool myopenstream(mng_handle h) { return 1; }

	static mng_bool myclosestream(mng_handle h) { return 1; }

	static mng_bool myreaddata(mng_handle h, mng_ptr buf, mng_uint32 size, mng_uint32 *bytesread)
	{
		PrivateMNGReader *self = (PrivateMNGReader *)mng_get_userdata(h);
		*bytesread = self->m_owner->m_ioFilter->Read(size, (unsigned char *)buf);
		return 1;
	}

	static mng_bool myprocessheader(mng_handle h, mng_uint32 width, mng_uint32 height)
	{
		PrivateMNGReader *self = (PrivateMNGReader *)mng_get_userdata(h);
		self->m_width = width;
		self->m_height = height;
		unsigned int rowbytes = height * 4;
		self->m_canvas = (unsigned char *)malloc(width * rowbytes);
		mng_set_canvasstyle(h, MNG_CANVAS_RGBA8);
		return 1;
	}

	static mng_ptr mygetcanvasline(mng_handle h, mng_uint32 line)
	{
		PrivateMNGReader *self = (PrivateMNGReader *)mng_get_userdata(h);
		return self->m_canvas + line * self->m_width * 4;
	}

	static mng_bool myrefresh(mng_handle h, mng_uint32 a, mng_uint32 b, mng_uint32 c, mng_uint32 d)
	{
		mng_get_userdata(h);
		return 1;
	}

	static mng_uint32 mygetticks(mng_handle h) { return 1; }

	static mng_bool mysettimer(mng_handle h, mng_uint32 msecs)
	{
		PrivateMNGReader *self = (PrivateMNGReader *)mng_get_userdata(h);
		self->m_timer = msecs;
		return 1;
	}

	/* Debug callback for mng_iterate_chunks (prints the chunk id); never registered. */
	static mng_bool myiteratechunk(mng_handle h, mng_handle chunk, mng_chunkid id, mng_uint32 seq)
	{
		printf("%c%c%c%c\n", (char)(id >> 24), (char)(id >> 16), (char)(id >> 8), (char)id);
		return 1;
	}

	/* Decodes the frames of the MNG stream into two parallel arrays (images and
	   durations) allocated here; `which` == -1 takes every frame, otherwise only
	   the frame with that index. Returns the number of frames. */
	int ReadFrames(ImageRGBA **&imgs, int *&delays, int which, unsigned char depth)
	{
		mng_handle handle;
		mng_retcode code;
		int n;
		int count;
		int i;

		handle = mng_initialize((mng_ptr)this, myalloc, myfree, 0);
		if (handle == 0)
			return 0;
		m_owner->m_ioFilter->Seek(0);
		mng_setcb_openstream(handle, myopenstream);
		mng_setcb_closestream(handle, myclosestream);
		mng_setcb_readdata(handle, myreaddata);
		mng_setcb_gettickcount(handle, mygetticks);
		mng_setcb_settimer(handle, mysettimer);
		mng_setcb_processheader(handle, myprocessheader);
		mng_setcb_getcanvasline(handle, mygetcanvasline);
		mng_setcb_refresh(handle, myrefresh);
		mng_set_storechunks(handle, MNG_TRUE);
		mng_set_suspensionmode(handle, MNG_TRUE);
		mng_set_cacheplayback(handle, MNG_FALSE);

		n = 0;
		code = mng_readdisplay(handle);
		while (code == MNG_NEEDMOREDATA || code == MNG_NEEDTIMERWAIT) {
			if (which == -1 || which == n) {
				frameTag *tag;

				tag = new frameTag;
				tag->img = new ImageRGBA(m_width, m_height, depth, ImageRGBA::RGBA8888);
				tag->img->SetImage(m_canvas, ImageRGBA::RGBA8888, 0);
				tag->delay = m_timer;
				m_frameTags.push_back(tag);
				if (which != -1)
					break;
			}
			code = mng_display_resume(handle);
			n++;
		}
		mng_cleanup(&handle);

		count = m_frameTags.size();
		if (count == 0) {
			imgs = 0;
			delays = 0;
			return 0;
		}
		imgs = new ImageRGBA *[count];
		delays = new int[count];
		for (i = 0; i < count; i++) {
			frameTag *tag = m_frameTags[i];

			imgs[i] = tag->img;
			delays[i] = tag->delay;
		}
		return count;
	}

	/* Public: InputDataFilterMNG::GetRawData reads and writes this state directly. */
public:
	/* 0x00 */ unsigned int m_status;  /* error code */
	/* 0x04 */ unsigned int m_width;
	/* 0x08 */ unsigned int m_height;
	/* 0x0C */ unsigned char *m_canvas;
	/* 0x10 */ unsigned int m_timer;
	/* 0x14 */ std::vector<frameTag *> m_frameTags;
	/* 0x20 */ std::vector<frameImageTag *> m_frameImages;
	/* 0x2C */ InputDataFilterMNG *m_owner;
};

InputDataFilterMNG::InputDataFilterMNG() : InputDataFilter()
{
	m_reader = new PrivateMNGReader(this);
}

InputDataFilterMNG::InputDataFilterMNG(InputIoFilter *ioFilter) : InputDataFilter(ioFilter)
{
	m_reader = new PrivateMNGReader(this);
}

InputDataFilterMNG::~InputDataFilterMNG()
{
	if (m_reader)
		delete m_reader;
}

int InputDataFilterMNG::IsSupported()
{
	return 1;
}

/* Decodes only the first frame into an ImageRGBA (ReadFrames with which == 0)
   and frees the two arrays. */
int InputDataFilterMNG::GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha)
{
	ImageRGBA **imgs;
	int *delays;
	int count;

	if (!m_ioFilter)
		return EICL_ERR_STREAM;

	*outHeight = 0;
	*outWidth = 0;
	count = m_reader->ReadFrames(imgs, delays, 0, alpha);
	if (count == 0)
		return EICL_ERR_BAD_DATA;
	*out = imgs[0];
	if (imgs)
		delete imgs;
	if (delays)
		delete delays;
	return 0;
}

/* Returns the frames through the references: images in out, durations in a,
   frame count in b; c selects the frame (-1 = all). */
int InputDataFilterMNG::GetRawData(ImageRGBA **&out, int *&delays, int &imageCount, int frameNo, unsigned char alpha)
{
	int count;

	if (!m_ioFilter)
		return EICL_ERR_STREAM;

	count = m_reader->ReadFrames(out, delays, frameNo, alpha);
	imageCount = count;
	return count != 0 ? 0 : 4097;
}

int InputDataFilterMNG::GetProps(PgProps *props)
{
	return 1;
}


/* Animated MNG decoding: runs mng_readdisplay/mng_display_resume and keeps a
   buffer for each frame (or only for frame n; n == -1 = all) in
   m_reader->m_frameImages, then builds the per-frame arrays of pixels and
   durations. a, b: requested size; c: format (0 = automatic, 3); d: alpha
   value; e: option mask; h: format produced (always 3); i, j: size; l: frame
   durations; m: frame count; n: frame index (-1 = all).
   Returns EICL_ERR_STREAM without a stream, otherwise 0 if there are frames, EICL_ERR_BAD_DATA if not;
   m_reader->m_status collects EICL_ERR_BAD_DATA, EICL_ERR_NO_MEMORY or EICL_ERR_FORMAT along the way. */
int InputDataFilterMNG::GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
    unsigned char **images, unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status,
    int *&delays, int &imageCount, int frameNo)
{
	PrivateMNGReader *reader;
	mng_handle handle;
	int frameIndex;
	int count;
	int size;
	mng_retcode code;
	int idx;
	int fmt;

	(void)images;
	(void)cluts;
	(void)status;

	if (!m_ioFilter)
		return EICL_ERR_STREAM;

	fmt = format;
	reader = m_reader;
	reader->m_status = 0;
	handle = mng_initialize((mng_ptr)reader, PrivateMNGReader::myalloc, PrivateMNGReader::myfree, 0);
	if (handle == 0) {
		reader->m_status = EICL_ERR_BAD_DATA;
		count = 0;
		goto setCount;
	}
	reader->m_owner->m_ioFilter->Seek(0);
	mng_setcb_openstream(handle, PrivateMNGReader::myopenstream);
	mng_setcb_closestream(handle, PrivateMNGReader::myclosestream);
	mng_setcb_readdata(handle, PrivateMNGReader::myreaddata);
	mng_setcb_gettickcount(handle, PrivateMNGReader::mygetticks);
	mng_setcb_settimer(handle, PrivateMNGReader::mysettimer);
	mng_setcb_processheader(handle, PrivateMNGReader::myprocessheader);
	mng_setcb_getcanvasline(handle, PrivateMNGReader::mygetcanvasline);
	mng_setcb_refresh(handle, PrivateMNGReader::myrefresh);
	mng_set_storechunks(handle, MNG_TRUE);
	mng_set_suspensionmode(handle, MNG_TRUE);
	mng_set_cacheplayback(handle, MNG_FALSE);

	frameIndex = 0;
	code = mng_readdisplay(handle);
	while (code == MNG_NEEDMOREDATA || code == MNG_NEEDTIMERWAIT) {
		if (frameNo == -1 || frameNo == frameIndex) {
			PrivateMNGReader::frameImageTag *frame;
			int npix;
			int fill = 0;

			frame = new PrivateMNGReader::frameImageTag;
			if (frame == 0)
				reader->m_status = EICL_ERR_NO_MEMORY;
			frame->data = 0;
			if (*dstWidth == 0 || *dstHeight == 0) {
				*dstWidth = reader->m_width;
				*dstHeight = reader->m_height;
			} else if (*dstWidth != (int)reader->m_width || *dstHeight != (int)reader->m_height) {
				reader->m_status = EICL_ERR_FORMAT;
			}
			if (fmt == 0)
				fmt = 3;
			if (fmt == 2) {
				unsigned char *dst;
				unsigned char *src;

				frame->data = new unsigned char[reader->m_width * 3 * reader->m_height];
				if (frame->data == 0)
					reader->m_status = EICL_ERR_NO_MEMORY;
				/* NOTE: bug: the buffer is written even when the allocation failed. */
				npix = reader->m_width * reader->m_height;
				dst = frame->data;
				src = reader->m_canvas;
				if (npix > 0) {
					int pixelsLeft = npix;

					do {
						*dst++ = *src++;
						*dst++ = *src++;
						*dst++ = *src;
						src += 2;
					} while (--pixelsLeft);
				}
			} else if (fmt == 3) {
				frame->data = new unsigned char[reader->m_width * 4 * reader->m_height];
				if (frame->data == 0)
					reader->m_status = EICL_ERR_NO_MEMORY;
				{
					unsigned int rowbytes = reader->m_height * 4;

					memcpy(frame->data, reader->m_canvas, reader->m_width * rowbytes);
				}
				fill = flags & theImageDecoder::fill_alpha;
				if (fill) {
					int pixelsLeft = reader->m_width * reader->m_height;
					unsigned char *p = frame->data + 3;

					if (pixelsLeft > 0) {
						do {
							*p = (unsigned char)alpha;
							p += 4;
						} while (--pixelsLeft);
					}
				}
			} else {
				reader->m_status = EICL_ERR_FORMAT;
			}

			if (reader->m_status != 0)
				goto frameError;
			frame->delay = reader->m_timer;
			reader->m_frameImages.push_back(frame);
			if (frameNo != -1)
				break;
			goto nextFrame;
frameError:
			if (frame->data) {
				delete[] frame->data;
				frame->data = 0;
			}
			size = reader->m_frameImages.end() - reader->m_frameImages.begin();
			for (idx = 0; idx < size; idx++) {
				PrivateMNGReader::frameImageTag *ft = reader->m_frameImages[idx];

				if (ft->data) {
					delete[] ft->data;
					ft->data = 0;
				}
			}
			count = 0;
			delays = 0;
			goto setCount;
		}
nextFrame:
		code = mng_display_resume(handle);
		frameIndex++;
	}

	mng_cleanup(&handle);
	size = reader->m_frameImages.end() - reader->m_frameImages.begin();
	if (size == 0) {
		count = 0;
		delays = 0;
		reader->m_status = EICL_ERR_BAD_DATA;
	} else {
		unsigned char **datas;
		int i;

		/* NOTE: bug: the pixel pointer array is never handed back to the caller. */
		datas = new unsigned char *[size];
		delays = new int[size];
		for (i = 0; i < size; i++) {
			PrivateMNGReader::frameImageTag *ft = reader->m_frameImages[i];

			datas[i] = ft->data;
			delays[i] = ft->delay;
		}
		count = size;
	}
setCount:
	imageCount = count;
	*srcWidth = *dstWidth;
	*srcHeight = *dstHeight;
	if (imageCount == 0)
		return EICL_ERR_BAD_DATA;
	*srcFormat = 3;
	return 0;
}
