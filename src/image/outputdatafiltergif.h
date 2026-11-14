/*
 * OutputDataFilterGIF: GIF encoder on top of giflib. The giflib output
 * callback finds the filter through GifFileType::UserData.
 */

#ifndef OUTPUTDATAFILTERGIF_H
#define OUTPUTDATAFILTERGIF_H

#include "outputdatafilter.h"

struct GifFileType;

class OutputDataFilterGIF : public OutputDataFilter {
public:
	class PrivateGIFWriter;
	friend class PrivateGIFWriter;

	class PrivateGIFWriter {
	public:
		OutputDataFilterGIF *m_filter;

		static int write(GifFileType *gif, const unsigned char *buf, int len);
	};

	OutputDataFilterGIF();
	OutputDataFilterGIF(OutputIoFilter *ioFilter);
	virtual ~OutputDataFilterGIF();

	virtual int SetRawData(const ImageRGBA *img);

private:
	/* 0x14 */ PrivateGIFWriter *m_writer;
};

#endif
