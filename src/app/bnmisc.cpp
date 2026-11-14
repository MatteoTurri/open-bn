/*
 * Creation of the sce_movie table in the media database, run with root
 * privileges.
 */

#include <string>
#include "bnmisc.h"
#include <list>

#include <unistd.h>
#include "lib/resuid.h"
#include "thread/commandinfo.h"
#include "thread/thposixthread.h"
#include "fs/thdbaccess.h"

static int g_mtUidRc;
static uid_t g_mtRuid;
static uid_t g_mtEuid;
static uid_t g_mtSuid;

/* Raises the process to root, saving the three uids; the result is kept in
   a global. */
static int bn_movie_table_reset(void)
{
	g_mtUidRc = getresuid(&g_mtRuid, &g_mtEuid, &g_mtSuid);
	if (g_mtUidRc == 0)
		g_mtUidRc = setresuid(0, 0, 0);
	return g_mtUidRc;
}

/* Restores the uids saved by bn_movie_table_reset. */
static int bn_movie_table_finish(void)
{
	g_mtUidRc = setresuid(g_mtRuid, g_mtEuid, g_mtSuid);
	return g_mtUidRc;
}

/* Creates the sce_movie table of movie.db unless a SELECT shows it already
   exists (returns 1); otherwise drops and recreates it and returns 0. Each
   step is a full run of a thDBAccess thread: open, exec, close. */
static int bn_movie_table_build(void)
{
	thDBAccess *db = new thDBAccess(0, 0);
	_command_info info;

	db->m_commandList.clear();
	info.cmd = THDB_CMD_OPEN;
	info.param = "/opt1/database/sqlite/movie.db";
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_EXEC;
	info.param = "select uri from sce_movie";
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_CLOSE;
	info.param = "";
	db->m_commandList.push_back(info);
	db->start();
	pthread_join(db->m_tid, 0);
	if (db->m_resultStatus == 1 || db->m_resultStatus == 2) {
		db->release();
		return 1;
	}

	db->m_commandList.clear();
	info.cmd = THDB_CMD_OPEN;
	info.param = "/opt1/database/sqlite/movie.db";
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_EXEC;
	info.param = "drop table sce_movie";
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_CLOSE;
	info.param = "";
	db->m_commandList.push_back(info);
	db->start();
	pthread_join(db->m_tid, 0);

	db->m_commandList.clear();
	info.cmd = THDB_CMD_OPEN;
	info.param = "/opt1/database/sqlite/movie.db";
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_EXEC;
	info.param = "create table sce_movie(";
	info.param += "title varchar,";
	info.param += "album_name varchar,";
	info.param += "create_date varchar,";
	info.param += "uri varchar,";
	info.param += "album_id varchar,";
	info.param += "content_type int,";
	info.param += "content_subtype int,";
	info.param += "content_attr int,";
	info.param += "table_version int";
	info.param += ")";
	db->m_commandList.push_back(info);
	info.cmd = THDB_CMD_CLOSE;
	info.param = "";
	db->m_commandList.push_back(info);
	db->start();
	pthread_join(db->m_tid, 0);
	db->release();
	return 0;
}

int bnCreateMovieTable(void)
{
	int table;

	bn_movie_table_reset();
	table = bn_movie_table_build();
	bn_movie_table_finish();
	return table;
}
