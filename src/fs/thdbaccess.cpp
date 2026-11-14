/*
 * thDBAccess: worker thread running SQLite commands on the photo and movie
 * databases, and the copy of a modified database to its PFS mirror.
 */

#define THDBACCESS_IMPLEMENTATION
#include "thdbaccess.h"
#include <iostream.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "../../ref/sqlite-2.7.2/gen/sqlite.h"
#include "bbfile.h"

int thDBAccess::BusyCounter::instance_count = 0;

/* Per-database state (A: photo.db, B: movie.db): mutex initialization flags
   and the copy mutexes taken by CopyDBImageToPfs. */
static int s_semaInitA = 0;
static int s_semaInitB = 0;

static pthread_mutex_t s_copyMutexA;
static pthread_mutex_t s_copyMutexB;

thDBAccess::thDBAccess(void *userData, bool (*completionCb)(void *, void *))
        : m_userData(userData), m_completionCb((int (*)(void *, thDBAccess *))completionCb)
{
	init();
}

/* Closes the database if still open, then frees the error message and the name. */
thDBAccess::~thDBAccess()
{
	if (m_db != 0) {
		m_closingDb = m_db;
		m_db = 0;
		dbClose();
	}
	if (m_errMsg) {
		free(m_errMsg);
		m_errMsg = 0;
	}
	if (m_dbName) {
		delete[] m_dbName;
	}
}

void thDBAccess::init()
{
	m_currentCmd = 0;
	m_currentParam.erase(m_currentParam.begin(), m_currentParam.end());
	m_commandList.clear();
	m_connectionStatus = 1;
	m_dbName = 0;
	m_resultStatus = 0;
	m_db = 0;
	m_rc = 0;
	m_clearOnNextRow = 1;
	m_rowCount = 0;
	m_errMsg = 0;
	m_dirty = 0;
}

/* Runs the queued commands in order until one fails or the thread is aborted:
   see THDB_CMD_*; any other value just succeeds. The commands stay in the queue. */
int thDBAccess::threadProc()
{
	BusyCounter counter;
	m_success = 0;
	if (m_abortFlag != 0)
		return 0;

	m_busy = 1;
	for (list<_command_info>::iterator it = m_commandList.begin(); it != m_commandList.end(); it++) {
		{
			m_currentCmd = (*it).cmd;
			m_currentParam = (*it).param;
			switch (m_currentCmd) {
			case THDB_CMD_OPEN:
				m_success = dbOpen();
				break;
			case THDB_CMD_CLOSE:
				if (m_db != 0) {
					m_closingDb = m_db;
					m_db = 0;
					m_success = dbClose();
				} else {
					m_success = 0;
				}
				break;
			case THDB_CMD_EXEC:
				m_success = dbExec();
				break;
			case THDB_CMD_TRBEGIN:
				m_success = dbTrBegin();
				break;
			case THDB_CMD_TREND:
				m_success = dbTrEnd();
				break;
			case THDB_CMD_TRABORT:
				m_success = dbTrAbort();
				break;
			case THDB_CMD_NONE:
			default:
				m_success = 1;
				break;
			}
			if (!m_success || m_abortFlag != 0)
				break;
		}
	}

	m_busy = 0;
	return 0;
}

int thDBAccess::threadFinished()
{
	if (m_abortFlag != 0)
		return 1;
	if (m_userData == 0)
		return 1;
	if (m_completionCb == 0)
		return 1;
	return m_completionCb(m_userData, this) != 0;
}

/* Opens the database named by the current command parameter (kept in m_dbName). */
int thDBAccess::dbOpen()
{
	struct timeval startTime;
	gettimeofday(&startTime, 0);

	m_dbName = new char[m_currentParam.length() + 1];
	const char *src = m_currentParam.c_str();
	strcpy(m_dbName, src);

	if (m_errMsg) {
		free(m_errMsg);
		m_errMsg = 0;
	}
	m_db = sqlite_open(m_dbName, 0666, &m_errMsg);
	if (m_db == 0) {
		m_connectionStatus = 1;
		m_rc = -1;
		return 0;
	}
	m_connectionStatus = 0;
	struct timeval endTime;
	gettimeofday(&endTime, 0);
	return 1;
}

