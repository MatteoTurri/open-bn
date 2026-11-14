/*
 * OutputDataFilterTIFF: TIFF encoder on top of libtiff. m_handler is allocated
 * only by the constructor that takes an OutputIoFilter.
 */

#ifndef OUTPUTDATAFILTERTIFF_H
#define OUTPUTDATAFILTERTIFF_H

#include "outputdatafilter.h"

class TIFFWriteHandler;

class OutputDataFilterTIFF : public OutputDataFilter {
public:
	OutputDataFilterTIFF();
	OutputDataFilterTIFF(OutputIoFilter *ioFilter);
	virtual ~OutputDataFilterTIFF();

	virtual int SetRawData(const ImageRGBA *img);
	virtual int SetProps(PgProps *props);

private:
	/* 0x14 */ int m_compression;  /* PgProps TIFF compression code */
	/* 0x18 */ TIFFWriteHandler *m_handler;
};

#endif
