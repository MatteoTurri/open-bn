/*
 * HTTP authentication: Basic and Digest (RFC 2617) Authorization headers,
 * parsing of the WWW-Authenticate and Authentication-Info headers, and
 * verification of the server's rspauth.
 */

#include <stdio.h>
#include <string.h>

#include "http/scehttp.h"
#include "http/httpmd5.h"
#include "http/scebase64.h"

#define HTTP_SET_ERR(c, e) do { (c)->client_error = (e); } while (0)

/* Appends an "Authorization: Basic <base64(user:pass)>" header (or
   Proxy-Authorization) to the request headers. Returns 0, or -1 with
   client_error SCE_HTTP_ERR_INVALID (':' in user) or SCE_HTTP_ERR_NOMEM (no memory). */
int sceHTTPSetBasicAuth(struct sceHTTPClient *conn, char *user, char *pass, int proxy)
{
	char *plain;
	char *buf;
	int len;
	struct sceHTTPHeaderList *hdr;

	if (strchr(user, ':') != 0) {
		conn->client_error = SCE_HTTP_ERR_INVALID;
		return -1;
	}

	plain = sceHTTPLibVStrcat(0, user, ":", pass, 0);
	if (plain == 0) {
		conn->client_error = SCE_HTTP_ERR_NOMEM;
		return -1;
	}

	len = strlen(plain);
	buf = xmalloc(((len + 2) / 3) * 4 + 7);
	if (buf == 0) {
		HTTP_SET_ERR(conn, SCE_HTTP_ERR_NOMEM);
		xfree(plain);
		return -1;
	}

	memcpy(buf, "Basic ", 6);
	buf[sceBASE64Encoder(plain, buf + 6, len) + 6] = 0;
	xfree(plain);

	if (proxy != 0)
		hdr = sceHTTPAddHeaderList(0, "Proxy-Authorization", buf);
	else
		hdr = sceHTTPAddHeaderList(0, "Authorization", buf);
	xfree(buf);
	if (hdr == 0)
		return -1;
	conn->headers = sceHTTPLibCatHeaderList(conn->headers, hdr);
	return 0;
}

/* Writes 16 bytes as 32 lowercase hex digits, without a NUL. */
static void http_hex32(const unsigned char *in, char *out)
{
	unsigned int i;

	for (i = 0; i < 16; i++) {
		unsigned int v;

		v = in[i] >> 4;
		out[i * 2] = v < 10 ? v + '0' : v + ('a' - 10);
		v = in[i] & 0xf;
		out[i * 2 + 1] = v < 10 ? v + '0' : v + ('a' - 10);
	}
}

/* clang-format off */
static const char *digestMethod[8] = {
	"OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE", "CONNECT",
};
/* clang-format on */

/* Computes HA1 = MD5(username ":" realm ":" password) in hex; for MD5-sess
   (algorithm 2) MD5(HA1 ":" nonce ":" cnonce), hashing the binary HA1. */
static int digest_ha1(struct sceHTTPDigest *au, char *out)
{
	MD5_CTX ctx;
	unsigned char digest[16];

	if (au == 0)
		goto bad;
	if (out == 0)
		goto bad;
	if (au->username == 0)
		goto bad;
	if (au->password == 0)
		goto bad;
	if (au->realm == 0)
		goto bad;
	MD5Init(&ctx);
	MD5Update(&ctx, au->username, strlen(au->username));
	MD5Update(&ctx, ":", 1);
	MD5Update(&ctx, au->realm, strlen(au->realm));
	MD5Update(&ctx, ":", 1);
	MD5Update(&ctx, au->password, strlen(au->password));
	MD5Final(digest, &ctx);
	if (au->algorithm == 2) {
		if (au->nonce == 0)
			goto bad;
		if (au->cnonce == 0)
bad:
			return -1;
		MD5Init(&ctx);
		MD5Update(&ctx, digest, 16);
		MD5Update(&ctx, ":", 1);
		MD5Update(&ctx, au->nonce, strlen(au->nonce));
		MD5Update(&ctx, ":", 1);
		MD5Update(&ctx, au->cnonce, strlen(au->cnonce));
		MD5Final(digest, &ctx);
	}
	http_hex32(digest, out);
	return 0;
}

