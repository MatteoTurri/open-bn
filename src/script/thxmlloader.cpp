/*
 * thXmlLoader: loads a PSML document and its script modules in a thread, with
 * HTTP authentication, script key handling and the XML cache ramdisk; also
 * GetFullPath, the path/URL resolver of PSML pages.
 */

#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include "http/httpfile.h"
#include "xml/xml_filedescriptor.h"
#include "thxmlloader.h"

#include "ui/navimenu.h"

#include "natives/hisextension.h"

#include "app/systemparam.h"

#include "app/stringutil.h"
#include "app/bnsysconf.h"
#include "http/sceurl.h"
#include <string.h>
#include "fs/bbfile.h"
#include <fcntl.h>
#include <unistd.h>
#include <list>
#include "xml/xml_nodelist.h"
#include "app/systemoptions.h"
#include "app/variables.h"
#include "xml/xmlparse_ns.h"
#include "script.h"
#include "dnas/dnas2m60_verify.h"
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>

/* The last n characters of s. */
static inline StringUtil StrRight(const StringUtil &s, unsigned int n)
{
	return s.substr(s.length() - n, n);
}

/* Trim of the two ends of a string, in place. */
static inline StringUtil authlist_mid(const string &s, int pos, int n)
{
	return s.substr(pos, n);
}

static inline void trim_left(string &t, const char *ws = " \t\r\n")
{
	int pos = t.find_first_not_of(ws);
	if (pos < 0) {
		t = "";
	} else {
		t = authlist_mid(t, pos, t.length() - pos);
	}
}

static inline void trim_right(string &t, const char *ws = " \t\r\n")
{
	int pos = t.find_last_not_of(ws);
	if (pos < 0) {
		t = "";
	} else {
		t = authlist_mid(t, 0, pos + 1);
	}
}

/* PSML versions: the rejected ones (none) and the accepted ones. */
struct psml_version_entry {
	const char *name;
	int version;
};

static psml_version_entry psml_rejected_versions[] = {
	{ 0, 0 },
};

static psml_version_entry psml_known_versions[] = {
	{ "1.1", 110 },
	{ "1.0", 100 },
	{ 0, 0 },
};

int thXmlLoader::for_modal_window_count = 0;
int thXmlLoader::for_cryption_state = 0;
int thXmlLoader::instance_count = 0;

/* URI predicates. */

static bool isNetworkUri(const char *s)
{
	return strncasecmp(s, "http:", 5) == 0 || strncasecmp(s, "https:", 6) == 0;
}

static bool isPfsUri(const char *s)
{
	return strncasecmp(s, "pfs:", 4) == 0;
}

static bool isCdromPath(const char *s)
{
	return strncasecmp(s, "file:/mnt/cdrom/", 16) == 0 || strncasecmp(s, "/mnt/cdrom/", 11) == 0;
}

/* Resolves path against the directory dir: keeps absolute URIs (file:,
   http:, https:, "$$" paths), collapses "/./" and "/../". The rules differ
   for PSML 1.1 (version > 100) and 1.0. */
