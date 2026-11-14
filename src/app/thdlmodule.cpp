/*
 * thDLModule: command queue thread for loading and unloading dynamic modules.
 */

#define THDLMODULE_IMPLEMENTATION
#include "thdlmodule.h"
#include "jobinit2.h"

thDLModule::~thDLModule()
{}

void thDLModule::init()
{
	m_currentCmd = 0;
	m_currentParam.erase(m_currentParam.begin(), m_currentParam.end());
	m_commandList.clear();
	m_handle = 0;
}

/* Runs the queued commands in order (the queue is not consumed): THDL_CMD_OPEN,
   THDL_CMD_CLOSE; anything else counts as success. Stops at the first command
   that returns 0, or when the thread is aborted. */
int thDLModule::threadProc()
{
	m_lastResult = 0;
	if (m_abortFlag)
		return 0;

	m_busy = 1;
	list<_command_info>::iterator it = m_commandList.begin();
	for (; it != m_commandList.end(); it++) {
		m_currentCmd = it->cmd;
		m_currentParam = it->param;

		switch (m_currentCmd) {
		case THDL_CMD_OPEN:
			m_lastResult = dlOpen();
			break;
		case THDL_CMD_CLOSE:
			m_lastResult = dlClose();
			InitInterupt();
			break;
		case THDL_CMD_NONE:  /* same as an unknown command */
		default:
			m_lastResult = 1;
			break;
		}

		if (m_lastResult == 0) {
			m_busy = 0;
			return 0;
		}
		if (m_abortFlag)
			break;
	}
	m_busy = 0;
	return 0;
}

/* Calls the completion callback with (m_userData, this). */
int thDLModule::threadFinished()
{
	if (m_abortFlag)
		return 1;
	if (m_userData == 0)
		return 1;
	if (m_completionCb == 0)
		return 1;
	return m_completionCb(m_userData, this) != 0;
}

/* Opens the module named by the current command parameter. */
int thDLModule::dlOpen()
{
	m_pathPtr = m_currentParam.c_str();

	dlerror();
	void *h = dlopen(m_pathPtr, RTLD_NOW | RTLD_GLOBAL);
	m_handle = h;
	return h != 0;
}

int thDLModule::dlClose()
{
	if (m_handle == 0)
		return 0;
	dlclose(m_handle);
	return 1;
}

void *thDLModule::getSymbolPtr(const string &name)
{
	if (m_handle == 0)
		return 0;
	if (name.length() == 0)
		return 0;

	dlerror();
	return dlsym(m_handle, name.c_str());
}

thDLModule::thDLModule(void *userData, Callback cb) : m_userData(userData), m_completionCb(cb)
{
	init();
}

void thDLModule::setCommand(_command_info &cmd)
{
	m_commandList.clear();
	m_commandList.push_back(cmd);
}

/* ---- command queue ---------------------------------------------------- */
void thDLModule::clearCommandList()
{
	m_commandList.clear();
}

void thDLModule::addCommandList(_command_info &cmd)
{
	m_commandList.push_back(cmd);
}

void thDLModule::setHandle(void *h)
{
	m_handle = h;
}

void *thDLModule::getHandle()
{
	return m_handle;
}

void thDLModule::abortProc()
{}

void thDLModule::releaseProc()
{}
