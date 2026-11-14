/*
 * Default (empty) methods of the SAX DeclHandler interface.
 */

#include "xml_filedescriptor.h"
#include "xml_handlers2.h"

void DeclHandler::attributeDecl(XMLString *, XMLString *, XMLString *, XMLString *, XMLString *)
{}
void DeclHandler::elementDecl(XMLString *, XMLString *)
{}
void DeclHandler::externalEntityDecl(XMLString *, XMLString *, XMLString *)
{}
void DeclHandler::internalEntityDecl(XMLString *, XMLString *)
{}
