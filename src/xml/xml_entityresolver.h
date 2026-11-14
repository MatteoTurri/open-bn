/*
 * EntityResolver: SAX entity resolution interface (pure virtual).
 */

#ifndef GUARD_XML_ENTITYRESOLVER_H
#define GUARD_XML_ENTITYRESOLVER_H

class XMLString;
class InputSource;

class EntityResolver {
public:
	virtual ~EntityResolver() {}
	virtual InputSource *resolveEntity(XMLString *publicId, XMLString *systemId) = 0;
};

#endif // GUARD_XML_ENTITYRESOLVER_H
