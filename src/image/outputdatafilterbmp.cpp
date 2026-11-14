/*
 * BMP encoder: writes an uncompressed 24 bpp BMP from an ImageRGBA.
 */

#include "outputdatafilterbmp.h"
#include "eiclerror.h"
#include "imagergba.h"
#include "iofilter.h"

/* Clears n bytes at p and returns p. */
#include "eiclmemory.h"

OutputDataFilterBMP::OutputDataFilterBMP() : OutputDataFilter()
{
	m_ioFilter = 0;
}

OutputDataFilterBMP::OutputDataFilterBMP(OutputIoFilter *ioFilter) : OutputDataFilter()
{
	m_ioFilter = ioFilter;
}

OutputDataFilterBMP::~OutputDataFilterBMP()
{}

/* Writes BITMAPFILEHEADER (14 bytes), BITMAPINFOHEADER (40 bytes) and the
   BGR888 rows bottom-up, each padded to 4 bytes. The headers are zeroed byte
   buffers, so the fields that are not written stay 0. */
int OutputDataFilterBMP::SetRawData(const ImageRGBA *img)
{
	int result;

	if (m_ioFilter != 0) {
		unsigned int rowSize = (img->Width() * 3 + 3) & ~3u;
		unsigned int offBits = 54;
		unsigned int fileSize = img->Height() * rowSize + offBits;

		unsigned char fileHeader[14];
		EiclZeroMemory(fileHeader, 14);
		fileHeader[0] = 'B';
		fileHeader[1] = 'M';
		fileHeader[2] = (unsigned char)fileSize;
		unsigned int b = fileSize >> 8;
		fileHeader[3] = (unsigned char)b;
		b = fileSize >> 16;
		fileHeader[4] = (unsigned char)b;
		b = fileSize >> 24;
		fileHeader[5] = (unsigned char)b;
		fileHeader[10] = (unsigned char)offBits;
		fileHeader[11] = (unsigned char)(offBits >> 8);
		fileHeader[12] = (unsigned char)(offBits >> 16);
		fileHeader[13] = (unsigned char)(offBits >> 24);

		unsigned int hdrSize = 40;
		unsigned char infoHeader[40];
		EiclZeroMemory(infoHeader, hdrSize);
		infoHeader[0] = (unsigned char)hdrSize;
		infoHeader[1] = (unsigned char)(hdrSize >> 8);
		infoHeader[2] = (unsigned char)(hdrSize >> 16);
		infoHeader[3] = (unsigned char)(hdrSize >> 24);
		infoHeader[12] = 1;
		infoHeader[13] = 0;
		infoHeader[14] = 24;
		infoHeader[15] = 0;
		infoHeader[16] = 0;
		infoHeader[17] = 0;
		infoHeader[18] = 0;
		infoHeader[19] = 0;
		infoHeader[20] = 0;
		infoHeader[21] = 0;
		infoHeader[22] = 0;
		infoHeader[23] = 0;
		unsigned int w = img->Width();
		infoHeader[4] = (unsigned char)w;
		unsigned int c = w >> 8;
		infoHeader[5] = (unsigned char)c;
		c = w >> 16;
		infoHeader[6] = (unsigned char)c;
		c = w >> 24;
		infoHeader[7] = (unsigned char)c;
		unsigned int h = img->Height();
		infoHeader[8] = (unsigned char)h;
		unsigned int d = h >> 8;
		infoHeader[9] = (unsigned char)d;
		d = h >> 16;
		infoHeader[10] = (unsigned char)d;
		d = h >> 24;
		infoHeader[11] = (unsigned char)d;

		if (m_ioFilter->Write(14, fileHeader) != 14)
			return EICL_ERR_STREAM_WRITE;
		if (m_ioFilter->Write(hdrSize, infoHeader) != (int)hdrSize)
			return EICL_ERR_STREAM_WRITE;

		unsigned char *rowBuf = new unsigned char[rowSize];
		EiclZeroMemory(rowBuf, rowSize);

		result = 0;
		for (int row = (int)img->Height() - 1; row >= 0; row--) {
			img->GetLine(row, rowBuf, ImageRGBA::BGR888);
			if (m_ioFilter->Write(rowSize, rowBuf) != (int)rowSize) {
				result = EICL_ERR_STREAM_WRITE;
				break;
			}
		}

		if (rowBuf)
			delete[] rowBuf;
	} else {
		result = EICL_ERR_STREAM;
	}

	return result;
}
