/*
 * HTTP redirection: the Location headers of a response and the redirection
 * option of a connection.
 */

#include "http/scehttp.h"

/* Returns a NULL-terminated array with copies of the values of the
   "Location" headers (case-insensitive) of the response. */
char **sceHTTPParseLocations(struct sceHTTPResponse *resp)
{
	struct sceHTTPHeaderList *node;
	int count;
	char **arr;
	char **p;

	if (resp == 0)
		return 0;

	count = 0;
	for (node = resp->headers; node != 0; node = node->next) {
		if (sceHTTPLibStrcasecmp(node->name, "Location") == 0)
			count++;
	}
	if (count == 0)
		return 0;

	arr = xmalloc((count + 1) * 4);
	if (arr == 0)
		return 0;
	arr[count] = 0;

	p = arr;
	for (node = resp->headers; node != 0; node = node->next) {
		if (sceHTTPLibStrcasecmp(node->name, "Location") == 0) {
			*p = sceHTTPLibStrsave(node->value);
			p++;
		}
	}
	return arr;
}

/* Frees each uri, then the array. */
int sceHTTPFreeLocations(char **loc)
{
	char **p = loc;

	while (*p != 0) {
		p++;
		xfree(p[-1]);
	}
	xfree(loc);
	return 0;
}

/* Enables redirection (up to count times) through option 3, or 4 if flag
   is set. */
int sceHTTPSetRedirection(struct sceHTTPClient *conn, struct sceHTTPParsedURI *uri, int flag)
{
	int option_id;
	int result;

	if (conn == 0 || uri == 0) {
		conn->client_error = SCE_HTTP_ERR_INVALID;  /* NOTE: bug: conn may be NULL here */
		return -1;
	}
	option_id = flag ? 4 : 3;
	result = sceHTTPSetOption(conn, option_id, uri);
	if (result < 0)
		return -1;
	conn->redirect_flag = 1;
	return 0;
}
