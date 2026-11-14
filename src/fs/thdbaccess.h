/*
 * thDBAccess: worker thread running SQLite commands (open, close, exec,
 * transactions) queued by the caller; results are kept as rows of strings.
 */

#ifndef THDBACCESS_H
#define THDBACCESS_H

#include <list>
#include <string>
#include <vector>
#include "thread/thposixthread.h"
#include "thread/commandinfo.h"

typedef struct sqlite sqlite;

/* One result row: the column values. */
struct sqlite_record {
	vector<string> fields;
};

/* Commands of a thDBAccess queue (_command_info::cmd). */
enum {
	THDB_CMD_NONE,
	THDB_CMD_OPEN,     /* open the database file named by the parameter */
	THDB_CMD_CLOSE,    /* close the open database */
	THDB_CMD_EXEC,     /* run the SQL statement in the parameter */
	THDB_CMD_TRBEGIN,  /* begin a transaction */
	THDB_CMD_TREND,    /* commit the transaction */
	THDB_CMD_TRABORT   /* roll the transaction back */
};

class thDBAccess : public thPosixThread {
public:
	thDBAccess(void *userData, bool (*completionCb)(void *, void *));
	virtual ~thDBAccess();

	/* 0x20 */ void *m_userData;
	/* 0x24 */ int (*m_completionCb)(void *, thDBAccess *);  /* called with (userData, this) */

	/* 0x28 */ int m_currentCmd;
	/* 0x2C */ string m_currentParam;
	/* 0x30 */ list<_command_info> m_commandList;

	/* 0x34 */ int m_connectionStatus;
	/* 0x38 */ int m_resultStatus;
	/* 0x3C */ int m_success;
	/* 0x40 */ int m_busy;
	/* 0x44 */ char *m_dbName;  /* allocated by dbOpen */
	/* 0x48 */ sqlite *m_db;
	/* 0x4C */ sqlite *m_closingDb;  /* handle being closed by dbClose */
	/* 0x50 */ int m_rc;
	/* 0x54 */ char *m_errMsg;
	/* 0x58 */ int m_clearOnNextRow;
	/* 0x5C */ int m_rowCount;
	/* 0x60 */ vector<sqlite_record> m_results;
	/* 0x6C */ int m_dirty;  /* database modified, copy to PFS pending */

	/* --- database access --- */
	void init();
	int dbOpen();
	int dbClose();
	int dbExec();
	virtual int threadProc();
	static int sqlite_callback(void *arg, int nCol, char **argv, char **colv);

	static void CopyDBImage(const char *path);

	static void CopyDBImageToPfs(const char *name);

	/* --- thread lifetime --- */
	virtual void abortProc();
	virtual void releaseProc();
	virtual int threadFinished();

	/* --- commands --- */
	void setCommand(_command_info &cmd);
	void addCommandList(_command_info &cmd);
	void clearCommandList();

	/* --- status and results --- */
	int getConnectionStatus();
	int getResultStatus();
	int IsError();
	static int BusyCount();

	int getNumTuples();
	int getNumFields();
	const char *getValue(int row, int col);

	/* --- transactions --- */
	int dbTrBegin();
	int dbTrEnd();
	int dbTrAbort();

	/* --- per-database counters, keyed by database name --- */
	static void ModifedDBCounterInc(const char *name);
	static void ModifedDBCounterDec(const char *name);
	static int GetModifedDBCounter(const char *name);
	static void CopySemaInit(const char *name);

	/* Number of threads running commands, read by BusyCount(); threadProc holds
	   one instance while it runs. */
	class BusyCounter {
	public:
		BusyCounter() { instance_count++; }
		~BusyCounter() { instance_count--; }
		static int instance_count;
	};
};

/* thdbaccess.cpp defines THDBACCESS_IMPLEMENTATION and provides the
   out-of-line copy. */
#ifndef THDBACCESS_IMPLEMENTATION
inline void thDBAccess::setCommand(_command_info &cmd)
{
	m_commandList.clear();
	m_commandList.push_back(cmd);
}
#endif

#endif