string GetFullPath(const string &dir, const string &path, int psmlVersion)
{
	if (psmlVersion > 100) {
		string full = dir;
		string p = path;
		unsigned int pos;
		unsigned int root;

		if (path.length() == 0)
			return path;

		if (path.find_first_of(":") != string::npos) {
			full = path;
		} else if (strncmp(path.c_str(), "$$", 2) == 0) {
			full = path;
		} else {
			if (p[0] == '/') {
				if (strncasecmp(full.c_str(), "file:/", 6) == 0) {
					full = "file:";
				} else if (strncasecmp(full.c_str(), "http://", 7) == 0) {
					pos = full.find("/", 7);
					if (pos != string::npos)
						full = full.substr(0, pos);
				} else if (strncasecmp(full.c_str(), "https://", 8) == 0) {
					pos = full.find("/", 8);
					if (pos != string::npos)
						full = full.substr(0, pos);
				} else {
					full = "";
				}
			}

			while ((pos = full.find("/./")) != string::npos)
				full = full.substr(0, pos + 1) + full.substr(pos + 3);

			full += p;

			if (strncasecmp(full.c_str(), "file:/", 6) == 0) {
				root = 6;
			} else if (strncasecmp(full.c_str(), "http://", 7) == 0) {
				unsigned int sl = full.find("/", 7);
				if (sl != string::npos) {
					root = sl + 1;
				} else {
					full += "/";
					root = full.length();
				}
			} else if (strncasecmp(full.c_str(), "https://", 8) == 0) {
				unsigned int sl = full.find("/", 8);
				if (sl != string::npos) {
					root = sl + 1;
				} else {
					full += "/";
					root = full.length();
				}
			} else {
				root = 0;
			}

			while ((pos = full.find("/../")) != string::npos) {
				if (root < pos) {
					unsigned int q = full.rfind("/", pos - 1);
					full = full.substr(0, q + 1) + full.substr(pos + 4);
				} else {
					full = full.substr(0, pos + 1) + full.substr(pos + 4);
				}
			}

			if (full.find_first_of(":") == string::npos)
				full.insert(0, "file:");
		}
		return full;
	} else {
		if (path.length() == 0)
			return path;
		if (path.find_first_of(":") != string::npos)
			return path;
		if (strncmp(path.c_str(), "$$", 2) == 0)
			return path;

		string full = dir;
		string p = path;
		int n;

		if (p[0] == '/') {
			if (strncasecmp(full.c_str(), "file:/", 7) == 0) {
				full = full.substr(0, 5);
			} else if (strncasecmp(full.c_str(), "http://", 7) == 0) {
				unsigned int pos = full.find("/", 7);
				if (pos != string::npos)
					full = full.substr(0, pos);
			} else if (strncasecmp(full.c_str(), "https://", 8) == 0) {
				unsigned int pos = full.find("/", 8);
				if (pos != string::npos)
					full = full.substr(0, pos);
			} else {
				full = "";
			}
		}

		while (strncmp(p.c_str(), "../", 3) == 0) {
			unsigned int q = full.rfind('/', full.length() - 2) + 1;
			full.erase(q, full.length() - q);
			p.erase(0, 3);
		}

		full += p;

		if (full.find_first_of(":") == string::npos)
			full.insert(0, "file:");

		for (;;) {
			StringUtil t(full);
			n = t.Replace("/./", "/");
			const string &r = t;  /* redundant as written */
			full = r;
			if (n == 0)
				break;
		}

		return full;
	}
}

/* Parts of an URI. */
/* The URI without its query string. */
static string url_part_a(string s)
{
	unsigned int pos = s.find_first_of("?");

	if (pos != string::npos)
		return s.substr(0, pos);
	return s;
}

/* Directory of the URI, up to the last "/" included; "./" if there is none. */
static string url_part_b(string s)
{
	string t = url_part_a(s);
	unsigned int pos = t.find_last_of("/");

	if (pos != string::npos)
		return t.substr(0, pos + 1);
	return "./";
}

/* Query string of the URI (after the first "?"), "" if there is none. */
static string url_part_c(string s)
{
	unsigned int pos = s.find_first_of("?");

	if (pos != string::npos)
		return s.substr(pos + 1);
	return "";
}

/* Defaults: fade-out time 500, status 200, client PSML version 110. */
thXmlLoader::thXmlLoader(const string &url, const string &a, const string &b, int c, int d, void *e,
    void (*cb)(void *, int, SceneWindow::CreateArg *), Buffer buf)
        : m_uri(url_part_a(url)), m_dir(url_part_b(url)), m_query(url_part_c(url)), m_name(a), m_buffer(buf), m_mode(c),
          m_flags(d), m_script(b), m_key(), m_fadeoutTime(500), m_cbArg(e), m_callback(cb), m_resultCode(200),
          m_psmlVersion(0), m_clientPsmlVersion(110), m_loginStage(0), m_loginRetry(0)
{
	instance_count++;
	if (!isNetworkUri(m_uri.c_str()))
		NaviMenu::Lock(true);
	if (!(m_flags & THXML_NO_MODAL_COUNT) && m_mode == SceneWindow::MODE_MODAL)
		for_modal_window_count++;
	if (m_mode == SceneWindow::MODE_MAIN && !(m_flags & THXML_NO_HISTORY) && !(m_flags & THXML_DOCUMENT_ONLY))
		HisExtension_Add(m_uri, m_name, m_script, m_buffer, (m_flags >> 3) & 1, (m_flags >> 4) & 1);
}

thXmlLoader::~thXmlLoader()
{
	instance_count--;
	if (!isNetworkUri(m_uri.c_str()))
		NaviMenu::Lock(false);
	if (!(m_flags & THXML_NO_MODAL_COUNT) && m_mode == SceneWindow::MODE_MODAL)
		for_modal_window_count--;
}