/* Writes the MD5 of data in hex. */
static int http_md5_hex(const void *data, unsigned int len, char *out)
{
	MD5_CTX ctx;
	unsigned char digest[16];

	if (data == 0 || out == 0)
		return -1;
	MD5Init(&ctx);
	MD5Update(&ctx, data, len);
	MD5Final(digest, &ctx);
	http_hex32(digest, out);
	return 0;
}

/* Computes the RFC 2617 response: HA2 = MD5(method ":" uri [":" ha2]),
   without the method when flags is set and with ha2 only for auth-int, then
   MD5(HA1 ":" nonce ":" [nc ":" cnonce ":" qop ":"] HA2) in hex. */
static int digest_response(struct sceHTTPDigest *au, char *ha1, char *ha2, char *out, int flags)
{
	MD5_CTX ctx;
	unsigned char digest[16];
	unsigned char resp[16];
	char hexa2[32];
	char ncbuf[9];
	const char *qop;

	if (au == 0)
		goto bad;
	if (ha1 == 0)
		goto bad;
	if (out == 0)
		goto bad;
	if (au->uri == 0)
		goto bad;
	if (au->nonce == 0)
		goto bad;
	if (au->method >= 8)
		goto bad;
	MD5Init(&ctx);
	if (flags == 0)
		MD5Update(&ctx, digestMethod[au->method], strlen(digestMethod[au->method]));
	MD5Update(&ctx, ":", 1);
	MD5Update(&ctx, au->uri, strlen(au->uri));
	if (au->qop == 2) {
		if (ha2 == 0)
			return -1;
		MD5Update(&ctx, ":", 1);
		MD5Update(&ctx, ha2, 32);
	}
	MD5Final(digest, &ctx);
	http_hex32(digest, hexa2);
	MD5Init(&ctx);
	MD5Update(&ctx, ha1, 32);
	MD5Update(&ctx, ":", 1);
	MD5Update(&ctx, au->nonce, strlen(au->nonce));
	MD5Update(&ctx, ":", 1);
	if (au->qop != 0) {
		if (au->cnonce == 0)
bad:
			return -1;
		qop = au->qop == 2 ? "auth-int" : "auth";
		sprintf(ncbuf, "%08x", au->nc);
		MD5Update(&ctx, ncbuf, 8);
		MD5Update(&ctx, ":", 1);
		MD5Update(&ctx, au->cnonce, strlen(au->cnonce));
		MD5Update(&ctx, ":", 1);
		MD5Update(&ctx, qop, strlen(qop));
		MD5Update(&ctx, ":", 1);
	}
	MD5Update(&ctx, hexa2, 32);
	MD5Final(resp, &ctx);
	http_hex32(resp, out);
	return 0;
}

/* Copies auth into a new node appended to list. Returns the node, or 0. */
static struct sceHTTPAuthList *http_append_auth(struct sceHTTPAuthList *list, const struct sceHTTPAuth *data)
{
	struct sceHTTPAuthList *n;

	n = (struct sceHTTPAuthList *)xcalloc(1, 72);
	if (n == 0)
		return 0;
	n->auth = *data;
	while (list != 0 && list->next != 0)
		list = list->next;
	n->prev = list;
	if (list != 0)
		list->next = n;
	return n;
}

/* Parses realm="..." at str into a new string in *out. Returns 0, or -1. */
static int http_parse_realm(char *str, char **out)
{
	char *end;
	char *tok;
	char *p;
	char *q;
	char c;
	int quote;
	char *val;

	if (str == 0)
		goto bad;
	if (*str == 0)
		goto bad;
	goto body;
bad:
	return -1;
body:
	end = str + strlen(str);
	str = sceHTTPLibSkipWS(str);
	tok = sceHTTPLibSkipToken(str);
	if (tok == str)
		return -1;
	if (tok == 0)
		return -1;

	c = *tok;
	*tok = 0;
	if (sceHTTPLibStrcasecmp(str, "realm") != 0)
		return -1;
	*tok = c;

	p = sceHTTPLibSkipWS(tok);
	if (*p != '=')
		return -1;
	p = sceHTTPLibSkipWS(p + 1);
	if (*p != '"')
		return -1;
	p++;

	quote = '"';
	q = end;
	if (q >= p) {
		do {
			if (*q == quote) {
				tok = q;
				goto found;
			}
			q--;
		} while (q >= p);
	}
	tok = 0;
found:
	if (tok == 0)
		goto bad;

	val = sceHTTPLibStrnsave(p, tok - p);
	if (val == 0)
		goto bad;
	*out = val;
	return 0;
}

