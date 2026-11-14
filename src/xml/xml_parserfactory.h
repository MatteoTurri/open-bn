/*
 * CreateXMLParser and DestroyXMLParser, with the SAX classes their callers need.
 */

#ifndef GUARD_XML_PARSERFACTORY_H
#define GUARD_XML_PARSERFACTORY_H
#include "xml_parser.h"
#include "sax.h"
#include "xml_scanner.h"

extern "C" XMLDocumentScanner *CreateXMLParser();
extern "C" void DestroyXMLParser(XMLDocumentScanner *parser);

#endif // GUARD_XML_PARSERFACTORY_H
