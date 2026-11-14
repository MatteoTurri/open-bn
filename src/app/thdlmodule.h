/*
 * thDLModule: a thread that loads and unloads a dynamic module (dlopen and
 * dlclose) by running a queue of commands.
 */

#ifndef THDLMODULE_H
#define THDLMODULE_H

#include <string>
#include <list>

#include "thread/thposixthread.h"

#include "thread/commandinfo.h"

#include <dlfcn.h>

/* Commands of a thDLModule queue (_command_info::cmd). */
enum {
	THDL_CMD_NONE,
	THDL_CMD_OPEN, /* dlopen the module named by the parameter */
	THDL_CMD_CLOSE /* dlclose the handle set with setHandle */
};

class thDLModule : public thPosixThread {
public:
	typedef bool (*Callback)(void *, void *);

	thDLModule(void *userData, Callback cb);
	virtual ~thDLModule();

	/* 0x20 */ void *m_userData;
	/* 0x24 */ Callback m_completionCb;
	/* 0x28 */ int m_currentCmd;
	/* 0x2C */ string m_currentParam;  /* parameter of the current command (module path for dlOpen) */
	/* 0x30 */ list<_command_info> m_commandList;
	/* 0x34 */ const char *m_pathPtr;  /* c_str() of the module path passed to dlopen */
	/* 0x38 */ void *m_handle;
	/* 0x3C */ int m_lastResult;  /* result of the last command */
	/* 0x40 */ int m_busy;  /* 1 while threadProc runs the queue */

	void init();

	int threadProc();
	int threadFinished();

	void setCommand(_command_info &cmd);
	void addCommandList(_command_info &cmd);
	void clearCommandList();

	void setHandle(void *h);
	void *getHandle();

	void abortProc();
	void releaseProc();

	int dlOpen();
	int dlClose();
	void *getSymbolPtr(const string &name);
};

/* thdlmodule.cpp defines THDLMODULE_IMPLEMENTATION and provides the
   out-of-line copies. */
#ifndef THDLMODULE_IMPLEMENTATION
inline thDLModule::thDLModule(void *userData, Callback cb) : m_userData(userData), m_completionCb(cb)
{
	init();
}

inline void thDLModule::setCommand(_command_info &cmd)
{
	m_commandList.clear();
	m_commandList.push_back(cmd);
}

inline void thDLModule::setHandle(void *h)
{
	m_handle = h;
}

inline void *thDLModule::getHandle()
{
	return m_handle;
}
#endif

#endif