/* Appends item to a NULL-terminated array, growing it to the next power of
   two. Returns the array, or 0. */
static void **http_ptr_array_add(void **list, void *item)
{
	void **p;
	void **nl;
	int n;
	int need;
	int cap;

	if (list == 0) {
		list = (void **)xmalloc(8);
		if (list == 0)
			return 0;
		list[0] = item;
		list[1] = 0;
	} else {
		n = 0;
		if (*list != 0) {
			p = list;
			do {
				p++;
				n++;
			} while (*p != 0);
		}
		need = n + 1;
		cap = 2;
		while (need >= cap)
			cap <<= 1;
		nl = (void **)xrealloc(list, cap * 4);
		if (nl == 0)
			return 0;
		list = nl;
		list[n] = item;
		list[n + 1] = 0;
	}
	return list;
}

/* Frees a NULL-terminated array of strings and the array itself. */
static void http_free_auth_data(void *data)
{
	void **list = (void **)data;
	void **p;

	if (*list != 0) {
		p = list;
		do {
			xfree(*p);
			p++;
		} while (*p != 0);
	}
	xfree(list);
}

/* Returns the last occurrence of c in [s, e], searching back from e. */
static __inline__ char *http_rfind(char *s, char *e, int c)
{
	for (; e >= s; e--)
		if (*e == c)
			return e;
	return 0;
}

/* Parses the parameters of a Digest challenge (realm, nonce, opaque, stale,
   algorithm, qop, domain); only the first occurrence of each counts, unknown
   ones are skipped. Returns 0, or -1 freeing what was copied. */
