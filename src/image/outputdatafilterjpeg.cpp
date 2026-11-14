/*
 * JPEG encoder: compresses an ImageRGBA with libjpeg through a destination
 * manager that writes to the OutputIoFilter.
 */

#include "outputdatafilterjpeg.h"
#include "eiclerror.h"
#include "imagergba.h"
#include "iofilter.h"

#include <stdio.h>
extern "C" {
#include "jpeglib.h"
}

OutputDataFilterJPEG::OutputDataFilterJPEG() : OutputDataFilter()
{
	m_ioFilter = 0;
	PrivateJPEGWriter *w = new PrivateJPEGWriter;
	w->m_filter = this;
	m_writer = w;
	return;  /* redundant as written */
}

OutputDataFilterJPEG::OutputDataFilterJPEG(OutputIoFilter *ioFilter) : OutputDataFilter()
{
	m_ioFilter = ioFilter;
	PrivateJPEGWriter *w = new PrivateJPEGWriter;
	w->m_filter = this;
	m_writer = w;
	return;  /* redundant as written */
}

OutputDataFilterJPEG::~OutputDataFilterJPEG()
{
	delete m_writer;
}

/* libjpeg destination manager extended with the filter and its buffer. */
struct JPEGBnDest : public jpeg_destination_mgr {
	/* 0x14 */ OutputDataFilterJPEG *filter;
	/* 0x18 */ JOCTET *buffer;
};

inline void OutputDataFilterJPEG::PrivateJPEGWriter::init_destination(jpeg_compress_struct *cinfo)
{
	JPEGBnDest *dest = (JPEGBnDest *)cinfo->dest;
	dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, 1, 4096);
	dest->next_output_byte = dest->buffer;
	dest->free_in_buffer = 4096;
}

inline int OutputDataFilterJPEG::PrivateJPEGWriter::empty_output_buffer(jpeg_compress_struct *cinfo)
{
	JPEGBnDest *dest = (JPEGBnDest *)cinfo->dest;
	dest->filter->m_ioFilter->Write(4096, dest->buffer);
	dest->free_in_buffer = 4096;
	dest->next_output_byte = dest->buffer;
	return 1;
}

inline void OutputDataFilterJPEG::PrivateJPEGWriter::term_destination(jpeg_compress_struct *cinfo)
{
	JPEGBnDest *dest = (JPEGBnDest *)cinfo->dest;
	unsigned int used = 4096 - dest->free_in_buffer;
	if (used) {
		dest->filter->m_ioFilter->Write(used, dest->buffer);
	}
}

/* Compresses the image row by row (ImageRGBA::GetLine, RGB888) at quality 84.
   Returns EICL_ERR_STREAM_WRITE if a scanline cannot be written. */
int OutputDataFilterJPEG::SetRawData(const ImageRGBA *img)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];
	int result;

	if (m_ioFilter->ObjectIdentifier()) {
		unsigned char *buf;

		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_compress(&cinfo);
		buf = (unsigned char *)malloc(img->Width() * 3);
		/* NOTE: bug: result is returned uninitialized when malloc fails. */
		if (buf) {
			PrivateJPEGWriter *w = m_writer;
			j_compress_ptr ci = &cinfo;
			JPEGBnDest *dest;

			if (ci->dest == 0)
				ci->dest = (struct jpeg_destination_mgr *)(*ci->mem->alloc_small)((j_common_ptr)ci,
				    JPOOL_PERMANENT, sizeof(JPEGBnDest));
			dest = (JPEGBnDest *)ci->dest;
			dest->init_destination = OutputDataFilterJPEG::PrivateJPEGWriter::init_destination;
			dest->empty_output_buffer = OutputDataFilterJPEG::PrivateJPEGWriter::empty_output_buffer;
			dest->term_destination = OutputDataFilterJPEG::PrivateJPEGWriter::term_destination;
			dest->filter = w->m_filter;
			cinfo.image_width = img->Width();
			cinfo.image_height = img->Height();
			cinfo.input_components = 3;
			cinfo.in_color_space = JCS_RGB;
			cinfo.dct_method = JDCT_FLOAT;  /* redundant as written */
			jpeg_set_defaults(&cinfo);
			jpeg_set_quality(&cinfo, 84, FALSE);
			jpeg_start_compress(&cinfo, TRUE);
			row_pointer[0] = buf;
			result = 0;
			while (cinfo.next_scanline < cinfo.image_height && result == 0) {
				img->GetLine(cinfo.next_scanline, buf, ImageRGBA::RGB888);
				result = jpeg_write_scanlines(&cinfo, row_pointer, 1) == 0 ? 0x103 : 0;
			}
			free(buf);
		}
		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);
	} else {
		result = EICL_ERR_NO_MEMORY;
	}
	return result;
}
