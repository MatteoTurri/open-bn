/*
 * OutputDataFilter base class: callback registration and the default
 * SetRawData stubs, which return EICL_ERR_CODEC (not supported).
 */

#pragma implementation "outputdatafilter.h"
#include "outputdatafilter.h"
#include "eiclerror.h"

OutputDataFilter::OutputDataFilter()
{
	InitializeVariables();
}

/* NOTE: bug: ioFilter is ignored and m_ioFilter stays 0. */
OutputDataFilter::OutputDataFilter(OutputIoFilter *ioFilter)
{
	InitializeVariables();
}

OutputDataFilter::~OutputDataFilter()
{}

int OutputDataFilter::SetProps(PgProps *props)
{
	m_props = props;
	return 0;
}

void OutputDataFilter::InitializeVariables()
{
	m_ioFilter = 0;
	m_callback = 0;
	m_callbackData = 0;
	m_props = 0;
}

OutputIoFilter *OutputDataFilter::GetOutputIoFilter()
{
	return m_ioFilter;
}

int OutputDataFilter::SetRawData(const RawImg *img)
{
	(void)img;
	return EICL_ERR_CODEC;
}

int OutputDataFilter::SetRawData(const ImageRGBA *img)
{
	(void)img;
	return EICL_ERR_CODEC;
}

inline void OutputDataFilter::RegisterCallback(OutputDataFilterCallback cb, void *userData)
{
	m_callback = cb;
	m_callbackData = userData;
}

inline int OutputDataFilter::CallCallback(int mode)
{
	if (!m_callback)
		return -1;
	return m_callback(mode, m_callbackData, (unsigned char *)0);
}