/* Reads a whole PFS file with the BB* API; bit 0 of flags appends a NUL. */
static void *load_pfs_file(const char *path, int *sizeOut, int flags)
{
	int fd;
	int nul;
	char *p;

	fd = BBopenCPP(path, 0);
	if (fd < 0)
		goto fail;
	*sizeOut = BBlseekCPP(fd, 0, 2);
	BBlseekCPP(fd, 0, 0);
	nul = flags & 1;
	p = (char *)malloc(*sizeOut + nul);
	if (p == 0)
		goto closefail;
	if (BBreadCPP(fd, p, *sizeOut) != *sizeOut)
		goto freefail;
	BBcloseCPP(fd);
	if (nul)
		p[*sizeOut] = 0;
	return p;
freefail:
	free(p);
	BBcloseCPP(fd);
	return 0;
closefail:
	BBcloseCPP(fd);
fail:
	return 0;
}

/* Same for any path: "pfs:" URIs go to load_pfs_file, the others through stdio. */
static void *load_file(const char *path, int *sizeOut, int flags)
{
	FILE *fp;
	int nul;
	char *p;

	if (isPfsUri(path))
		return load_pfs_file(path, sizeOut, flags);
	fp = fopen(path, "rb");
	if (fp == 0)
		goto fail;
	fseek(fp, 0, SEEK_END);
	*sizeOut = ftell(fp);
	rewind(fp);
	nul = flags & 1;
	p = (char *)malloc(*sizeOut + nul);
	if (p == 0)
		goto closefail;
	if (fread(p, 1, *sizeOut, fp) != (unsigned int)*sizeOut)
		goto freefail;
	fclose(fp);
	if (nul)
		p[*sizeOut] = 0;
	return p;
freefail:
	free(p);
	fclose(fp);
	return 0;
closefail:
	fclose(fp);
fail:
	return 0;
}

static void reset_xmlload_ramdisk();
static int setup_xmlload_ramdisk();
static int run_ps2cpnc(const char *firstArg, ...);

/* Part of s from p, n characters long. */
static inline string str_mid(const string &s, unsigned int p, unsigned int n)
{
	string t = s.substr(p, n);

	return t;
}

/* Debug trace, compiled out. */
#define XL_DPRINTF(args)

/* Reads the system authlist (exported by the "-D" tool into the XML ramdisk)
   and, if its first line is for uri, copies id and password into
   SystemParam::auth_id/auth_pass. */
static int load_authlist(const string &uri)
{
	int fd = -1;
	char *buf = 0;
	int size;
	StringUtil content;
	vector<StringUtil> lines;

	SystemParam::auth_id = "";
	SystemParam::auth_pass = "";

	if (setup_xmlload_ramdisk() != 0)
		goto done;
	if (run_ps2cpnc("-D", "/opt0/bn/sysconf/authlist", "/var/ps2/xmlload/authlist.tmp", 0) != 0)
		goto done;
	fd = open("/var/ps2/xmlload/authlist.tmp", O_RDONLY);
	if (fd < 0)
		goto done;
	size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	if (size <= 0)
		goto done;
	buf = (char *)malloc(size + 1);
	if (buf == 0)
		goto closefd;
	if (read(fd, buf, size) < size)
		goto done;
	buf[size] = 0;
	content.assign(buf);
done:
	if (buf != 0)
		free(buf);
closefd:
	if (fd >= 0)
		close(fd);
	reset_xmlload_ramdisk();

	lines = content.Separate("\n");

	if (!lines.empty()) {
		for (unsigned int i = 0; i < lines.size(); i++)
			XL_DPRINTF(("authlist[%d] %s\n", i, lines[i].c_str()));
		vector<StringUtil> fields = lines[0].Separate(" ");

		if (fields.size() == 3 && uri == string(fields[0])) {
			trim_left(fields[1]);
			trim_right(fields[1]);
			SystemParam::auth_id = fields[1];
			trim_left(fields[2]);
			trim_right(fields[2]);
			SystemParam::auth_pass = fields[2];
			return 1;
		}
	}
	return 0;
}

/* Escapes the special characters of a query value: "%" first (so that the
   escapes already inserted are not doubled), then ";", "=" and " ". */
static void escape_psml(StringUtil &s)
{
	StringUtil t;

	t = s;
	t.Replace("%", "%%");
	t.Replace(";", "%s");
	t.Replace("=", "%e");
	t.Replace(" ", "%b");
	s = t;
}

