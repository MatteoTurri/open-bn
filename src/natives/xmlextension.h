/*
 * xmlextension.cpp: XmlExtension, the XML natives and the `$$word$$` replacement table.
 */

#ifndef XMLEXTENSION_H
#define XMLEXTENSION_H

#include <string>

void XmlExtension_Init();
void XmlExtension_Exit();
void XmlExtension_UpdateReplaceWord();
void XmlExtension_Replace(string &str);

#endif
