/*
 * InputDataFilter: base class of the image decoders' data filters.
 */

#pragma implementation "inputdatafilter.h"
#include "inputdatafilter.h"
#include "eiclerror.h"

InputDataFilter::InputDataFilter()
{
	m_ioFilter = 0;
	InitVariables();
}

InputDataFilter::InputDataFilter(InputIoFilter *ioFilter)
{
	m_ioFilter = ioFilter;
	InitVariables();
}

InputDataFilter::~InputDataFilter()
{
	ReleaseRawData();
}

int InputDataFilter::GetProps(PgProps *props)
{
	return 0;
}

int InputDataFilter::SetMovieTime(int time)
{
	m_movieTime = time;
	return 1;
}

int InputDataFilter::ReleaseRawData()
{
	return 1;
}

int InputDataFilter::GetRawDataFast(const EiclSize *sz, RawImg **out)
{
	unsigned w = sz->GetWidth();
	unsigned h = sz->GetHeight();
	return GetRawDataFast(w, h, out);
}

int InputDataFilter::GetRawDataFast(unsigned width, unsigned height, RawImg **out)
{
	return GetRawData(out);
}

void InputDataFilter::RegisterCallback(DataFilterCallback cb, void *userData)
{
	m_callback = cb;
	m_callbackData = userData;
}

int InputDataFilter::CallCallback(int mode, unsigned char *buf, tagRECT *rect)
{
	if (!m_callback)
		return -1;
	return m_callback(mode, m_callbackData, buf, rect);
}

void InputDataFilter::InitVariables()
{
	m_callback = 0;
	m_callbackData = 0;
}

int InputDataFilter::GetRawData(RawImg **out)
{
	return EICL_ERR_STREAM;
}

int InputDataFilter::GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha)
{
	return EICL_ERR_STREAM;
}

int InputDataFilter::GetRawData(ImageRGBA **&out, int *&delays, int &imageCount, int frameNo, unsigned char alpha)
{
	return EICL_ERR_STREAM;
}

int InputDataFilter::GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
    unsigned char **images, unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status)
{
	return EICL_ERR_STREAM;
}

int InputDataFilter::GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
    unsigned char **images, unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status,
    int *&delays, int &imageCount, int frameNo)
{
	return EICL_ERR_STREAM;
}