/* Saves the line "<uri> <id> <pass>" (only if auth_save is set) into the
   system authlist, through the XML ramdisk and the "-E" tool; does nothing
   unless auth_ok is set. */
static int save_authlist(const string &uri)
{
	if (SystemParam::auth_ok == 0)
		return 1;

	string line;
	int fd = -1;

	if (SystemParam::auth_save != 0) {
		string id = SystemParam::auth_id;
		string pass = SystemParam::auth_pass;

		line = uri + " " + id + " " + pass;
	}

	if (setup_xmlload_ramdisk() == 0) {
		fd = open("/var/ps2/xmlload/authlist.tmp", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd < 0)
			return 0;
		write(fd, line.c_str(), line.length());
		close(fd);
		run_ps2cpnc("-E", "/var/ps2/xmlload/authlist.tmp", "/opt0/bn/sysconf/authlist", 0);
	}
	if (fd >= 0)
		close(fd);
	reset_xmlload_ramdisk();
	sync();
	return 1;
}

static inline int b64val(int c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 26;
	if (c >= '0' && c <= '9')
		return c - '0' + 52;
	if (c == '+')
		return 62;
	if (c == '/')
		return 63;
	return -1;
}

/* Decodes base64 in place; stores the decoded length in *lenOut and returns
   the number of characters consumed. */
static int b64_decode(char *buf, int *lenOut)
{
	char *p = buf;
	char *q = buf;
	int acc = 0;
	int bits = 0;
	int v;

	while ((v = b64val(*p)) >= 0) {
		if (bits != 0) {
			acc = (acc << bits) | (v >> (6 - bits));
			*q++ = acc;
		}
		acc = v;
		p++;
		bits = (bits + 2) & 7;
	}
	*lenOut = q - buf;
	return p - buf;
}

/* Undoes the obfuscation of a script key: ROT13 on letters and the swap
   '+' <-> '&', '/' <-> '#', '=' <-> '~', then base64 decoding. */
static void psml_decrypt(char *s, int *lenOut)
{
	int i;

	for (i = 0; s[i] != 0; i++) {
		if (s[i] >= 'a' && s[i] <= 'z')
			s[i] = (s[i] - 84) % 26 + 'a';
		else if (s[i] >= 'A' && s[i] <= 'Z')
			s[i] = (s[i] - 52) % 26 + 'A';
		else if (s[i] == '+')
			s[i] = '&';
		else if (s[i] == '/')
			s[i] = '#';
		else if (s[i] == '=')
			s[i] = '~';
		else if (s[i] == '&')
			s[i] = '+';
		else if (s[i] == '#')
			s[i] = '/';
		else if (s[i] == '~')
			s[i] = '=';
	}
	b64_decode(s, lenOut);
}

/* bnEnumSysconf callback: assigns lit to s if s is still empty. */
static int fill_if_empty(const char *, const char *lit, void *arg)
{
	string &s = *(string *)arg;

	if (s.length() == 0)
		s = lit;
	return 0;
}


/* Returns the decoded script key of uri: from the SCRIPT_KEYS section of the
   sysconf (by host) for network URIs, a fixed key for the CD-ROM and another
   one for the rest. */
static string script_key(const string &uri)
{
	string key;
	char *buf;
	int n;

	if (isNetworkUri(uri.c_str())) {
		string host = HttpFile::UriToHost(uri);
		bnEnumSysconf("SCRIPT_KEYS", host.c_str(), fill_if_empty, &key);
	} else if (isCdromPath(uri.c_str())) {
		key = "ovLkZlt2oaLkAQVdqaOiXmA0ot~~";
	} else {
		key = "HGAcGaMdFKp5qN~~";
	}

	buf = (char *)malloc(key.length() + 1);
	strcpy(buf, key.c_str());
	psml_decrypt(buf, &n);
	buf[n] = 0;
	key = buf;
	free(buf);
	return key;
}

