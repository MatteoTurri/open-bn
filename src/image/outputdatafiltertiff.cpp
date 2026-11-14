/*
 * TIFF encoder: writes an ImageRGBA through libtiff with the TIFFWriteHandler
 * client callbacks.
 */

#include "outputdatafiltertiff.h"
#include "eiclerror.h"
#include "tiffwritehandler.h"
#include "pgprops.h"
#include "imagergba.h"
#include "iofilter.h"

#include "../../ref/tiff-3.4beta037/tiffio.h"

OutputDataFilterTIFF::OutputDataFilterTIFF() : OutputDataFilter()
{
	m_ioFilter = 0;
	m_handler = 0;
	m_compression = 0;
}

OutputDataFilterTIFF::OutputDataFilterTIFF(OutputIoFilter *ioFilter) : OutputDataFilter()
{
	m_ioFilter = ioFilter;
	m_handler = new TIFFWriteHandler;
	m_compression = 0;
	m_handler->m_ioFilter = m_ioFilter;
}

OutputDataFilterTIFF::~OutputDataFilterTIFF()
{
	delete m_handler;
	m_handler = 0;
}

/* Opens the client stream in mode "wm" and writes the tags and the scanlines.
   Compression codes: 0 none, 1 LZW, 2 CCITT G3, 3 CCITT G4, others none.
   Returns EICL_ERR_STREAM if TIFFClientOpen fails, EICL_ERR_NO_MEMORY if out of memory, EICL_ERR_STREAM_WRITE if a
   scanline cannot be written, 0 on success. */
int OutputDataFilterTIFF::SetRawData(const ImageRGBA *img)
{
	TIFFWriteHandler *handler = m_handler;
	TIFF *tif;
	int result;

	tif = TIFFClientOpen(handler->m_ioFilter->ObjectIdentifier(), "wm", handler, TIFFWriteHandler::read,
	    TIFFWriteHandler::write, TIFFWriteHandler::seek, TIFFWriteHandler::close, TIFFWriteHandler::size,
	    TIFFWriteHandler::map, TIFFWriteHandler::unmap);
	if (tif) {
		unsigned char *buf;
		unsigned int width = img->Width();

		TIFFSetField(tif, 256, width);

		unsigned int height = img->Height();

		TIFFSetField(tif, 257, height);
		TIFFSetField(tif, 262, 2);
		TIFFSetField(tif, 284, 1);
		TIFFSetField(tif, 278, height);
		TIFFSetField(tif, 258, 8);
		TIFFSetField(tif, 277, 3);
		if (m_compression == 0)
			TIFFSetField(tif, 259, 1);
		else if (m_compression == 1)
			TIFFSetField(tif, 259, 5);
		else if (m_compression == 2)
			TIFFSetField(tif, 259, 3);
		else if (m_compression == 3)
			TIFFSetField(tif, 259, 4);
		else
			TIFFSetField(tif, 259, 1);
		TIFFSetField(tif, 296, 2);
		TIFFSetField(tif, 282, 72.0);
		TIFFSetField(tif, 283, 72.0);
		buf = (unsigned char *)malloc(width * 3);
		if (buf) {
			int row;

			result = 0;
			for (row = 0; row < (int)height && result == 0; row++) {
				img->GetLine(row, buf, ImageRGBA::RGB888);
				result = TIFFWriteScanline(tif, buf, row, 0) > 0 ? 0 : 0x103;
			}
			free(buf);
		} else {
			result = EICL_ERR_NO_MEMORY;
		}
		TIFFClose(tif);
	} else {
		result = EICL_ERR_STREAM;
	}
	return result;
}

int OutputDataFilterTIFF::SetProps(PgProps *props)
{
	m_compression = props->GetTiffCompression();
	return OutputDataFilter::SetProps(props);
}
