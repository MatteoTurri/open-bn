/*
 * DeclManager: empty common base of EntityDeclManager and AttListDeclManager.
 */

#ifndef GUARD_XML_DECLMANAGER_H
#define GUARD_XML_DECLMANAGER_H
#include "xml_mmobject.h"

class DeclManager : public XMLMMObject {
public:
	virtual ~DeclManager() {}
};

#endif // GUARD_XML_DECLMANAGER_H
