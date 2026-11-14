/*
 * ErrorHandler: SAX error callback interface (pure virtual).
 */

#ifndef GUARD_XML_ERRORHANDLER_H
#define GUARD_XML_ERRORHANDLER_H

class SAXParseException;

class ErrorHandler {
public:
	virtual void error(SAXParseException &exc) = 0;
	virtual void fatalError(SAXParseException &exc) = 0;
	virtual void warning(SAXParseException &exc) = 0;
};

#endif // GUARD_XML_ERRORHANDLER_H
