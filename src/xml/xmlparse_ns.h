/*
 * xmlparse_ns.cpp: XmlParse, module init/exit and the DOM parse driver.
 */

#ifndef XMLPARSE_NS_H
#define XMLPARSE_NS_H

#include "xml_document.h"

namespace XmlParse {
void Init();
void Exit();
Document ParseString(const char *xml);
}

#endif