/* Rewrites a query string with every key and value URL-escaped. */
static string url_query_escape(const string &q)
{
	StringUtil src = q;
	string out;
	vector<StringUtil> parts = src.Separate("&");
	unsigned int i;

	for (i = 0; i < parts.size(); i++) {
		int pos = parts[i].find(string("="));

		if (pos > 0 && pos < (int)parts[i].length() - 1) {
			StringUtil k = str_mid(parts[i], 0, pos);
			unsigned int len = parts[i].length();
			unsigned int st = pos + 1;
			StringUtil v = str_mid(parts[i], st, len - st);
			char *ek = sceURLEscape(k.c_str());
			char *ev = sceURLEscape(v.c_str());

			out += string(ek) + "=" + ev;
			if (i < parts.size() - 1)
				out += "&";
			free(ek);
			free(ev);
		} else {
			char *e = sceURLEscape(string(parts[i]).c_str());

			out += e;
			if (i < parts.size() - 1)
				out += "&";
			free(e);
		}
	}
	return out;
}

/* Reads <RESOURCE version="..."> and sets m_psmlVersion; returns 0, or
   0xFFFEA06E if the root is not RESOURCE or the version is missing,
   0xFFFEA06B for a rejected version, 0xFFFEA06C for an unknown one. */
int thXmlLoader::psml_version_check()
{
	string ver;
	Element root = m_document.get_documentElement();
	string tag = (const char *)root.get_tagName().GetBufferPointer();
	if (tag.compare("RESOURCE") == 0) {
		NamedNodeMap attrs = root.get_attributes();
		Node n = attrs.getNamedItem(XMLString("version", false));
		if (n != 0 && n.get_nodeType() != 0)
			ver = (const char *)n.get_nodeValue().GetBufferPointer();
		else
			return 0xfffea06e;
	} else {
		return 0xfffea06e;
	}

	int i;
	for (i = 0; psml_known_versions[i].name != 0; i++) {
		if (ver.compare(psml_known_versions[i].name) == 0) {
			m_psmlVersion = psml_known_versions[i].version;
			return 0;
		}
	}
	for (i = 0; psml_rejected_versions[i].name != 0; i++) {
		if (ver.compare(psml_rejected_versions[i].name) == 0)
			return 0xfffea06b;
	}
	return 0xfffea06c;
}

/* Reads screen_w, screen_h and screen_d from the <RESOURCE> root (defaults 640, 448, 500). */
static int read_screen_params(const Document &doc, int *w, int *h, int *d)
{
	string sw, sh, sd;
	Element root = doc.get_documentElement();
	string tag = (const char *)root.get_tagName().GetBufferPointer();

	if (tag.compare("RESOURCE") == 0) {
		NamedNodeMap attrs = root.get_attributes();
		Node n = attrs.getNamedItem(XMLString("screen_w", false));
		if (n != 0 && n.get_nodeType() != 0)
			sw = (const char *)n.get_nodeValue().GetBufferPointer();
		else
			sw = "640";
		n = attrs.getNamedItem(XMLString("screen_h", false));
		if (n != 0 && n.get_nodeType() != 0)
			sh = (const char *)n.get_nodeValue().GetBufferPointer();
		else
			sh = "448";
		n = attrs.getNamedItem(XMLString("screen_d", false));
		if (n != 0 && n.get_nodeType() != 0)
			sd = (const char *)n.get_nodeValue().GetBufferPointer();
		else
			sd = "500";
	} else {
		return 0;
	}

	*w = strtol(sw.c_str(), 0, 10);
	*h = strtol(sh.c_str(), 0, 10);
	*d = strtol(sd.c_str(), 0, 10);
	return 1;
}

/* Case-insensitive comparison of two strings. */
static inline int same_text_nocase(const StringUtil &s, StringUtil pat)
{
	StringUtil head(s);

	head.MakeUpper();
	pat.MakeUpper();
	return head.compare(pat);
}

/* Thread body: downloads or reads the PSML document (asking for the
   credentials when the server wants them), parses it, checks its version,
   then walks the <SCRIPT> elements loading the script modules into
   m_imageMap/m_uriMap. */
