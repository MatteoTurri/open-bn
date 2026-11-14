/*
 * PNG encoder: writes an ImageRGBA as 8-bit RGB through libpng.
 */

#include "outputdatafilterpng.h"
#include "eiclerror.h"
#include "imagergba.h"
#include "iofilter.h"

#include "png.h"

/* libpng write callback: forwards the bytes to the OutputIoFilter and records
   a write error in m_writeError (errors are sticky). */
void write_to_io_filter(struct png_struct_def *png, unsigned char *data, unsigned length)
{
	OutputDataFilterPNG *self = (OutputDataFilterPNG *)png_get_io_ptr(png);
	int err;

	/* NOTE: bug: with no io pointer the flag is stored through a NULL pointer. */
	if (self == 0) {
		err = 1;
	} else if (self->m_writeError != 0) {
		err = 1;
	} else {
		OutputIoFilter *io = self->GetOutputIoFilter();

		if (io != 0)
			err = (io->Write(length, data) != (int)length);
		else
			err = 1;
	}
	self->m_writeError = err;
}

OutputDataFilterPNG::OutputDataFilterPNG() : OutputDataFilter()
{
	m_ioFilter = 0;
	InitializeValiables_();
	return;  /* redundant as written */
}

OutputDataFilterPNG::OutputDataFilterPNG(OutputIoFilter *ioFilter) : OutputDataFilter()
{
	m_ioFilter = ioFilter;
	InitializeValiables_();
	return;  /* redundant as written */
}

int OutputDataFilterPNG::InitializeValiables_()
{
	m_writeError = 0;
	return 1;
}

/* Writes the image as 8-bit RGB (color type 2) with png_write_image.
   Returns EICL_ERR_STREAM without an output filter, EICL_ERR_CODEC if libpng cannot be set
   up, EICL_ERR_NO_MEMORY if out of memory, EICL_ERR_STREAM_WRITE after a write error, 0 on success. */
int OutputDataFilterPNG::SetRawData(const ImageRGBA *img)
{
	int result;

	if (m_ioFilter != 0) {
		png_structp png = png_create_write_struct("1.0.3", 0, 0, 0);

		if (png != 0) {
			png_infop info = png_create_info_struct(png);

			if (info != 0) {
				unsigned char **rows;

				png_set_write_fn(png, this, write_to_io_filter, 0);
				png_set_IHDR(png, info, img->Width(), img->Height(), 8, 2, 0, 0, 0);
				png_write_info(png, info);
				rows = (unsigned char **)malloc(img->Height() * 4);
				if (rows != 0) {
					unsigned int rows3 = img->Height() * 3;
					unsigned char *buf = (unsigned char *)malloc(img->Width() * rows3);

					if (buf != 0) {
						unsigned int i;

						img->GetImage(buf, ImageRGBA::RGB888);
						for (i = 0; i < img->Height(); i++)
							rows[i] = buf + i * (img->Width() * 3);
						png_write_image(png, rows);
						if (m_writeError == 0)
							png_write_end(png, info);
						result = m_writeError != 0 ? 0x103 : 0;
					} else {
						result = EICL_ERR_NO_MEMORY;
					}
					free(rows);
				} else {
					result = EICL_ERR_NO_MEMORY;
				}
				png_destroy_write_struct(&png, &info);
			} else {
				png_destroy_write_struct(&png, 0);
				result = EICL_ERR_CODEC;
			}
		} else {
			result = EICL_ERR_CODEC;
		}
	} else {
		result = EICL_ERR_STREAM;
	}
	return result;
}

OutputDataFilterPNG::~OutputDataFilterPNG()
{}
