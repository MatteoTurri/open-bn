/*
 * SScanner: scanner of the white space between constructs.
 */

#include "xml_tokenscanner.h"
#include "xml_scanners.h"

SScanner::SScanner(XMLDocumentScanner *ds) : TokenScanner(ds)
{}

SScanner::~SScanner()
{}

/* Returns the result of XMLInputStream::SkipWhiteSpace. */
int SScanner::Scan(XMLInputStream *stream, XMLStreamManager *mgr)
{
	return stream->SkipWhiteSpace();
}