int thXmlLoader::threadProc()
{
	string url;
	string base(m_uri);
	HttpFile hf;
	sceHttpFileRequestArg arg;
	string user;
	string pass;
	int done;
	int size;
	int ret;

	memset(&arg, 0, sizeof(arg));
	arg.done_flag = &done;
	hf.LimitCache(512, 0x1000000);
	for (;;) {
		if (isNetworkUri(m_uri.c_str())) {
			{
				std::list<string> vars;

				if (Variables::GetVariable(string("UpdateErrorFlag"), vars) <= 0) {
					m_resultCode = 0xfffea068;
					break;
				}
				/* the variable is "SysupOK" or "SysupNGn" */
				if (strncmp((*vars.begin()).c_str(), "SysupOK", 7) != 0) {
					switch (strtol(vars.front().c_str() + 7, 0, 10)) {
					case 1:
					case 8:
					case 9:
					case 10:
					case 12:
						m_resultCode = 0xfffea069;
						break;
					case 2:
					case 14:
					case 15:
						m_resultCode = 0xfffea066;
						break;
					default:
						m_resultCode = 0xfffea068;
						break;
					}
					break;
				}
			}
			/* HTTP request */
			{
				Buffer buf(m_buffer);
				int post = 0;

				if (m_clientPsmlVersion == 100) {
					if (m_buffer.m_p != 0 && (m_flags & THXML_POST_ESCAPED)) {
						buf = Buffer(url_query_escape(m_buffer));
						post = 1;
					}
				} else if (m_buffer.m_p != 0 && (m_flags & THXML_POST_FORM))
					post = 1;
				if (post)
					arg.content_type = (char *)"application/x-www-form-urlencoded";

				string u(m_uri);
				if (m_clientPsmlVersion == 100) {
					if (m_query.length() != 0)
						u += "?" + url_query_escape(m_query);
				} else if (m_query.length() != 0)
					u += "?" + m_query;

				arg.mode = 1;
				arg.timeout = 30000;
				ret = hf.Request(u, buf.GetData(), buf.GetSize(), &arg);
				arg.mode = 0;
				arg.content_type = 0;
				if (done != 0)
					hf.save_cookie();
				string redirect = HttpFile::CheckRedirectList(u.c_str());
				if (redirect != u) {
					m_uri = url_part_a(redirect);
					m_dir = url_part_b(redirect);
					m_query = url_part_c(redirect);
				}
				url = (const char *)hf;
			}
		} else if (strncasecmp(m_uri.c_str(), "file:", 5) == 0) {
			/* Local "file:" URI. */
			ret = 200;
			url = m_uri.substr(5);
		} else {
			ret = 200;
			url = m_uri;
		}

		if (ret == 200) {
			char *p = (char *)load_file(url.c_str(), &size, 1);

			if (p == 0) {
				m_resultCode = 404;
				break;
			}
			m_document = XmlParse::ParseString(p);
			free(p);
			break;
		}
		if (ret != 401 && ret != 407) {
			m_resultCode = ret;
			break;
		}
		/* Authentication required. */
		{
			string host = HttpFile::UriToHost(m_uri);

			load_authlist(host);
			m_loginStage = 0;
			m_pendingEvent = 1;
			waitEventProc();
			if (SystemParam::auth_ok != 0) {
				user = SystemParam::auth_id;
				pass = SystemParam::auth_pass;
				escape_psml(static_cast<StringUtil &>(SystemParam::auth_id));
				escape_psml(static_cast<StringUtil &>(SystemParam::auth_pass));
				save_authlist(host);
				if (!user.empty() && !pass.empty()) {
					arg.user = (char *)user.c_str();
					arg.pass = (char *)pass.c_str();
				}
				continue;
			} else {
				m_resultCode = ret;
				break;
			}
		}
	}

	/* Leave at once unless the status is 200. */
	if (m_resultCode != 200)
		return 0;

	/* Valid document? */
	if (m_document == 0) {
		m_resultCode = 0xfffea06f;
		return 0;
	}
	if (m_flags & THXML_DOCUMENT_ONLY)
		return 0;
	{
		int rc = psml_version_check();
		if (rc != 0) {
			m_resultCode = rc;
			return 0;
		}
	}
	if (!read_screen_params(m_document, &m_screenW, &m_screenH, &m_screenD))
		return 0;
	m_key = script_key(base);

	/* Walk the <SCRIPT> elements and load the .bnx modules. */
	{
		NodeList list = m_document.getElementsByTagName(XMLString("SCRIPT", false));
		unsigned long i;

		for (i = 0; i < list.get_length(); i++) {
			NamedNodeMap attrs = list.item(i).get_attributes();
			Node t = attrs.getNamedItem(XMLString("type", false));

			if (t != 0 && t.get_nodeType() != 0) {
				string type = (const char *)t.get_nodeValue().GetBufferPointer();

				if (type.compare("binary/small") == 0) {
					Node s = attrs.getNamedItem(XMLString("src", false));

					if (s != 0 && s.get_nodeType() != 0) {
						string src = (const char *)s.get_nodeValue().GetBufferPointer();
						int bnx;

						src = GetFullPath(m_dir, src, m_clientPsmlVersion);
						bnx = same_text_nocase(StrRight(src, 4), ".bnx");
						if (bnx != 0)
							src += ".bnx";
						/* Fetch of the module: a single pass. */
						for (;;) {  /* redundant as written */
							if (isNetworkUri(src.c_str())) {
								ret = hf.Request(src, 0, 1, &arg);
								if (done != 0)
									hf.save_cookie();
								url = (const char *)hf;
							} else if (strncasecmp(src.c_str(), "file:", 5) == 0) {
								ret = 200;
								url = src.substr(5);
							} else {
								ret = 200;
								url = src;
							}
							if (ret == 200) {
								string id;
								Node d = attrs.getNamedItem(XMLString("id", false));

								if (d != 0 && d.get_nodeType() != 0)
									id = (const char *)d.get_nodeValue()
									         .GetBufferPointer();
								if (m_imageMap.find(id) == m_imageMap.end()) {
									int isize;
									unsigned int n;
									void *img = load_file(url.c_str(), &isize, 0);

									if (img == 0) {
										m_resultCode = 404;
										return 0;
									}
									if (__sce_dnas2_symbol60010(isize,
									        (unsigned char *)img, m_key.length(),
									        (const unsigned char *)m_key.c_str(),
									        &n)
									    != 0) {
										m_resultCode = 0xfffea06d;
										free(img);
										return 0;
									}
									if (Script::IsValidAmx((const AMX_HEADER *)img,
									        m_psmlVersion)) {
										m_imageMap[id] = img;
										m_uriMap[id] = src;
										Script::LoadModule(img);
									} else {
										free(img);
										m_resultCode = 0xfffea067;
										return 0;
									}
								}
							} else {
								m_resultCode = ret;
								return 0;
							}
							break;
						}
					}
				}
			}
		}
	}
	/* At least one module is needed, and one of them must be "main". */
	if (!m_imageMap.empty()) {
		if (m_imageMap.find("main") == m_imageMap.end()) {
			m_resultCode = 0xfffea06e;
			return 0;
		}
	} else {
		m_resultCode = 0xfffea06e;
		return 0;
	}
	m_resultCode = 200;
	return 0;
}

