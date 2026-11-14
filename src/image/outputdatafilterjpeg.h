/*
 * OutputDataFilterJPEG: JPEG encoder on top of libjpeg. PrivateJPEGWriter holds
 * the destination manager callbacks (cf. jdatadst.c).
 */

#ifndef OUTPUTDATAFILTERJPEG_H
#define OUTPUTDATAFILTERJPEG_H

#include "outputdatafilter.h"

struct jpeg_compress_struct;

class OutputDataFilterJPEG : public OutputDataFilter {
public:
	class PrivateJPEGWriter;
	friend class PrivateJPEGWriter;

	class PrivateJPEGWriter {
	public:
		OutputDataFilterJPEG *m_filter;

		static void init_destination(jpeg_compress_struct *cinfo);
		static int empty_output_buffer(jpeg_compress_struct *cinfo);
		static void term_destination(jpeg_compress_struct *cinfo);
	};

	OutputDataFilterJPEG();
	OutputDataFilterJPEG(OutputIoFilter *ioFilter);
	virtual ~OutputDataFilterJPEG();

	virtual int SetRawData(const ImageRGBA *img);

private:
	/* 0x14 */ PrivateJPEGWriter *m_writer;
};

#endif