static int http_parse_digest_body(char *p, char **realm_out, char **nonce_out, char **opaque_out, void ***domain_out,
    int *stale_out, int *algorithm_out, int *qop_out)
{
	char *q, *semi, *t;
	char c;
	int qop;
	char *end;
	char *realm, *nonce, *opaque;
	void **domain;
	int stale, algorithm;
	char *v;

	if (p == 0 || *p == 0)
		return -1;
	qop = 0;
	opaque = 0;
	nonce = 0;
	realm = 0;
	domain = 0;
	algorithm = 0;
	stale = 0;
	end = p + strlen(p);
	while (*p != 0) {
		p = sceHTTPLibSkipWS(p);
		q = sceHTTPLibSkipToken(p);
		if (q == p || q == 0)
			goto fail;
		c = *q;
		*q = 0;
		t = strchr(q + 1, ',');
		semi = t ? t : end;
		if (sceHTTPLibStrcasecmp(p, "realm") == 0 && realm == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto fail;
			p = sceHTTPLibSkipWS(p + 1);
			if (*p != '"')
				goto fail;
			p++;
			q = http_rfind(p, semi, '"');
			if (q == 0)
				goto fail;
			if ((realm = sceHTTPLibStrnsave(p, q - p)) != 0)
				goto next;
			goto fail;
		} else if (sceHTTPLibStrcasecmp(p, "nonce") == 0 && nonce == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto fail;
			p = sceHTTPLibSkipWS(p + 1);
			if (*p != '"')
				goto fail;
			p++;
			q = http_rfind(p, semi, '"');
			if (q == 0)
				goto fail;
			if ((nonce = sceHTTPLibStrnsave(p, q - p)) != 0)
				goto next;
			goto fail;
		} else if (sceHTTPLibStrcasecmp(p, "opaque") == 0 && opaque == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto fail;
			p = sceHTTPLibSkipWS(p + 1);
			if (*p != '"')
				goto fail;
			p++;
			q = http_rfind(p, semi, '"');
			if (q == 0)
				goto fail;
			if ((opaque = sceHTTPLibStrnsave(p, q - p)) != 0)
				goto next;
			goto fail;
		} else if (sceHTTPLibStrcasecmp(p, "stale") == 0 && stale == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto fail;
			p = sceHTTPLibSkipWS(p + 1);
			q = sceHTTPLibSkipToken(p);
			if (q == p || q == 0)
				goto fail;
			c = *q;
			*q = 0;
			if (sceHTTPLibStrcasecmp(p, "true") == 0)
				stale = 1;
			else if (sceHTTPLibStrcasecmp(p, "false") == 0)
				stale = 2;
			else
				stale = 0;
			*q = c;
		} else if (sceHTTPLibStrcasecmp(p, "algorithm") == 0 && algorithm == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto fail;
			p = sceHTTPLibSkipWS(p + 1);
			q = sceHTTPLibSkipToken(p);
			if (q == p || q == 0)
				goto fail;
			c = *q;
			*q = 0;
			if (sceHTTPLibStrcasecmp(p, "MD5") == 0)
				algorithm = 1;
			else if (sceHTTPLibStrcasecmp(p, "MD5-sess") == 0)
				algorithm = 2;
			else
				algorithm = 0;
			*q = c;
		} else if (sceHTTPLibStrcasecmp(p, "qop") == 0 && qop == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto fail;
			p = sceHTTPLibSkipWS(p + 1);
			if (*p != '"')
				goto fail;
			p++;
			do {
				q = sceHTTPLibSkipToken(p);
				if (q == p || q == 0)
					goto fail;
				c = *q;
				*q = 0;
				if (sceHTTPLibStrcasecmp(p, "auth") == 0)
					qop |= 1;
				else if (sceHTTPLibStrcasecmp(p, "auth-int") == 0)
					qop |= 2;
				*q = c;
				q = sceHTTPLibSkipWS(q);
				p = sceHTTPLibSkipWS(q + 1);
			} while (*q == ',');
			if (*q != '"')
				goto fail;
			t = strchr(q + 1, ',');
			semi = t ? t : end;
		} else if (sceHTTPLibStrcasecmp(p, "domain") == 0 && domain == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto fail;
			p = sceHTTPLibSkipWS(p + 1);
			if (*p != '"')
				goto fail;
			p++;
			do {
				void **nd;
				char *e;

				e = sceHTTPLibSkipToken(p);
				while (*e == '/' || *e == '@')
					e = sceHTTPLibSkipToken(e + 1);
				v = 0;
				if (e != p) {
					if (e == 0)
						goto fail;
					if ((v = sceHTTPLibStrnsave(p, e - p)) == 0)
						goto fail;
				}
				nd = http_ptr_array_add(domain, v);
				if (nd == 0)
					goto fail;
				domain = nd;
				p = sceHTTPLibSkipWS(e);
			} while (*p != '"');
		}
next:
		p = semi + (*semi == ',');
	}
	*realm_out = realm;
	*nonce_out = nonce;
	*opaque_out = opaque;
	*domain_out = domain;
	*stale_out = stale;
	*algorithm_out = algorithm;
	*qop_out = qop;
	return 0;

fail:
	if (realm != 0)
		xfree(realm);
	if (nonce != 0)
		xfree(nonce);
	if (opaque != 0)
		xfree(opaque);
	if (domain != 0)
		http_free_auth_data(domain);
	return -1;
}

/* Returns the value of a lowercase hex digit, -1 if c is not one. */
static __inline__ int http_hexval(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return -1;
}

#define HTTP_SKIP_EQ(p, q, c) do { \
	*(q) = (c); \
	(p) = sceHTTPLibSkipWS(q); \
	if (*(p) != '=') \
		goto fail; \
	(p) = sceHTTPLibSkipWS((p) + 1); \
} while (0)

/* Parses the parameters of an Authentication-Info header (nextnonce,
   cnonce, rspauth, qop, nc). Returns 0, or -1 freeing what was copied. */