/* Closes the database handed over in m_closingDb and, if it was modified,
   copies it to PFS; m_dirty is cleared once no copy is pending. */
int thDBAccess::dbClose()
{
	struct timeval startTime;
	gettimeofday(&startTime, 0);
	if (m_closingDb == 0)
		return 0;
	sqlite_close(m_closingDb);
	m_closingDb = 0;
	if (m_dirty) {
		CopyDBImageToPfs(m_dbName);
		if (GetModifedDBCounter(m_dbName) == 0)
			m_dirty = 0;
	}
	struct timeval endTime;
	gettimeofday(&endTime, 0);
	return 1;
}

/* Runs the SQL in the current command parameter; rows arrive through
   sqlite_callback. An INSERT, DELETE or UPDATE marks the database dirty and
   counts one pending modification. */
int thDBAccess::dbExec()
{
	struct timeval startTime, endTime;
	gettimeofday(&startTime, 0);

	if (m_db == 0)
		return 0;
	if (m_currentParam.length() == 0)
		return 0;

	m_clearOnNextRow = 1;
	m_results.clear();
	m_rowCount = 0;

	m_clearOnNextRow = 0;
	if (m_errMsg) {
		free(m_errMsg);
		m_errMsg = 0;
	}

	const char *sql = m_currentParam.c_str();

	m_rc = sqlite_exec(m_db, sql, sqlite_callback, this, &m_errMsg);
	if (m_rc != 0) {
		m_clearOnNextRow = 0;
		m_resultStatus = 7;
		sqlite_close(m_db);
		m_db = 0;
		return 0;
	}

	const char *keyword[3] = { "INSERT", "DELETE", "UPDATE" };

	const char *trimmed = m_currentParam.c_str();
	if (*trimmed == ' ') { /* redundant as written */
		do {
			trimmed++;
		} while (*trimmed == ' ');
	}

	for (unsigned int i = 0; i < sizeof(keyword) / sizeof(keyword[0]); i++) {
		if (strncasecmp(trimmed, keyword[i], strlen(keyword[i])) == 0) {
			if (!m_dirty) {
				CopySemaInit(m_dbName);
				ModifedDBCounterInc(m_dbName);
			}
			m_dirty = 1;
		}
	}

	if (m_clearOnNextRow)
		m_resultStatus = 1;
	else
		m_resultStatus = 2;
	m_clearOnNextRow = 0;
	gettimeofday(&endTime, 0);
	return 1;
}

/* Pending modification counters of the two databases. */
static volatile unsigned short s_photoDbModCount = 0;
static volatile unsigned short s_movieDbModCount = 0;

/* Transactions. On failure the error is logged, m_resultStatus becomes 7 and
   the database is closed. The gettimeofday results are never read. */
int thDBAccess::dbTrBegin()
{
	struct timeval startTime;
	gettimeofday(&startTime, 0);
	if (m_db == 0)
		return 0;
	if (m_errMsg) {
		free(m_errMsg);
		m_errMsg = 0;
	}
	m_rc = sqlite_exec(m_db, "begin;", sqlite_callback, this, &m_errMsg);
	if (m_rc != 0) {
		cout << "Could not begin Transaction: " << m_errMsg << endl;
		m_resultStatus = 7;
		sqlite_close(m_db);
		m_db = 0;
		return 0;
	}
	m_resultStatus = 1;
	struct timeval endTime;
	gettimeofday(&endTime, 0);
	return 1;
}

