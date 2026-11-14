/*
 * OutputDataFilter: base class of the image encoders, writing through an
 * OutputIoFilter. The subclasses set m_ioFilter directly in their constructors.
 */

#ifndef OUTPUTDATAFILTER_H
#define OUTPUTDATAFILTER_H

#include "inputdatafilter.h" /* RawImg, ImageRGBA, tagRECT, PgProps */

class OutputIoFilter;

typedef int (*OutputDataFilterCallback)(int, void *, unsigned char *);

class OutputDataFilter {
public:
	OutputDataFilter();
	OutputDataFilter(OutputIoFilter *ioFilter);

	virtual int SetRawData(const RawImg *img);
	virtual int SetRawData(const ImageRGBA *img);
	virtual int SetProps(PgProps *props);
	virtual void RegisterCallback(OutputDataFilterCallback cb, void *userData);
	virtual int CallCallback(int mode);

	virtual ~OutputDataFilter();

	void InitializeVariables();
	OutputIoFilter *GetOutputIoFilter();

private:
	/* 0x00 */ OutputDataFilterCallback m_callback;
	/* 0x04 */ void *m_callbackData;

protected:
	/* 0x08 */ OutputIoFilter *m_ioFilter;

private:
	/* 0x0C */ PgProps *m_props;
	/* 0x10: vtable pointer */  /* size = 0x14 */
};

#endif
