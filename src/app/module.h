/*
 * Module: registry of the dynamically loaded (dlopen) modules that provide
 * AMX native functions. Modules are reference counted by name.
 */

#ifndef MODULE_H
#define MODULE_H

#include <string>

struct __amx;
struct tagAMX_NATIVE_INFO;

class Module {
public:
	static void Init();
	static void Exit();
	static void Load(const string &name);
	static void Release(const string &name);
	static int Regist(__amx *amx, const string &name);

	Module(const string &name);
	~Module();

	/* 0x00 */ string name;
	/* 0x04 */ int refcount;
	/* 0x08 */ void *handle;  /* dlopen handle */
	/* 0x0C */ void (*init_fn)();  /* module "Init", called after loading */
	/* 0x10 */ void (*exit_fn)();  /* module "Exit", called before unloading */
	/* 0x14 */ tagAMX_NATIVE_INFO *natives;  /* module "FuncTable", registered by Regist */
};

#endif
