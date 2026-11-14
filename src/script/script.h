/*
 * Script: host of a Small (AMX) virtual machine running the script of a PSML
 * page: loaded program, message queues, user data, callbacks and properties.
 */

#ifndef SCRIPT_H
#define SCRIPT_H

#include <vector>
#include <string>
#include <map>
#include <list>
#include "amx.h"

class Object;
class SceneWindow;
class Document;
class XmlTagTable;

class Script {
public:
	enum CallbackType {
		CALLBACK_EVERY_FRAME = 0,
		CALLBACK_ON_DESTROY,
		CALLBACK_TYPE_COUNT
	};

	struct MessageInt {
		int a, b, c, d, e;
	};

	struct MessageStr {
		int key;
		string value;
		MessageStr(int k, const string &v);
	};

	struct CallbackData {
		unsigned int key;
		void (*fn)(__amx *);
	};

	/* 0x00 */ string file;  /* key of the program in the window file cache */
	/* 0x04 */ string uri;  /* URI of that file */
	/* 0x08 */ __amx amx;
	/* 0x80 */ void *program;  /* loaded program, returned by LoadProgram */
	/* 0x84 */ int stp_cache;  /* stp of the header, kept across the reset of amx */
	/* 0x88 */ int state;  /* 3 released, 4 program failed to load */
	/* 0x8C */ int entry_index;  /* public function started by Run on a cold start */
	/* 0x90 */ unsigned int
	    flags; /* 0x1 held until its window starts, 0x2 network script, 0x4 busy, 0x8 HTTPS busy, 0x10 wipe program on free */
	/* 0x94 */ vector<int> int_list; /* the list of the list* natives */
	/* 0xA0 */ vector<MessageInt> int_queue; /* integer messages */
	/* 0xAC */ vector<MessageStr> string_queue; /* string messages */
	/* 0xB8 */ map<unsigned int, void *> user_data;
	/* 0xC4 */ vector<CallbackData> callbacks[CALLBACK_TYPE_COUNT];
	/* 0xDC */ map<string, string> args; /* "key=value;..." argument string, read by GetArg */

	static void Init();
	Script(Object *obj, const string &a, const string &b, const string &c);
	int GetWindowMode();
	SceneWindow *GetSceneWindow();
	static void Exit();
	static int IsValidAmx(const AMX_HEADER *hdr, int ver);
	void Release();
	void SetBusy(bool busy);
	void SetHttpsBusy(bool busy);
	void *GetUserData(unsigned int id);
	void *SetUserData(unsigned int id, void *data);
	void *DelUserData(unsigned int id);
	void SetCallback(CallbackType type, unsigned int key, void (*fn)(__amx *));
	void DoCallback(CallbackType type);
	static void TranslateAll(void (*fn)(Script *));
	static void RemoveAll();
	static void RunAll();
	void SendMessage(int a, int b, int c, int d, int e);
	int PeekMessage(int &a, int &b, int &c, int &d, int &e);
	int RecvMessage(int &a, int &b, int &c, int &d, int &e);
	int FindMessage(int key, int &b, int &c, int &d, int &e);
	void SendMessage(int key, const string &value);
	int PeekMessage(int &key, string &value);
	int RecvMessage(int &key, string &value);
	int FindMessage(int key, string &value);
	int RegisterAPI();
	void AddRefModule();
	void DecRefModule();
	void RegistModule();
	static void LoadModule(void *program);
	static void ReleaseModule(void *program);
	void *LoadProgram(void *program, int flag);
	void Run();
	~Script();

	/* Class counters. */
	static int instance_count;
	static int released_count;
	static int network_vm_sumsize;
	static int busy_count;
	static int https_busy_count;
}; /* size = 0xE8 */

unsigned Object__CommonSetup(Object *obj);
void Object__DisableDestroyFromScript(Object *obj);
Script *CreateScript(Object *obj, const string &file, const string &func, const string &arg);
string CellToString(struct __amx *amx, int index);
Document *GetDocument(struct __amx *amx);
XmlTagTable *GetNodeList(struct __amx *amx);
SceneWindow *GetSceneWindow(struct __amx *amx);
SceneWindow *GetSceneWindow(Object *obj);
Script *GetSceneScript(SceneWindow *win);

#endif
