/*
 * CreateXMLParser and DestroyXMLParser: the public factory of the XML parser.
 */

#include "xml_parserfactory.h"
#include "xml_inputsource.h"

XMLDocumentScanner *CreateXMLParser()
{
	return new XMLDocumentScanner();
}

void DestroyXMLParser(XMLDocumentScanner *parser)
{
	if (parser)
		delete parser;
}
