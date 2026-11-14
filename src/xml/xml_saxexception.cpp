/*
 * SAXException: an exception that owns a copy of its message.
 */

#include "xml_filedescriptor.h"
#pragma implementation "xml_saxexception.h"
#include "xml_saxexception.h"

SAXException SAXException::getException()
{
	return *this;
}

XMLString *SAXException::getMessage()
{
	return fMessage;
}

SAXException::SAXException()
{
	fMessage = 0;
}

SAXException::SAXException(SAXException &other)
{
	if (other.fMessage)
		fMessage = other.fMessage->Clone();
	else
		fMessage = 0;
}

SAXException::SAXException(XMLString *message)
{
	if (message)
		fMessage = message->Clone();
	else
		fMessage = 0;
}

/* Never implemented: the body is empty. */
SAXException::SAXException(XMLString *systemId, SAXException other)
{}

SAXException::~SAXException()
{
	delete fMessage;
}

SAXException &SAXException::operator=(SAXException &other)
{
	fMessage = other.fMessage->Clone();
	return *this;
}
