/*
 * Module methods: loading and unloading through a thDLModule thread, and the
 * global registry with its lookup by name.
 */

#include "module.h"

#include <vector>
#include <list>

#include "amx.h"

#include <pthread.h>

#include "thdlmodule.h"

/* Scoped lock on the registry mutex. */
class Lock {
public:
	Lock(pthread_mutex_t *m) : m_mutex(m) { pthread_mutex_lock(m_mutex); }
	~Lock() { pthread_mutex_unlock(m_mutex); }

	pthread_mutex_t *m_mutex;
};

/* Registry of the loaded modules and its mutex. */
static vector<Module *> g_modules;
static pthread_mutex_t g_mutex;

/* Loads "/opt0/bn/modules/<name>.so" through a thDLModule thread
   (THDL_CMD_OPEN), waits for it, resolves the "Init", "Exit" and "FuncTable" entry
   points and calls Init. */
Module::Module(const string &n) : name(n), refcount(1), handle(0), init_fn(0), exit_fn(0), natives(0)
{
	_command_info info;
	info.cmd = THDL_CMD_OPEN;
	info.param = string("/opt0/bn/modules/") + name + ".so";

	thDLModule *dl = new thDLModule(0, 0);
	dl->setCommand(info);
	dl->start();
	pthread_join(dl->m_tid, 0);

	handle = dl->getHandle();
	init_fn = (void (*)())dl->getSymbolPtr("Init");
	exit_fn = (void (*)())dl->getSymbolPtr("Exit");
	natives = (tagAMX_NATIVE_INFO *)dl->getSymbolPtr("FuncTable");
	dl->release();

	if (init_fn)
		init_fn();
}

/* Calls the module Exit, then unloads it through a thDLModule thread
   (THDL_CMD_CLOSE) and waits for it. */
Module::~Module()
{
	if (exit_fn)
		exit_fn();

	_command_info info;
	info.cmd = THDL_CMD_CLOSE;

	thDLModule *dl = new thDLModule(0, 0);
	dl->setHandle(handle);
	dl->setCommand(info);
	dl->start();
	pthread_join(dl->m_tid, 0);
	dl->release();
}

void Module::Init()
{
	pthread_mutex_init(&g_mutex, 0);
	Lock lk(&g_mutex);
	g_modules.clear();
}

void Module::Exit()
{}

void Module::Load(const string &name)
{
	Lock lk(&g_mutex);
	for (vector<Module *>::iterator i = g_modules.begin(); i != g_modules.end(); ++i) {
		if ((*i)->name == name) {
			(*i)->refcount++;
			return;
		}
	}
	Module *m = new Module(name);
	g_modules.push_back(m);
}

void Module::Release(const string &name)
{
	Lock lk(&g_mutex);
	for (vector<Module *>::iterator i = g_modules.begin(); i != g_modules.end(); ++i) {
		if ((*i)->name == name) {
			(*i)->refcount--;
			if ((*i)->refcount <= 0) {
				delete *i;
				g_modules.erase(i);
			}
			return;
		}
	}
}

int Module::Regist(__amx *amx, const string &name)
{
	Lock lk(&g_mutex);
	for (vector<Module *>::iterator i = g_modules.begin(); i != g_modules.end(); ++i) {
		if ((*i)->name == name) {
			if ((*i)->natives == 0)
				return 19;  /* AMX_ERR_NOTFOUND */
			return amx_Register(amx, (*i)->natives, -1);
		}
	}
	return 19;  /* AMX_ERR_NOTFOUND */
}