/* Login loaders still open. */
static int login_loader_count;

/* Close callback of the login window. */
static void login_loader_done()
{
	login_loader_count--;
}

/* Callback of the login loader: creates the login window and registers its close callback. */

static void login_loader_callback(void *, int, SceneWindow::CreateArg *createArg)
{
	SceneWindow *sc;

	if (createArg == 0)
		return;
	sc = CreateSceneWindow(*createArg);
	if (sc == 0)
		return;
	sc->SetCallback((SceneWindow::CallbackType)1, 0x43494C, login_loader_done);
}

/* Event pump of the thread. Stage 0 (m_loginStage) starts a second loader on
   login.xml with the stored credentials; stage 1 waits for it to close. */
int thXmlLoader::threadEvent()
{
	if (m_abortFlag != 0)
		return 1;

	switch (m_loginStage) {
	case 0: {
		char buf[32];
		sprintf(buf, "%d", m_loginRetry);
		string q = string("id=") + SystemParam::auth_id + string(";pass=") + SystemParam::auth_pass
		    + string(";retry=") + buf;
		thXmlLoader *p = new thXmlLoader("/opt0/bn/script/system/login.xml", "", q, SceneWindow::MODE_MODAL, 0,
		    0, login_loader_callback, Buffer());
		p->start();
		p->release();
		login_loader_count++;
		m_loginRetry = 1;
		m_loginStage = 1;
		return 0;
	}
	case 1:
		if (login_loader_count > 0)
			return 0;
		/* falls through to case 2 */
	case 2:
		return 1;
	default:
		return 1;
	}
}

/* Hands the window to create (CreateArg built from the loaded document) to
   the callback, or to CreateSceneWindow when there is none; on error (status
   not 200) the callback gets 0. */
int thXmlLoader::threadFinished()
{
	m_pendingEvent = 0;
	if (m_resultCode == 200) {
		SceneWindow::CreateArg arg(m_mode, m_psmlVersion, m_screenW, m_screenH, m_screenD, m_fadeoutTime, m_uri,
		    m_dir, m_name, m_script, m_key, m_document, m_imageMap, m_uriMap);
		if (m_callback)
			m_callback(m_cbArg, m_resultCode, &arg);
		else
			CreateSceneWindow(arg);
	} else {
		if (m_callback)
			m_callback(m_cbArg, m_resultCode, 0);
	}
	return 1;
}