static int http_parse_authinfo_value(char *p, char **nextnonce_out, char **rspauth_out, char **cnonce_out, int *nc_out,
    int *qop_out)
{
	char *q, *semi, *t;
	char c;
	char *end;
	char *rspauth, *cnonce, *nextnonce;
	int qop;
	int nc;
	int i, d;

	if (p == 0 || *p == 0)
		return -1;
	rspauth = 0;
	cnonce = 0;
	nextnonce = 0;
	qop = 0;
	nc = 0;
	end = p + strlen(p);
	while (*p != 0) {
		p = sceHTTPLibSkipWS(p);
		q = sceHTTPLibSkipToken(p);
		if (q == p || q == 0)
			goto fail;
		c = *q;
		*q = 0;
		t = strchr(q + 1, ',');
		semi = t ? t : end;
		if (sceHTTPLibStrcasecmp(p, "nextnonce") == 0 && nextnonce == 0) {
			HTTP_SKIP_EQ(p, q, c);
			if (*p != '"')
				goto fail;
			p++;
			q = http_rfind(p, semi, '"');
			if (q == 0)
				goto fail;
			if ((nextnonce = sceHTTPLibStrnsave(p, q - p)) != 0)
				goto next;
			goto fail;
		} else if (sceHTTPLibStrcasecmp(p, "cnonce") == 0 && cnonce == 0) {
			HTTP_SKIP_EQ(p, q, c);
			if (*p != '"')
				goto fail;
			p++;
			q = http_rfind(p, semi, '"');
			if (q == 0)
				goto fail;
			if ((cnonce = sceHTTPLibStrnsave(p, q - p)) != 0)
				goto next;
			goto fail;
		} else if (sceHTTPLibStrcasecmp(p, "rspauth") == 0 && rspauth == 0) {
			HTTP_SKIP_EQ(p, q, c);
			if (*p != '"')
				goto fail;
			p++;
			q = http_rfind(p, semi, '"');
			if (q == 0)
				goto fail;
			if ((rspauth = sceHTTPLibStrnsave(p, q - p)) != 0)
				goto next;
			goto fail;
		} else if (sceHTTPLibStrcasecmp(p, "qop") == 0 && qop == 0) {
			HTTP_SKIP_EQ(p, q, c);
			q = sceHTTPLibSkipToken(p);
			if (q == p || q == 0)
				goto fail;
			c = *q;
			*q = 0;
			if (sceHTTPLibStrcasecmp(p, "auth") == 0)
				qop = 1;
			else if (sceHTTPLibStrcasecmp(p, "auth-int") == 0)
				qop = 2;
			else
				qop = 0;
			*q = c;
		} else if (sceHTTPLibStrcasecmp(p, "nc") == 0) {
			HTTP_SKIP_EQ(p, q, c);
			q = sceHTTPLibSkipToken(p);
			if (q == 0 || q - p != 8)
				goto fail;
			nc = 0;
			c = *q;
			*q = 0;
			for (i = 0; i < 8; i++) {
				d = http_hexval(p[i]);
				if (d < 0)
					goto fail;
				nc = nc * 16 + d;
			}
			*q = c;
		}
next:
		p = semi + (*semi == ',');
	}
	*nextnonce_out = nextnonce;
	*cnonce_out = cnonce;
	*rspauth_out = rspauth;
	*nc_out = nc;
	*qop_out = qop;
	return 0;

fail:
	if (nextnonce != 0)
		xfree(nextnonce);
	if (cnonce != 0)
		xfree(cnonce);
	if (rspauth != 0)
		xfree(rspauth);
	return -1;
}

int sceHTTPFreeAuthList(struct sceHTTPAuthList *node)
{
	struct sceHTTPAuthList *next;

	while (node != 0) {
		next = node->next;
		if (node->auth.realm)
			xfree(node->auth.realm);
		if (node->auth.nonce)
			xfree(node->auth.nonce);
		if (node->auth.opaque)
			xfree(node->auth.opaque);
		if (node->auth.domain)
			http_free_auth_data(node->auth.domain);
		xfree(node);
		node = next;
	}
	return 0;
}

/* Returns a list with the Basic and Digest challenges of the
   WWW-Authenticate and Proxy-Authenticate headers of resp, or 0. */
struct sceHTTPAuthList *sceHTTPParseAuth(struct sceHTTPResponse *resp)
{
	void *list = 0;
	void *newlist;
	struct sceHTTPHeaderList *h;
	struct sceHTTPAuth auth;
	int proxy;
	char *realm;
	char *nonce;
	char *opaque;
	void **domain;
	int stale;
	int algorithm;
	int qop;

	if (resp == 0)
		return 0;
	h = resp->headers;
	if (h == 0)
		goto done;