int thDBAccess::dbTrEnd()
{
	struct timeval startTime;
	gettimeofday(&startTime, 0);
	if (m_db == 0)
		return 0;
	if (m_errMsg) {
		free(m_errMsg);
		m_errMsg = 0;
	}
	m_rc = sqlite_exec(m_db, "end", sqlite_callback, this, &m_errMsg);
	if (m_rc != 0) {
		cout << "Could not finish Transaction: " << m_errMsg << endl;
		m_resultStatus = 7;
		sqlite_close(m_db);
		m_db = 0;
		return 0;
	}
	m_resultStatus = 1;
	struct timeval endTime;
	gettimeofday(&endTime, 0);
	return 1;
}

int thDBAccess::dbTrAbort()
{
	struct timeval startTime;
	gettimeofday(&startTime, 0);
	if (m_db == 0)
		return 0;
	if (m_errMsg) {
		free(m_errMsg);
		m_errMsg = 0;
	}
	m_rc = sqlite_exec(m_db, "abort", sqlite_callback, this, &m_errMsg);
	if (m_rc != 0) {
		cout << "Could not abort Transaction: " << m_errMsg << endl;
		m_resultStatus = 7;
		sqlite_close(m_db);
		m_db = 0;
		return 0;
	}
	m_resultStatus = 1;
	struct timeval endTime;
	gettimeofday(&endTime, 0);
	return 1;
}

int thDBAccess::getNumTuples()
{
	if (!m_db)
		return -1;
	return m_rowCount;
}

int thDBAccess::getNumFields()
{
	if (!m_db)
		return -1;
	if (m_results.size() == 0)
		return -1;
	return m_results[0].fields.size();
}

const char *thDBAccess::getValue(int row, int col)
{
	if (m_db == 0) {
fail:
		return 0;
	}
	if (row < 0)
		goto fail;
	if ((unsigned int)row >= m_results.size())
		goto fail;
	if (col < 0)
		goto fail;
	{
		sqlite_record &r = m_results[row];
		if ((unsigned int)col >= r.fields.size())
			goto fail;
		return r.fields[col].c_str();
	}
}

/* sqlite_exec row callback (arg is the thDBAccess): appends the row to
   m_results, clearing them first when m_clearOnNextRow is set. */
int thDBAccess::sqlite_callback(void *arg, int nCol, char **argv, char **colv)
{
	thDBAccess *db = (thDBAccess *)arg;
	int i;

	if (db == 0)
		return -1;

	if (db->m_clearOnNextRow != 0) {
		db->m_results.erase(db->m_results.begin(), db->m_results.end());
		db->m_rowCount = 0;
		db->m_clearOnNextRow = 0;
	}

	string s;
	sqlite_record row;

	if (argv != 0) {
		for (i = 0; i < nCol; i++) {
			if (argv[i] != 0)
				s = argv[i];
			else
				s = "";
			row.fields.push_back(s);
		}
	}
	db->m_results.push_back(row);
	db->m_rowCount++;
	return 0;
}

/* ---- per-database counters, keyed by database name ---- */
/* The counters are shared between threads, hence volatile. */
void thDBAccess::ModifedDBCounterInc(const char *name)
{
	if (strcmp(name, "/opt1/database/sqlite/photo.db") == 0) {
		s_photoDbModCount++;
		return;
	}
	if (strcmp(name, "/opt1/database/sqlite/movie.db") == 0) {
		s_movieDbModCount++;
	}
}

void thDBAccess::ModifedDBCounterDec(const char *name)
{
	if (strcmp(name, "/opt1/database/sqlite/photo.db") == 0) {
		s_photoDbModCount--;
		return;
	}
	if (strcmp(name, "/opt1/database/sqlite/movie.db") == 0) {
		s_movieDbModCount--;
	}
}

int thDBAccess::GetModifedDBCounter(const char *name)
{
	if (strcmp(name, "/opt1/database/sqlite/photo.db") == 0)
		return s_photoDbModCount;
	if (strcmp(name, "/opt1/database/sqlite/movie.db") == 0)
		return s_movieDbModCount;
	return 0;
}

