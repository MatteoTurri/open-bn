/*
 * XmlParse namespace: module init/exit and the DOM parse driver.
 */

#include <pthread.h>
#include "xml_parserfactory.h"
#include "xml_domunit.h"
#include "xmlparse_ns.h"

namespace XmlParse {

static pthread_mutex_t g_mutex;

void Init()
{
	pthread_mutex_init(&g_mutex, 0);
}

/* Empty: the mutex is never destroyed. */
void Exit()
{}

/* Parses an XML string into a DOM Document under the module mutex; returns an
   empty Document when parsing fails. */
Document ParseString(const char *xml)
{
	Document doc(0);
	DOMUnit *unit;
	XMLDocumentScanner *parser;

	pthread_mutex_lock(&g_mutex);
	unit = new DOMUnit();
	parser = CreateXMLParser();
	parser->setDocumentHandler(unit);
	parser->setDTDHandler(unit);
	parser->setEntityResolver(unit);
	parser->setErrorHandler(unit);
	parser->setDeclHandler(unit);
	parser->setLexicalHandler(unit);
	unit->InternalCheck(false);
	parser->parse((char *)xml, XMLIOType_Memory, XMLCode_None);
	unit->InternalCheck(true);
	if (unit->Status().Success())
		doc = unit->getDocument();
	delete unit;
	DestroyXMLParser(parser);
	pthread_mutex_unlock(&g_mutex);
	return doc;
}

}