	while (h != 0) {
		if (sceHTTPLibStrcasecmp(h->name, "WWW-Authenticate") == 0
		    || sceHTTPLibStrcasecmp(h->name, "Proxy-Authenticate") == 0) {
			proxy = sceHTTPLibStrcasecmp(h->name, "Proxy-Authenticate") == 0;
			if (sceHTTPLibStrcasesub(h->value, "Basic") == 0) {
				memset(&auth, 0, sizeof(auth));
				auth.type = 0;
				auth.proxy = proxy;
				if (http_parse_realm(h->value + 5, &auth.realm) != 0)
					goto fail;
				newlist = http_append_auth((struct sceHTTPAuthList *)list, &auth);
				if (newlist == 0)
					goto fail;
				list = newlist;
			} else if (sceHTTPLibStrcasesub(h->value, "Digest") == 0) {
				if (http_parse_digest_body(h->value + 6, &realm, &nonce, &opaque, &domain, &stale,
				        &algorithm, &qop)
				    != 0)
					goto fail;
				auth.type = 1;
				auth.proxy = proxy;
				auth.realm = realm;
				auth.nonce = nonce;
				auth.opaque = opaque;
				auth.domain = domain;
				auth.stale = stale;
				auth.algorithm = algorithm;
				auth.qop = qop;
				newlist = http_append_auth((struct sceHTTPAuthList *)list, &auth);
				if (newlist == 0)
					goto fail;
				list = newlist;
			}
		}
		h = h->next;
	}
done:
	return list;
fail:
	if (list != 0)
		sceHTTPFreeAuthList(list);
	return 0;
}

/* Name tables of sceHTTPSetDigestAuth, indexed by value - 1. */
static const char *digestAlg[2] = { "MD5", "MD5-sess" };
static const char *digestQop[2] = { "auth", "auth-int" };

/* Appends an "Authorization: Digest ..." header (or Proxy-Authorization)
   built from au to the request headers. Returns 0, or -1 with client_error
   set. */
int sceHTTPSetDigestAuth(struct sceHTTPClient *conn, struct sceHTTPDigest *au, int proxy)
{
	char *str;
	char *tmp;
	struct sceHTTPHeaderList *hdr;
	char ha1[32];
	char ha2[32];
	char resp[33];
	char ncbuf[9];

	if (au->username == 0 || au->realm == 0 || au->password == 0 || au->nonce == 0 || au->uri == 0) {
		HTTP_SET_ERR(conn, SCE_HTTP_ERR_INVALID);
		return -1;
	}
	if (au->qop >= 3 || au->algorithm >= 3) {
		HTTP_SET_ERR(conn, SCE_HTTP_ERR_INVALID);
		return -1;
	}

	str = sceHTTPLibVStrcat(0, "Digest username=\"", au->username, "\",", " realm=\"", au->realm, "\",",
	    " nonce=\"", au->nonce, "\",", " uri=\"", au->uri, "\",", 0);
	if (str == 0) {
		HTTP_SET_ERR(conn, SCE_HTTP_ERR_NOMEM);
		return -1;
	}

	if (au->algorithm != 0) {
		tmp = sceHTTPLibVStrcat(str, " algorithm=", digestAlg[au->algorithm - 1], ",", 0);
		if (tmp == 0) {
			HTTP_SET_ERR(conn, SCE_HTTP_ERR_NOMEM);
			goto freeret;
		}
		str = tmp;
	}
	if (au->qop != 0) {
		tmp = sceHTTPLibVStrcat(str, " qop=", digestQop[au->qop - 1], ",", 0);
		if (tmp == 0) {
			HTTP_SET_ERR(conn, SCE_HTTP_ERR_NOMEM);
			goto freeret;
		}
		str = tmp;
	}
	if (au->nc > 0) {
		sprintf(ncbuf, "%08x", au->nc);
		ncbuf[8] = 0;
		tmp = sceHTTPLibVStrcat(str, " nc=", ncbuf, ",", 0);
		if (tmp == 0) {
			HTTP_SET_ERR(conn, SCE_HTTP_ERR_NOMEM);
			goto freeret;
		}
		str = tmp;
	}
	if (au->cnonce != 0) {
		tmp = sceHTTPLibVStrcat(str, " cnonce=\"", au->cnonce, "\",", 0);
		if (tmp == 0) {
			HTTP_SET_ERR(conn, SCE_HTTP_ERR_NOMEM);
			goto freeret;
		}
		str = tmp;
	}
	if (au->opaque != 0) {
		tmp = sceHTTPLibVStrcat(str, " opaque=\"", au->opaque, "\",", 0);
		if (tmp == 0) {
			HTTP_SET_ERR(conn, SCE_HTTP_ERR_NOMEM);
			goto freeret;
		}
		str = tmp;
	}

	digest_ha1(au, ha1);
	if (au->qop == 2)
		http_md5_hex(au->entity, au->entity_len, ha2);
	if (digest_response(au, ha1, ha2, resp, 0) != 0) {
		HTTP_SET_ERR(conn, SCE_HTTP_ERR_INVALID);
		goto freeret;
	}
	resp[32] = 0;
	tmp = sceHTTPLibVStrcat(str, " response=\"", resp, "\"", 0);
	if (tmp == 0) {
		HTTP_SET_ERR(conn, SCE_HTTP_ERR_NOMEM);
		goto freeret;
	}
	str = tmp;
	if (proxy != 0)
		hdr = sceHTTPAddHeaderList(0, "Proxy-Authorization", str);
	else
		hdr = sceHTTPAddHeaderList(0, "Authorization", str);
	xfree(tmp);
	if (hdr == 0)
		return -1;
	conn->headers = sceHTTPLibCatHeaderList(conn->headers, hdr);
	return 0;
freeret:
	if (str != 0)
		xfree(str);
	return -1;
}

