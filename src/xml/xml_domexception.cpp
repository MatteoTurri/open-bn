/*
 * DOMException: an exception that carries one DOM error code.
 */

#pragma implementation "xml_domexception.h"
#include "xml_domexception.h"

DOMException::DOMException()
{
	fCode = NO_ERR;
}

DOMException::DOMException(ExceptionCode code)
{
	fCode = code;
}

DOMException::DOMException(const DOMException &other)
{
	fCode = other.fCode;
}

DOMException::~DOMException()
{}

DOMException::ExceptionCode DOMException::get_code() const
{
	return fCode;
}

void DOMException::set_code(ExceptionCode code)
{
	fCode = code;
}
