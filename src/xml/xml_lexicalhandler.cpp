/*
 * Default (empty) methods of the SAX LexicalHandler interface.
 */

#include "xml_filedescriptor.h"
#include "xml_handlers2.h"

void LexicalHandler::comment(XMLString *, unsigned long, unsigned long)
{}
void LexicalHandler::endCDATA()
{}
void LexicalHandler::endDTD()
{}
void LexicalHandler::endEntity(XMLString *)
{}
void LexicalHandler::startCDATA()
{}
void LexicalHandler::startDTD(XMLString *, XMLString *, XMLString *)
{}
void LexicalHandler::startEntity(XMLString *)
{}