/* Returns the parsed Authentication-Info (or Proxy-Authenticate-Info)
   header of resp, or 0. */
struct sceHTTPAuthInfo *sceHTTPParseAuthInfo(struct sceHTTPResponse *resp)
{
	struct sceHTTPAuthInfo *info = 0;
	struct sceHTTPHeaderList *h;
	char *a0_;
	char *a1_;
	char *a2_;
	int a3_;
	int a4_;

	if (resp == 0)
		return 0;

	h = resp->headers;
	while (h != 0) {
		if (sceHTTPLibStrcasecmp(h->name, "Authentication-Info") == 0
		    || sceHTTPLibStrcasecmp(h->name, "Proxy-Authenticate-Info") == 0) {
			info = xcalloc(1, sizeof(struct sceHTTPAuthInfo));
			if (http_parse_authinfo_value(h->value, &a0_, &a1_, &a2_, &a3_, &a4_) != 0)
				goto fail;
			info->nextnonce = a0_;
			info->cnonce = a2_;
			info->rspauth = a1_;
			info->nc = a3_;
			info->qop = a4_;
			if (sceHTTPLibStrcasecmp(h->name, "Proxy-Authenticate-Info") == 0)
				info->proxy = 1;
			else
				info->proxy = 0;
			break;
		}
		h = h->next;
	}
	return info;
fail:
	if (info != 0)
		xfree(info);
	return 0;
}

/* Checks the rspauth computed by the server against au. Returns 1 if it
   matches or there is nothing to check, 0 if it does not, -1 with
   client_error SCE_HTTP_ERR_INVALID on bad arguments. */
int sceHTTPVerifyAuthInfo(struct sceHTTPClient *conn, struct sceHTTPAuthInfo *info, struct sceHTTPDigest *au, int flags)
{
	char ha1[32];
	char ha2[32];
	char resp[33];
	int ok;

	if (conn == 0 || info == 0 || au == 0)
		goto fail;
	if ((info->qop & 3) == 0)
		return 0;
	if (info->nc != au->nc && strcmp(info->cnonce, au->cnonce) != 0)
		return 0;
	if ((flags & 1) == 0)
		return 1;
	if (info->qop == 2 && (conn->out_fd != -1 || au->entity == 0))
		return 1;
	digest_ha1(au, ha1);
	if (info->qop == 2)
		http_md5_hex(au->entity, au->entity_len, ha2);
	ok = 1;
	if (digest_response(au, ha1, ha2, resp, ok) != 0) {
		conn->client_error = SCE_HTTP_ERR_INVALID;
		goto fail;
	}
	resp[32] = 0;
	if (strcmp(info->rspauth, resp) != 0)
		ok = 0;
	return ok;
fail:
	HTTP_SET_ERR(conn, SCE_HTTP_ERR_INVALID);
	return -1;
}

/* Frees an auth info; returns -1 if p is NULL. */
int sceHTTPFreeAuthInfo(struct sceHTTPAuthInfo *p)
{

	if (p == 0)
		return -1;
	if (p->nextnonce)
		xfree(p->nextnonce);
	if (p->cnonce)
		xfree(p->cnonce);
	if (p->rspauth)
		xfree(p->rspauth);
	xfree(p);
	return 0;
}
