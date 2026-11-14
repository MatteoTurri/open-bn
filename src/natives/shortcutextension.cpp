/*
 * Small/amx natives "shortcut*": the shortcut list of the PSBBN application.
 * One page of at most 4 entries, loaded from /opt1/bn/sysconf/shortcut_%d at
 * init and saved back asynchronously by a thJob after every change.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <vector>

#include "amx.h"
#include "natives/smallext.h"

#include "http/sceurl.h"
#include "script/script.h"
#include "shortcutextension.h"
#include "thread/thposixthread.h"
#include "thread/thjob.h"

struct Shortcut {
	/* 0x00 */ string label;  /* replaced when an entry with the same keys is set */
	/* 0x04 */ string key1;
	/* 0x08 */ string key2;

	Shortcut(const string &a, const string &b, const string &c) : label(a), key1(b), key2(c) {}
	Shortcut(const Shortcut &o) : label(o.label), key1(o.key1), key2(o.key2) {}
};

class MutexLock {
public:
	MutexLock(pthread_mutex_t *m) : m_m(m) { pthread_mutex_lock(m_m); }
	~MutexLock() { pthread_mutex_unlock(m_m); }

private:
	pthread_mutex_t *m_m;
};

/* One page of shortcuts. */
class ShortcutList : public vector<Shortcut> {  /* redundant as written */
public:
	ShortcutList() {}
	~ShortcutList() {}
};

static ShortcutList s_shortcuts[1];
static pthread_mutex_t s_shortcutMutex;

static void ShortcutSave(int page);

/* True if [p, q] lies entirely in the AMX heap or entirely in the script stack. */
static inline int amx_addr_ok(AMX *amx, cell p, cell q)
{
	if (p >= 0 && p < amx->hea && q >= 0 && q < amx->hea)
		return 1;
	if (p >= amx->stk && p < amx->stp && q >= amx->stk && q < amx->stp)
		return 1;
	return 0;
}

/* Helpers of shortcutSet: add a new entry at the front, or move an existing one there. */
static inline void ShortcutAdd(vector<Shortcut> *sc, const string &a, const string &b, const string &c)
{
	sc->insert(sc->begin(), Shortcut(a, b, c));
}

static inline void ShortcutTop(vector<Shortcut> *sc, vector<Shortcut>::iterator it)
{
	Shortcut s = *it;
	sc->erase(it);
	sc->insert(sc->begin(), s);
}

/*
 * shortcutSet(page, name, key, url): if an entry with the same key and url
 * exists it is renamed and moved to the front; otherwise a new entry is added
 * at the front if there is room (at most 4). Returns 0, or -1 if the page is
 * full; the page is saved in both cases.
 */
