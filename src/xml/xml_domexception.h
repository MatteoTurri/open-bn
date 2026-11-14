/*
 * DOMException: DOM exception carrying an ExceptionCode.
 */

#ifndef GUARD_XML_DOMEXCEPTION_H
#define GUARD_XML_DOMEXCEPTION_H
#pragma interface

class DOMException {
public:
	/* The exception codes of the DOM specification. */
	enum ExceptionCode {
		NO_ERR,
		INDEX_SIZE_ERR,
		DOMSTRING_SIZE_ERR,
		HIERARCHY_REQUEST_ERR,
		WRONG_DOCUMENT_ERR,
		INVALID_CHARACTER_ERR,
		NO_DATA_ALLOWED_ERR,
		NO_MODIFICATION_ALLOWED_ERR,
		NOT_FOUND_ERR,
		NOT_SUPPORTED_ERR,
		INUSE_ATTRIBUTE_ERR,
		INVALID_STATE_ERR,
		SYNTAX_ERR,
		INVALID_MODIFICATION_ERR,
		NAMESPACE_ERR,
		INVALID_ACCESS_ERR
	};
	DOMException();
	DOMException(ExceptionCode code);
	DOMException(const DOMException &other);
	virtual ~DOMException();
	ExceptionCode get_code() const;
	void set_code(ExceptionCode code);

private:
	/* 0x00 */ ExceptionCode fCode;
};

#endif // GUARD_XML_DOMEXCEPTION_H
