/*
 * DOMImplementation: reference-counted DOM handle, with DOMImplementationImpl.
 */

#ifndef GUARD_XML_DOMIMPL_H
#define GUARD_XML_DOMIMPL_H
#include "xml_gc.h"
#include "xml_domimplobject.h"

class DOMImplementationImpl : public DOMImplObject {
public:
	DOMImplementationImpl();
	virtual ~DOMImplementationImpl();
};

class DOMImplementation {
public:
	DOMImplementation();
	DOMImplementation(DOMImplementationImpl *impl);
	DOMImplementation(const DOMImplementation &other);
	virtual ~DOMImplementation();

	DOMImplementation &operator=(const DOMImplementation &other);
	DOMImplementation &operator=(const int *other);
	bool operator==(const DOMImplementation &other) const;
	bool operator!=(const DOMImplementation &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;

protected:
	DOMImplementationImpl *fImpl;
};

#endif // GUARD_XML_DOMIMPL_H
