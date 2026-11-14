/*
 * Constructor and static member of TokenScanner.
 */

#include "xml_scanner.h"
#include "xml_tokenscanner.h"
#include "xml_entitydeclmanager.h"

int TokenScanner::PEReference = 0;

/* Stores the document scanner and borrows its error processor. */
TokenScanner::TokenScanner(XMLDocumentScanner *ds)
{
	fDocScanner = ds;
	if (ds)
		fErrorProcessor = ds->GetErrorProcessor();
	else
		fErrorProcessor = 0;
}