static cell AMX_NATIVE_CALL n_shortcutSet(AMX *amx, cell *params)
{
	int page = params[1];
	string a = CellToString(amx, params[2]);
	string b = CellToString(amx, params[3]);
	string c = CellToString(amx, params[4]);
	int ret;

	if (page != 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	vector<Shortcut> *sc = &s_shortcuts[0];
	vector<Shortcut>::iterator it;
	const string *pa = &a;

	for (it = sc->begin(); it != sc->end(); ++it) {
		if (it->key1 == b && it->key2 == c) {
			it->label = *pa;
			ShortcutTop(sc, it);
			ret = 0;
			goto save;
		}
	}
	if (sc->size() + 1 <= 4) {
		ShortcutAdd(sc, *pa, b, c);
		ret = 0;
	} else {
		ret = -1;
	}
save:
	ShortcutSave(page);
	return ret;
}

/* Field 0-2 of entry `index`, or an empty string. */
static inline string ShortcutField(vector<Shortcut> *sc, int index, int field)
{
	if ((unsigned int)index < sc->size()) {
		switch (field) {
		case 0:
			return (*sc)[index].label;
		case 1:
			return (*sc)[index].key1;
		case 2:
			return (*sc)[index].key2;
		}
	}
	return "";
}

/* shortcutGet(page, index, field, dest, destsize): packed copy of one field, truncated to destsize cells. */
static cell AMX_NATIVE_CALL n_shortcutGet(AMX *amx, cell *params)
{
	int page = params[1];
	int index = params[2];
	int field = params[3];
	cell *cptr;

	if (page != 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	if (index < 0 || index >= (int)s_shortcuts[page].size()) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	if ((unsigned int)field >= 3) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	{
		string v = ShortcutField(&s_shortcuts[page], index, field);

		if ((unsigned int)(params[5] * 4) < v.length() + 1)
			v = v.substr(0, params[5] * 4 - 1);
		if (!amx_addr_ok(amx, params[4], v.length() + params[4])) {
			amx_RaiseError(amx, 5);
			return 0;
		}
		amx_GetAddr(amx, params[4], &cptr);
		amx_SetString(cptr, (char *)v.c_str(), 1);
	}
	return 0;
}

/* shortcutDel(page, key, url): removes the matching entry, then saves the page. */
static cell AMX_NATIVE_CALL n_shortcutDel(AMX *amx, cell *params)
{
	int page = params[1];
	string a = CellToString(amx, params[2]);
	string b = CellToString(amx, params[3]);

	if (page != 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	{
		vector<Shortcut> *sc = &s_shortcuts[0];
		vector<Shortcut>::iterator it;
		const string *pa = &a;

		for (it = sc->begin(); it != sc->end(); ++it) {
			if (it->key1 == *pa && it->key2 == b) {
				sc->erase(it);
				break;
			}
		}
	}
	ShortcutSave(page);
	return 0;
}

/* shortcutNum(page): number of entries; only page 0 exists. */
static cell AMX_NATIVE_CALL n_shortcutNum(AMX *amx, cell *params)
{
	vector<Shortcut> *sc;

	if (params[1] != 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	sc = &s_shortcuts[0];
	return sc->size();
}

/* Rebuilds the page from the file text: records `label SP key1 SP key2 LF`, URL-escaped. */
static inline void ShortcutParse(vector<Shortcut> *sc, const string &text)
{
	sc->erase(sc->begin(), sc->end());

	unsigned pos = 0;
	unsigned p;

	while ((p = text.find(" ", pos)) != (unsigned)-1) {
		string fieldLabel = text.substr(pos, p - pos);
		pos = p + 1;

		p = text.find(" ", pos);
		if (p == (unsigned)-1)
			break;
		string fieldKey1 = text.substr(pos, p - pos);
		pos = p + 1;

		p = text.find("\n", pos);
		if (p == (unsigned)-1)
			break;
		string fieldKey2 = text.substr(pos, p - pos);
		pos = p + 1;

		char *label = sceURLUnescape(fieldLabel.c_str());
		char *key1 = sceURLUnescape(fieldKey1.c_str());
		char *key2 = sceURLUnescape(fieldKey2.c_str());

		sc->push_back(Shortcut(label, key1, key2));

		free(label);
		free(key1);
		free(key2);
	}
}

AMX_NATIVE_INFO smallext_shortcut[] = {
	{ "shortcutSet", n_shortcutSet },
	{ "shortcutGet", n_shortcutGet },
	{ "shortcutDel", n_shortcutDel },
	{ "shortcutNum", n_shortcutNum },
	{ 0, 0 },
};

void ShortcutExtension_Init()
{
	int i;

	pthread_mutex_init(&s_shortcutMutex, 0);

	MutexLock lk(&s_shortcutMutex);

	for (i = 0; i < 1; i++) {
		char path[256];

		sprintf(path, "/opt1/bn/sysconf/shortcut_%d", i);

		FILE *fp = fopen(path, "rb");
		if (fp) {
			fseek(fp, 0, SEEK_END);
			int len = ftell(fp);
			rewind(fp);

			char *data = (char *)malloc(len + 1);  /* NOTE: bug: never freed when the read succeeds */
			if (data) {
				int n = fread(data, 1, len, fp);
				if (n == len) {
					fclose(fp);
					data[n] = 0;

					string text(data);

					ShortcutParse(&s_shortcuts[i], text);
				} else {
					free(data);
					fclose(fp);
				}
			} else {
				fclose(fp);
			}
		}
	}
}

void ShortcutExtension_Exit()
{}

/* Save job slots: m_int[SHORTCUT_JOB_PAGE] the page, m_ptr[0] the vector<Shortcut> copy. */
enum {
	SHORTCUT_JOB_PAGE = 0
};

static void ShortcutSaveRun(thJob *job);
static void ShortcutSaveDone(thJob *job);

/* Saves the page asynchronously: a copy of the vector goes to a thJob that writes the file. */
static void ShortcutSave(int page)
{
	vector<Shortcut> *copy = new vector<Shortcut>;

	*copy = s_shortcuts[page];

	thJob *job = new thJob(0, ShortcutSaveRun, 0, ShortcutSaveDone);
	job->m_ptr[0] = copy;
	job->m_int[SHORTCUT_JOB_PAGE] = page;
	job->start();
	job->release();
}

/* File text of a page: `label SP key1 SP key2 LF` per entry, fields URL-escaped. */
static inline string ShortcutText(vector<Shortcut> *sc)
{
	string text;

	for (vector<Shortcut>::iterator it = sc->begin(); it != sc->end(); ++it) {
		char *escLabel = sceURLEscape(it->label.c_str());
		char *escKey1 = sceURLEscape(it->key1.c_str());
		char *escKey2 = sceURLEscape(it->key2.c_str());

		text += string(escLabel) + " " + escKey1 + " " + escKey2 + "\n";

		free(escLabel);
		free(escKey1);
		free(escKey2);
	}
	return text;
}

/* Job body: writes the page file. */
static void ShortcutSaveRun(thJob *job)
{
	MutexLock lk(&s_shortcutMutex);
	string data = ShortcutText((vector<Shortcut> *)job->m_ptr[0]);
	char path[256];

	sprintf(path, "/opt1/bn/sysconf/shortcut_%d", job->m_int[SHORTCUT_JOB_PAGE]);
	FILE *fp = fopen(path, "w");
	if (fp == 0) {
		perror("fopen");
	} else {
		if (fwrite(data.c_str(), data.length(), 1, fp) == 0)
			perror("fopen");
	}
	if (fp)
		fclose(fp);
	sync();
}

/* Job completion callback: deletes the vector copy. */
static void ShortcutSaveDone(thJob *job)
{
	delete (vector<Shortcut> *)job->m_ptr[0];
}