void thDBAccess::CopySemaInit(const char *name)
{
	if (strcmp(name, "/opt1/database/sqlite/photo.db") == 0) {
		if (s_semaInitA == 0) {
			s_semaInitA = 1;
			pthread_mutex_init(&s_copyMutexA, 0);
		}
	} else if (strcmp(name, "/opt1/database/sqlite/movie.db") == 0) {
		if (s_semaInitB == 0) {
			s_semaInitB = 1;
			pthread_mutex_init(&s_copyMutexB, 0);
		}
	}
}

/* Copies a database file to the PFS mirror "pfs:__contents/contents/database"
   through a temporary file with a "_" prefix, renamed on success. Copies only
   when exactly one modification is pending and more than 128 MiB stay free;
   drops one pending modification on exit. */
void thDBAccess::CopyDBImage(const char *name)
{
	if (GetModifedDBCounter(name) == 1) {
		int err = 0;
		unsigned long long freeSize = 0;
		string dst;
		string tmpPath;
		struct stat st;
		struct timeval startTime;
		struct timeval endTime;

		gettimeofday(&startTime, 0);

		if (BBlstatCPP("pfs:__contents/contents/database", &st) < 0)
			BBmkdirCPP("pfs:__contents/contents/database", 511);

		dst = "pfs:__contents/contents/database" + string(strrchr(name, '/'));
		tmpPath = dst;
		tmpPath.insert(tmpPath.rfind('/') + 1, "_");

		/* NOTE: bug: returns without ModifedDBCounterDec. */
		if (stat(name, &st) < 0)
			return;

		unsigned int srcSize = st.st_size;
		unsigned int dstSize = BBlstatCPP(dst.c_str(), &st) < 0 ? 0 : (unsigned int)st.st_size;

		BBgetPFSPartitionFreeSizeCPP("pfs:__contents", &freeSize);

		if (freeSize - ((unsigned long long)srcSize - (unsigned long long)dstSize) > 0x8000000) {
			FILE *fp = fopen(name, "r");
			int fd = BBopenCPP(tmpPath.c_str(), 257, 436);
			char buf[1024];
			int n;
			while ((n = fread(buf, 1, 1024, fp)) != 0) {
				if (BBwriteCPP(fd, buf, n) < 0) {
					err = 1;
					break;
				}
			}
			BBcloseCPP(fd);
			fclose(fp);
		} else {
			err = 1;
		}

		if (err) {
			BBremoveCPP(dst.c_str());
			BBremoveCPP(tmpPath.c_str());
		} else {
			BBrenameCPP(tmpPath.c_str(), dst.c_str());
		}
		gettimeofday(&endTime, 0);
	}
	ModifedDBCounterDec(name);
}

void thDBAccess::CopyDBImageToPfs(const char *name)
{
	if (strcmp(name, "/opt1/database/sqlite/photo.db") == 0) {
		pthread_mutex_lock(&s_copyMutexA);
		CopyDBImage(name);
		pthread_mutex_unlock(&s_copyMutexA);
	} else if (strcmp(name, "/opt1/database/sqlite/movie.db") == 0) {
		pthread_mutex_lock(&s_copyMutexB);
		CopyDBImage(name);
		pthread_mutex_unlock(&s_copyMutexB);
	}
}

void thDBAccess::setCommand(_command_info &cmd)
{
	m_commandList.clear();
	m_commandList.push_back(cmd);
}

/* ---- command queue ---- */
void thDBAccess::clearCommandList()
{
	m_commandList.clear();
}

void thDBAccess::addCommandList(_command_info &cmd)
{
	m_commandList.push_back(cmd);
}

int thDBAccess::IsError()
{
	return m_success ^ 1;
}

/* ---- status and results ---- */
int thDBAccess::getConnectionStatus()
{
	return m_connectionStatus;
}

int thDBAccess::getResultStatus()
{
	return m_resultStatus;
}

/* ---- thread lifetime ---- */
void thDBAccess::abortProc()
{}

void thDBAccess::releaseProc()
{}

int thDBAccess::BusyCount()
{
	return BusyCounter::instance_count;
}
