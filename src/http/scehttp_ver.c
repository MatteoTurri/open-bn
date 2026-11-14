/*
 * Version string of the HTTP library.
 */

#include "http/scehttp.h"

char *sceHTTPLibVersion = "1.2.42";

char *sceHTTPGetLibVersion(void)
{
	return sceHTTPLibVersion;
}
