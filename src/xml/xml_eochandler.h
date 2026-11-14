/*
 * EOCHandler: end-of-chunk callback of an XMLInputSource that is not final.
 */

#ifndef GUARD_XML_EOCHANDLER_H
#define GUARD_XML_EOCHANDLER_H
#include "xml_mmobject.h"

class EOCHandler : public XMLMMObject {
public:
	virtual void endOfChunk() = 0;
};

#endif // GUARD_XML_EOCHANDLER_H