/* Tells whether m_uri is a network URI. */
int thXmlLoader::IsFromNetwork()
{
	return isNetworkUri(m_uri.c_str());
}

/* Programs started by run_ps2cpnc and run_mke2fs. */
static char *g_ps2cpncPath = "/opt0/bn/netconf/ps2cpnc";
static char *g_mke2fsPath = "/sbin/mke2fs";

/* Unmounts the XML cache ramdisk and frees its buffers (BLKFLSBUF, also if open failed). */
static void reset_xmlload_ramdisk()
{
	int fd;

	if (umount("/var/ps2/xmlload") != 0)
		perror("warning umount\n");
	fd = open("/dev/ramdisk", O_RDWR);
	if (fd == -1)
		perror("warning open\n");
	ioctl(fd, BLKFLSBUF);
	close(fd);
}

/* (Re)creation of the ramdisk of the XML cache. */

static int run_mke2fs(const char *firstArg, ...);

/* Creates /var/ps2/xmlload, makes a 512-block ext2 on the ramdisk, mounts it
   and removes lost+found; on error resets the ramdisk and returns -1. */
static int setup_xmlload_ramdisk()
{
	if (mkdir("/var/ps2", 0700) != 0 && errno != EEXIST)
		goto err_mkdir;
	if (mkdir("/var/ps2/xmlload", 0700) != 0 && errno != EEXIST) {
err_mkdir:
		perror("error mkdir\n");
		goto fail;
	}
	if (run_mke2fs("-m0", "-b1024", "/dev/ramdisk", "512", 0) != 0) {
		perror("error mke2fs\n");
		goto fail;
	}
	if (mount("/dev/ramdisk", "/var/ps2/xmlload", "ext2", 0, 0) != 0) {
		perror("error mount\n");
		goto fail;
	}
	rmdir("/var/ps2/xmlload/lost+found");
	return 0;
fail:
	reset_xmlload_ramdisk();
	return -1;
}

static volatile int g_toolPid;        /* redundant as written */

/* fork + execv + waitpid; returns the exit status of the child, 127 if it could not run. */
static int spawn_wait(const char *path, char *const *argv)
{
	int status;

	g_toolPid = fork();
	if (g_toolPid > 0) {
		waitpid(g_toolPid, &status, 0);
		g_toolPid = 0;
		return (status >> 8) & 0xff;
	}
	if (g_toolPid != 0)
		return 127;
	execv(path, argv);
	exit(127);
}

/* Run g_ps2cpncPath / g_mke2fsPath with a NULL-terminated argument list. */

static int run_ps2cpnc(const char *firstArg, ...)
{
	char *argv[16];
	va_list ap;
	int i = 0;

	argv[i++] = g_ps2cpncPath;
	argv[i] = (char *)firstArg;
	va_start(ap, firstArg);
	while (argv[i++] != 0)
		argv[i] = va_arg(ap, char *);
	return spawn_wait(g_ps2cpncPath, argv);
}

static int run_mke2fs(const char *firstArg, ...)
{
	char *argv[16];
	va_list ap;
	int i = 0;

	argv[i++] = g_mke2fsPath;
	argv[i] = (char *)firstArg;
	va_start(ap, firstArg);
	while (argv[i++] != 0)
		argv[i] = va_arg(ap, char *);
	return spawn_wait(g_mke2fsPath, argv);
}

void thXmlLoader::SetFadeoutTime(int t)
{
	m_fadeoutTime = t;
}

const string &thXmlLoader::getURI() const
{
	return m_uri;
}

void thXmlLoader::abortProc()
{}

void thXmlLoader::releaseProc()
{}

int thXmlLoader::PSMLVersion() const
{
	return m_psmlVersion;
}

void thXmlLoader::SetClientPSMLVersion(int v)
{
	m_clientPsmlVersion = v;
}

int thXmlLoader::IsLoadingModalWindow()
{
	return for_modal_window_count != 0;
}

void thXmlLoader::SetCryptionState(int v)
{
	for_cryption_state = v;
}

int thXmlLoader::GetCryptionState()
{
	return for_cryption_state;
}

int thXmlLoader::InstanceCount()
{
	return instance_count;
}
