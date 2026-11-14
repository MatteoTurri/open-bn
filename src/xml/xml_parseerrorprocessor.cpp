/*
 * Methods of ParseErrorProcessor. The message of an error is its code in decimal.
 */

#include "xml_filedescriptor.h"
#include "xml_saxparseexception.h"
#include "xml_errorhandler.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#include "xml_parseerrorprocessor.h"
#include <stdio.h>

ParseErrorProcessor::ParseErrorProcessor()
{
	fErrorHandler = 0;
}

ParseErrorProcessor::~ParseErrorProcessor()
{}

void ParseErrorProcessor::SetErrorHandler(ErrorHandler *eh)
{
	fErrorHandler = eh;
}

XMLString ParseErrorProcessor::GetErrorString(XMLParseErrorState state)
{
	char buf[16];
	sprintf(buf, "%ld", state);
	XMLString temp(buf, false);
	return temp;
}

static inline XMLString makeErrorString(int state)
{
	char buf[16];
	sprintf(buf, "%ld", state);
	XMLString t(buf, false);
	return t;
}

/* Reports the error to fErrorHandler->fatalError as a SAXParseException. */
void ParseErrorProcessor::TreatErrorState(XMLParseErrorState state, XMLInputStream *stream)
{
	Locator *locator = 0;
	if (stream)
		locator = stream->GetLocator();

	XMLString errorMsg = makeErrorString(state);

	if (state & 0x8000) {  /* redundant as written */
		SAXParseException exc(&errorMsg, locator);
		if (fErrorHandler)
			fErrorHandler->fatalError(exc);
	} else {
		SAXParseException exc(&errorMsg, locator);
		if (fErrorHandler)
			fErrorHandler->fatalError(exc);
	}
}
