/*
 * Small/amx natives "dat*" (database and file list queries, run on a
 * thDBAccess or thFileList thread) and "aval*" (configuration variables),
 * table smallext_dat. Each script keeps its DatData under the tag "_DAT".
 */

#include "amx.h"
#include "natives/smallext.h"

#include <stdio.h>
#include <stdlib.h>

#include <iostream.h>
#include <string>
#include <list>
#include <vector>

#include "fs/bbfile.h"

#include "script/script.h"
#include "thread/commandinfo.h"
#include "fs/fmentry.h"
#include "thread/thposixthread.h"
#include "fs/thdbaccess.h"
#include "fs/thfilelist.h"
#include "app/variables.h"

static const long TAG_PSCR = 0x52435350;        /* "PSCR" */
static const unsigned int TAG_DAT = 0x5441445f; /* "_DAT" */

/* State block hung on the Script as user data "_DAT". */
class DatData {
public:
	/* 0x00 */ int state;  /* 0 = closed (file list), 1..3 or 99 = database type */
	/* 0x04 */ thDBAccess *db;  /* a thFileList in state 0 */
	/* 0x08 */ int lastOp;  /* code of the last native called */
	/* 0x0C */ int num;
	/* 0x10 */ fmentry_order order;  /* sort order of the file list */
	/* 0x14 */ vector<fmEntry> rows;
	/* 0x20 */ vector<unsigned int> ints;

	static int instance_count;

	/* Stops the database thread and empties both tables; called by datClose and
	   by the destructor. */
	void stop()
	{
		switch (state) {
		case 0: {
			thDBAccess *p = db;

			if (p != 0) {
				p->m_abortFlag = 1;
				p->release();
			}
		}
			rows.clear();
			ints.clear();
			break;
		case 1:
		case 2:
		case 3:
		case 99: {
			_command_info info;
			thDBAccess *p;

			info.cmd = THDB_CMD_CLOSE;
			p = db;
			if (p != 0) {
				p->setCommand(info);
				p->start();
				p->release();
			}
			ints.clear();
		} break;
		}
	}

	DatData() { instance_count++; }

	~DatData()
	{
		instance_count--;
		stop();
	}
};

int DatData::instance_count = 0;

/* True when [p,q] lies entirely in the script heap or entirely in its stack. */
static inline int amx_addr_ok(AMX *amx, cell p, cell q)
{
	if (p >= 0 && p < amx->hea && q >= 0 && q < amx->hea)
		return 1;
	if (p >= amx->stk && p < amx->stp && q >= amx->stk && q < amx->stp)
		return 1;
	return 0;
}

/* The data state of a script, user data '_DAT'. */
static inline DatData *ScriptDatData(Script *owner)
{
	return (DatData *)owner->GetUserData(TAG_DAT);
}

/* Removes it from the script and returns it. */
static inline DatData *TakeScriptDatData(Script *owner)
{
	return (DatData *)owner->DelUserData(TAG_DAT);
}

/* The script that runs amx, user data 'PSCR' of the AMX. */
static inline Script *AmxScript(AMX *amx)
{
	Script *s;

	amx_GetUserData(amx, TAG_PSCR, (void **)&s);
	return s;
}

static inline DatData *AmxDat(AMX *amx)
{
	return ScriptDatData(AmxScript(amx));
}

/* Repair flag files, one per database (photo, music, movie). */
#define kRepairPhoto "linux:/opt0/bn/sysconf/db_repair_photo.dat"
#define kRepairMusic "linux:/opt0/bn/sysconf/db_repair_music.dat"
#define kRepairMovie "linux:/opt0/bn/sysconf/db_repair_movie.dat"

/* Destroy callback of the script: deletes its DatData. */
static void dat_dispose(AMX *amx)
{
	DatData *d = AmxDat(amx);

	if (d != 0)
		delete d;
}

/* Opens the module: params[1] is the type (0 = file list, 1/2/3/99 = a
   database), params[2] the name or URL, params[3] (optional) the order of the
   file list rows. Any previous DatData is stopped and deleted; the new one is
   hung on the Script with dat_dispose as destroy callback. The database path
   is "/opt1/database/sqlite/%s.db", with %s replaced by hand. */
static cell AMX_NATIVE_CALL n_datOpen(AMX *amx, cell *params)
{
	DatData *d = AmxDat(amx);
	int type = params[1];
	string name = CellToString(amx, params[2]);
	int order = 3;

	if (params[0] / sizeof(cell) == 3)
		order = params[3];

	DatData *nd = new DatData;

	if (nd == 0)
		return -1;
	nd->lastOp = 1;
	switch (type) {
	case 0:
		if (d != 0) {
			{
				thDBAccess *p = d->db;

				if (p != 0) {
					p->m_abortFlag = 1;
					p->release();
				}
			}
			d->rows.clear();
			d->ints.clear();
			delete d;
		}
		switch (order) {
		case 0:
			nd->order = FMENTRY_ORDER_CTIME_LESSER;
			break;
		case 1:
			nd->order = FMENTRY_ORDER_CTIME_GREATER;
			break;
		case 2:
			nd->order = FMENTRY_ORDER_MTIME_LESSER;
			break;
		case 3:
			nd->order = FMENTRY_ORDER_MTIME_EQUAL_GREATER;
			break;
		case 4:
			nd->order = FMENTRY_ORDER_NAME_LESSER;
			break;
		case 5:
			nd->order = FMENTRY_ORDER_NAME_GREATER;
			break;
		case 6:
			nd->order = FMENTRY_ORDER_CTCOUNT_LESSER;
			break;
		case 7:
			nd->order = FMENTRY_ORDER_CTCOUNT_GREATER;
			break;
		case 8:
			nd->order = FMENTRY_ORDER_MTCOUNT_LESSER;
			break;
		case 9:
			nd->order = FMENTRY_ORDER_MTCOUNT_EQUAL_GREATER;
			break;
		default:
			nd->order = FMENTRY_ORDER_MTIME_EQUAL_GREATER;
			break;
		}
		{
			thFileList *fl = new thFileList(0, 0);

			fl->setUrl(name.c_str());
			fl->setNoParentDirFlag();
			fl->setEntryListOrder(nd->order);
			fl->start();
			nd->state = type;
			nd->db = (thDBAccess *)fl;
			nd->rows.clear();
			nd->ints.clear();
		}
		AmxScript(amx)->SetUserData(TAG_DAT, nd);
		AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_DAT, dat_dispose);
		break;
	case 1:
	case 2:
	case 3:
	case 99:
		if (d != 0) {
			_command_info info;
			thDBAccess *p;

			info.cmd = THDB_CMD_CLOSE;
			p = d->db;
			if (p != 0) {
				p->setCommand(info);
				p->start();
				p->release();
			}
			d->ints.clear();
			delete d;
		}
		{
			_command_info info;

			info.cmd = THDB_CMD_OPEN;
			if (name.length() != 0 && name != "bndb") {
				string &path = info.param;

				path = "/opt1/database/sqlite/%s.db";
				path.replace(path.find("%s"), 2, name);
			} else
				cout << "Error: DB name is not assigned" << endl;

			thDBAccess *db = new thDBAccess(0, 0);

			db->setCommand(info);
			db->start();
			nd->state = type;
			nd->db = db;
			nd->ints.clear();
			AmxScript(amx)->SetUserData(TAG_DAT, nd);
			AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_DAT, dat_dispose);
		}
		break;
	default:
		return -1;
	}
	return 0;
}

/* Stops the database and detaches the DatData from the Script, then deletes it. */
static cell AMX_NATIVE_CALL n_datClose(AMX *amx, cell *params)
{
	DatData *d = AmxDat(amx);

	if (d == 0)
		return -1;
	d->lastOp = 2;
	d->stop();
	{
		DatData *e;

		e = TakeScriptDatData(AmxScript(amx));
		if (e == 0)
			return 0;
		delete e;
	}
	return 0;
}

/* Starts a search: params[1] is the condition, params[2] the table. Without a
   database (state 0) the condition is a comma separated list of content types
   (image, music, ...) for a thFileList on URL params[2]; with a database it
   builds the SELECT (channel for photos, contents for music, the given table
   otherwise) and queues it to the database thread. */
static cell AMX_NATIVE_CALL n_datFind(AMX *amx, cell *params)
{
	DatData *d = AmxDat(amx);

	if (d == 0)
		return -1;
	d->lastOp = 3;
	string cond = CellToString(amx, params[1]);
	string table = CellToString(amx, params[2]);

	switch (d->state) {
	case 0: {
		thDBAccess *p = d->db;

		if (p != 0) {
			p->m_abortFlag = 1;
			p->release();
		}
	}
		{
			string type;
			vector<unsigned int> types;
			unsigned int pos;

			do {
				type = cond;
				pos = cond.find_first_of(",");
				if (pos != string::npos) {
					type.erase(pos);
					cond.erase(0, pos + 1);
				} else
					cond.erase();
				if (type == "image")
					types.push_back(256);
				else if (type == "music")
					types.push_back(257);
				else if (type == "movie")
					types.push_back(258);
				else if (type == "real")
					types.push_back(259);
				else if (type == "pss")
					types.push_back(260);
				else if (type == "ps2savedata")
					types.push_back(3);
				else if (type == "ps1savedata")
					types.push_back(4);
				else if (type == "game")
					types.push_back(6);
			} while (pos != string::npos || cond.length() != 0);

			if (types.size() == 0)
				return -1;

			thFileList *fl = new thFileList(0, 0);

			fl->setUrl(table.c_str());
			fl->setEntryListOrder(d->order);
			fl->setRecursiveFlag();
			for (unsigned int i = 0; i < types.size(); i++)
				fl->addFindContentsTypeList(types[i]);
			fl->start();
			d->db = (thDBAccess *)fl;
			d->rows.clear();
			d->ints.clear();
		}
		break;
	case 1: {
		_command_info info;
		thDBAccess *db = d->db;

		if (db == 0)
			return -1;
		db->m_commandList.clear();
		info.cmd = THDB_CMD_EXEC;
		info.param = "select * from channel";
		if (cond.length() != 0) {
			info.param += " where ";
			info.param += cond;
		}
		db->m_commandList.push_back(info);
		db->start();
	} break;
	case 2: {
		_command_info info;
		thDBAccess *db = d->db;

		if (db == 0)
			return -1;
		db->m_commandList.clear();
		info.cmd = THDB_CMD_EXEC;
		info.param = "select * from contents";
		if (table.length() != 0) {
			info.param += ",";
			info.param += table;
		}
		if (cond.length() != 0) {
			info.param += " where ";
			info.param += cond;
		}
		db->m_commandList.push_back(info);
		db->start();
	} break;
	case 3:
	case 99: {
		_command_info info;
		thDBAccess *db = d->db;

		if (db == 0)
			return -1;
		db->m_commandList.clear();
		info.cmd = THDB_CMD_EXEC;
		info.param = "select * from ";
		if (table.length() != 0)
			info.param += table;
		if (cond.length() != 0) {
			info.param += " where ";
			info.param += cond;
		}
		db->m_commandList.push_back(info);
		db->start();
	} break;
	default:
		return -1;
	}
	return 0;
}

/* Waits for the running operation and moves its result into the DatData: in
   state 0 (lastOp 1 = open, 3 = query) the rows of the file list are copied
   into both vectors and the thread released; with a database only ints is
   sized to the number of tuples. */
static cell AMX_NATIVE_CALL n_datSync(AMX *amx, cell *params)
{
	DatData *d = AmxDat(amx);

	if (d == 0)
		return -1;
	switch (d->state) {
	case 0: {
		thFileList *fl;
		int ret;

		switch (d->lastOp) {
		case 1:
			fl = (thFileList *)d->db;
			if (fl == 0)
				return 1;
			if (fl->m_abortFlag != 0)
				return 1;
			if (fl->IsRelease())
				return 1;
			if (!fl->IsEnd())
				return 0;
			d->num = fl->m_entryList.size();
			ret = fl->m_result;
			if (d->num > 0) {
				d->rows.reserve(d->num);
				d->ints.reserve(d->num);
				for (list<fmEntry>::iterator it = fl->m_entryList.begin(); it != fl->m_entryList.end();
				    it++) {
					d->rows.push_back(*it);
					d->ints.push_back(0);
				}
			}
			fl->release();
			d->db = 0;
			if (ret == 0)
				break;
			return ret;
		case 3:
			fl = (thFileList *)d->db;
			if (fl == 0)
				return 1;
			if (fl->m_abortFlag != 0)
				return 1;
			if (fl->IsRelease())
				return 1;
			if (!fl->IsEnd())
				return 0;
			d->num = fl->m_entryList.size();
			ret = fl->m_result;
			if (d->num > 0) {
				d->rows.reserve(d->num);
				d->ints.reserve(d->num);
				for (list<fmEntry>::iterator it = fl->m_entryList.begin(); it != fl->m_entryList.end();
				    it++) {
					d->rows.push_back(*it);
					d->ints.push_back(0);
				}
			}
			fl->release();
			d->db = 0;
			if (ret == 0)
				break;
			return ret;
		default:
			return 1;
		}
	} break;
	case 1:
	case 2:
	case 3:
	case 99: {
		int err;
		thDBAccess *db = d->db;

		if (db == 0)
			return -1;
		if (db->m_abortFlag != 0)
			return 1;
		if (db->IsRelease())
			return 1;
		if (!db->IsEnd())
			return 0;
		if (d->lastOp == 3 || d->lastOp == 8) {
			int i;

			d->num = db->getNumTuples();
			if (d->num > 0) {
				d->ints.reserve(d->num);
				for (i = 0; i < d->num; i++)
					d->ints.push_back(0);
			}
		}
		err = db->m_resultStatus;
		if (err < 8) {
			if (err >= 5) {
				fprintf(stderr, "DB datSync() Error [%d]!!\n", err);
				return -err;
			}
		}
		if (db->m_connectionStatus != 1)
			return 1;
		fprintf(stderr, "Could not establish Transction!!\n");
		return -1;
	}
	}
	return 1;
}

static cell AMX_NATIVE_CALL n_datGetNum(AMX *amx, cell *params)
{
	DatData *d = AmxDat(amx);

	if (d == 0)
		return -1;
	d->lastOp = 4;
	switch (d->state) {
	case 0:
		return d->num;
	case 1:
	case 2:
	case 3:
	case 99:
		return d->num;
	}
	return 0;
}

/* State 0 (no database) always has 9 fields. */
static cell AMX_NATIVE_CALL n_datGetNumFields(AMX *amx, cell *params)
{
	DatData *d = AmxDat(amx);

	if (d == 0)
		return -1;
	d->lastOp = 5;
	switch (d->state) {
	case 0:
		return 9;
	case 1:
	case 2:
	case 3:
	case 99:
		if (d->db == 0)
			return -1;
		return d->db->getNumFields();
	}
	return 0;
}

/* Local work object of datGetData and datUpdateData: a string at offset 8. */
struct DatWork {
	/* 0x00 */ int a;  /* unused, but part of the layout */
	/* 0x04 */ int b;  /* unused, but part of the layout */
	/* 0x08 */ string s;
};

/* datGetData(index, field, addr, size): copies field `field` of row `index` to
   AMX address params[3], params[4] cells long. State 0 reads the cached rows
   (9 fields), state 1 queries the database (14 fields), states 2/3/99 use the
   real number of fields. */
static cell AMX_NATIVE_CALL n_datGetData(AMX *amx, cell *params)
{
	DatData *d = AmxDat(amx);

	if (d == 0)
		return -1;
	d->lastOp = 6;
	{
		int index = params[1];
		int field = params[2];
		fmEntry entry;  /* unused, but part of the layout */
		DatWork value;  /* unused, but part of the layout */
		cell *cptr;
		thDBAccess *db;

		switch (d->state) {
		case 0:
			if (index >= d->num || index < 0)
				return -1;
			if (field >= 9 || field < 0)
				return -1;
			switch (field) {
			case 0:
				if ((unsigned int)(params[4] * 4) < 4) {
					amx_RaiseError(amx, 10);
					return 0;
				}
				if (amx_GetAddr(amx, params[3], &cptr) == 0)
					*cptr = d->ints[index];
				break;
			case 1: {
				string v = d->rows[index].name;

				if ((unsigned int)(params[4] * 4) < v.length() + 1)
					v = v.substr(0, params[4] * 4 - 1);
				if (!amx_addr_ok(amx, params[3], v.length() + params[3])) {
					amx_RaiseError(amx, 5);
					return 0;
				}
				amx_GetAddr(amx, params[3], &cptr);
				amx_SetString(cptr, (char *)v.c_str(), 1);
			} break;
			case 2: {
				string v = d->rows[index].comment;

				if ((unsigned int)(params[4] * 4) < v.length() + 1)
					v = v.substr(0, params[4] * 4 - 1);
				if (!amx_addr_ok(amx, params[3], v.length() + params[3])) {
					amx_RaiseError(amx, 5);
					return 0;
				}
				amx_GetAddr(amx, params[3], &cptr);
				amx_SetString(cptr, (char *)v.c_str(), 1);
			} break;
			case 3:
				if ((unsigned int)(params[4] * 4) < 4) {
					amx_RaiseError(amx, 10);
					return 0;
				}
				if (amx_GetAddr(amx, params[3], &cptr) == 0)
					*cptr = d->rows[index].unk_B8;
				break;
			case 4:
				if ((unsigned int)(params[4] * 4) < 4) {
					amx_RaiseError(amx, 10);
					return 0;
				}
				if (amx_GetAddr(amx, params[3], &cptr) == 0)
					*cptr = d->rows[index].type;
				break;
			case 5:
				if ((unsigned int)(params[4] * 4) < 4) {
					amx_RaiseError(amx, 10);
					return 0;
				}
				if (amx_GetAddr(amx, params[3], &cptr) == 0)
					*cptr = d->rows[index].devType;
				break;
			case 6:
				if ((unsigned int)(params[4] * 4) < 4) {
					amx_RaiseError(amx, 10);
					return 0;
				}
				if (amx_GetAddr(amx, params[3], &cptr) == 0)
					*cptr = d->rows[index].st.st_mode;
				break;
			case 7:
				if ((unsigned int)(params[4] * 4) < 4) {
					amx_RaiseError(amx, 10);
					return 0;
				}
				if (amx_GetAddr(amx, params[3], &cptr) == 0)
					*cptr = d->rows[index].attr;
				break;
			case 8:
				if ((unsigned int)(params[4] * 4) < 4) {
					amx_RaiseError(amx, 10);
					return 0;
				}
				if (amx_GetAddr(amx, params[3], &cptr) == 0)
					*cptr = d->rows[index].badName;
				break;
			}
			break;
		case 1:
			db = d->db;
			if (db == 0)
				return -1;
			if (db->m_abortFlag != 0)
				return -1;
			if (!db->IsEnd())
				return -1;
			if (index >= d->num || index < 0)
				return -1;
			switch (field) {
			default:
				return -1;
			case 0:
				if ((unsigned int)(params[4] * 4) < 4) {
					amx_RaiseError(amx, 10);
					return 0;
				}
				if (amx_GetAddr(amx, params[3], &cptr) == 0)
					*cptr = d->ints[index];
				break;
			case 1:
			case 5:
			case 8:
				if ((unsigned int)(params[4] * 4) < 4) {
					amx_RaiseError(amx, 10);
					return 0;
				}
				if (amx_GetAddr(amx, params[3], &cptr) == 0)
					*cptr = strtol(db->getValue(index, field - 1), 0, 10);
				break;
			case 2:
			case 3:
			case 4:
			case 6:
			case 7:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13: {
				string v = db->getValue(index, field - 1);

				if ((unsigned int)(params[4] * 4) < v.length() + 1)
					v = v.substr(0, params[4] * 4 - 1);
				if (!amx_addr_ok(amx, params[3], v.length() + params[3])) {
					amx_RaiseError(amx, 5);
					return 0;
				}
				amx_GetAddr(amx, params[3], &cptr);
				amx_SetString(cptr, (char *)v.c_str(), 1);
			} break;
			}
			break;
		case 2:
		case 3:
		case 99:
			db = d->db;
			if (db == 0)
				return -1;
			if (db->m_abortFlag != 0)
				return -1;
			if (!db->IsEnd())
				return -1;
			if (index >= d->num || index < 0)
				return -1;
			if (field >= db->getNumFields() + 1 || field < 0)
				return -1;
			{
				if (field == 0) {
					if ((unsigned int)(params[4] * 4) < 4) {
						amx_RaiseError(amx, 10);
						return 0;
					}
					if (amx_GetAddr(amx, params[3], &cptr) == 0)
						*cptr = d->ints[index];
					break;
				}
				{
					string v = db->getValue(index, field - 1);

					if ((unsigned int)(params[4] * 4) < v.length() + 1)
						v = v.substr(0, params[4] * 4 - 1);
					if (!amx_addr_ok(amx, params[3], v.length() + params[3])) {
						amx_RaiseError(amx, 5);
						return 0;
					}
					amx_GetAddr(amx, params[3], &cptr);
					amx_SetString(cptr, (char *)v.c_str(), 1);
				}
			}
			break;
		}
		return 0;
	}
}

/* Writes a field of row params[1]: params[2] chooses the integer (0, stored in
   d->ints) or, only with the database closed, the string (1, d->rows[i].name). */
static cell AMX_NATIVE_CALL n_datUpdateData(AMX *amx, cell *params)
{
	DatData *d = AmxDat(amx);

	if (d == 0)
		return -1;
	d->lastOp = 7;
	{
		cell *cptr;
		int index = params[1];
		int what = params[2];
		DatWork value;  /* unused, but part of the layout */

		switch (d->state) {
		case 0:
			if (index >= d->num || index < 0)
				return -1;
			if (what == 0) {
				if (amx_GetAddr(amx, params[3], &cptr) == 0)
					d->ints[index] = *cptr;
			} else if (what == 1) {
				d->rows[index].name = CellToString(amx, params[3]);
			}
			break;
		case 1:
		case 2:
		case 3:
		case 99:
			if (index >= d->num || index < 0)
				return -1;
			if (what == 0) {
				if (amx_GetAddr(amx, params[3], &cptr) == 0)
					d->ints[index] = *cptr;
			}
			break;
		}
		return 0;
	}
}

/* Queues the SQL string of the argument to the database thread and starts it. */
static cell AMX_NATIVE_CALL n_datSQLExec(AMX *amx, cell *params)
{
	DatData *d = AmxDat(amx);

	if (d == 0)
		return -1;
	d->lastOp = 8;
	{
		string sql = CellToString(amx, params[1]);

		switch (d->state) {
		case 1:
		case 2:
		case 3:
		case 99: {
			_command_info info;
			thDBAccess *db = d->db;

			if (db == 0)
				return -1;
			db->m_commandList.clear();
			info.cmd = THDB_CMD_EXEC;
			info.param = sql;
			db->m_commandList.push_back(info);
			db->start();
		} break;
		default:
			return -1;
		case 0:
			break;
		}
		return 0;
	}
}

/* datTRBegin, datTREnd and datTRAbort differ only in lastOp (9/10/11) and in
   the queued command (4/5/6, with an empty parameter). */
static cell AMX_NATIVE_CALL n_datTRBegin(AMX *amx, cell *params)
{
	DatData *d = AmxDat(amx);

	if (d == 0)
		return -1;
	d->lastOp = 9;
	switch (d->state) {
	case 0:
		return 0;
	default:
		return -1;
	case 1:
	case 2:
	case 3:
	case 99: {
		_command_info info;
		thDBAccess *db = d->db;

		if (db == 0)
			return -1;
		db->m_commandList.clear();
		info.cmd = THDB_CMD_TRBEGIN;
		info.param = "";
		db->m_commandList.push_back(info);
		db->start();
		return 0;
	}
	}
}

static cell AMX_NATIVE_CALL n_datTREnd(AMX *amx, cell *params)
{
	DatData *d = AmxDat(amx);

	if (d == 0)
		return -1;
	d->lastOp = 10;
	switch (d->state) {
	case 0:
		return 0;
	default:
		return -1;
	case 1:
	case 2:
	case 3:
	case 99: {
		_command_info info;
		thDBAccess *db = d->db;

		if (db == 0)
			return -1;
		db->m_commandList.clear();
		info.cmd = THDB_CMD_TREND;
		info.param = "";
		db->m_commandList.push_back(info);
		db->start();
		return 0;
	}
	}
}

static cell AMX_NATIVE_CALL n_datTRAbort(AMX *amx, cell *params)
{
	DatData *d = AmxDat(amx);

	if (d == 0)
		return -1;
	d->lastOp = 11;
	switch (d->state) {
	case 0:
		return 0;
	default:
		return -1;
	case 1:
	case 2:
	case 3:
	case 99: {
		_command_info info;
		thDBAccess *db = d->db;

		if (db == 0)
			return -1;
		db->m_commandList.clear();
		info.cmd = THDB_CMD_TRABORT;
		info.param = "";
		db->m_commandList.push_back(info);
		db->start();
		return 0;
	}
	}
}

/* Tells whether the repair flag file of database params[1] exists. */
static cell AMX_NATIVE_CALL n_datCheckRepairFlag(AMX *amx, cell *params)
{
	struct stat st;
	int type = params[1];
	int r;

	switch (type) {
	case 1:
		if (BBlstatCPP(kRepairPhoto, &st) != 0)
			type = 0;
		r = type;
		break;
	case 2:
		r = (BBlstatCPP(kRepairMusic, &st) == 0);
		break;
	case 3:
		r = (BBlstatCPP(kRepairMovie, &st) == 0);
		break;
	default:
		r = 0;
		break;
	}
	return r != 0;
}

/* Removes the repair flag file of database params[1]; returns the result of
   BBremoveCPP, 0 for an unknown database. */
static cell AMX_NATIVE_CALL n_datClearRepairFlag(AMX *amx, cell *params)
{
	switch (params[1]) {
	case 1:
		return BBremoveCPP(kRepairPhoto);
	case 2:
		return BBremoveCPP(kRepairMusic);
	case 3:
		return BBremoveCPP(kRepairMovie);
	}
	return 0;
}

/* Variables::SetVariable(name, value). */
static cell AMX_NATIVE_CALL n_avalSet(AMX *amx, cell *params)
{
	string tmpName = CellToString(amx, params[1]);
	string tmpValue = CellToString(amx, params[2]);
	string name = tmpName;
	string value = tmpValue;

	Variables::SetVariable(name, value);
	return 0;
}

/* Variables::AddVariable(name, value). */
static cell AMX_NATIVE_CALL n_avalAdd(AMX *amx, cell *params)
{
	string tmpName = CellToString(amx, params[1]);
	string tmpValue = CellToString(amx, params[2]);
	string name = tmpName;
	string value = tmpValue;

	Variables::AddVariable(name, value);
	return 0;
}

/* Number of values of a variable. */
static cell AMX_NATIVE_CALL n_avalGetNum(AMX *amx, cell *params)
{
	string tmpName = CellToString(amx, params[1]);
	string name = tmpName;
	list<string> values;

	Variables::GetVariable(name, values);
	return values.size();
}

/* Copies value params[2] of the variable into AMX buffer params[3],
   params[4] cells long, truncated to params[4]*4-1 characters if needed. */
static cell AMX_NATIVE_CALL n_avalGetValue(AMX *amx, cell *params)
{
	string tmpName = CellToString(amx, params[1]);
	string name = tmpName;
	list<string> values;
	list<string>::iterator it;
	int i;

	Variables::GetVariable(name, values);
	if (params[2] < 0 || params[2] >= (int)values.size())
		return -1;
	i = 0;
	for (it = values.begin(); it != values.end(); it++, i++) {
		if (i != params[2])
			continue;
		{
			string v = *it;
			cell *cptr;

			if ((unsigned int)(params[4] * 4) < v.length() + 1)
				v = v.substr(0, params[4] * 4 - 1);
			if (!amx_addr_ok(amx, params[3], v.length() + params[3])) {
				amx_RaiseError(amx, 5);
				return 0;
			}
			amx_GetAddr(amx, params[3], &cptr);
			amx_SetString(cptr, (char *)v.c_str(), 1);
		}
		break;
	}
	return 0;
}

/* Variables::DeleteVariable(name). */
static cell AMX_NATIVE_CALL n_avalDelete(AMX *amx, cell *params)
{
	string tmp = CellToString(amx, params[1]);
	string name = tmp;

	Variables::DeleteVariable(name);
	return 0;
}

/* The aval*File natives are empty. */
static cell AMX_NATIVE_CALL n_avalReadFile(AMX *amx, cell *params)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_avalWriteFile(AMX *amx, cell *params)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_avalDeleteFile(AMX *amx, cell *params)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_avalCheckFile(AMX *amx, cell *params)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_avalReadFileSub(AMX *amx, cell *params)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_avalSub2Main(AMX *amx, cell *params)
{
	return 0;
}

AMX_NATIVE_INFO smallext_dat[] = {
	{ "datOpen", n_datOpen },
	{ "datClose", n_datClose },
	{ "datFind", n_datFind },
	{ "datSync", n_datSync },
	{ "datGetNum", n_datGetNum },
	{ "datGetNumFields", n_datGetNumFields },
	{ "datGetData", n_datGetData },
	{ "datUpdateData", n_datUpdateData },
	{ "datSQLExec", n_datSQLExec },
	{ "datTRBegin", n_datTRBegin },
	{ "datTREnd", n_datTREnd },
	{ "datTRAbort", n_datTRAbort },
	{ "datCheckRepairFlag", n_datCheckRepairFlag },
	{ "datClearRepairFlag", n_datClearRepairFlag },
	{ "avalSet", n_avalSet },
	{ "avalAdd", n_avalAdd },
	{ "avalGetNum", n_avalGetNum },
	{ "avalGetValue", n_avalGetValue },
	{ "avalDelete", n_avalDelete },
	{ "avalReadFile", n_avalReadFile },
	{ "avalWriteFile", n_avalWriteFile },
	{ "avalDeleteFile", n_avalDeleteFile },
	{ "avalCheckFile", n_avalCheckFile },
	{ "avalReadFileSub", n_avalReadFileSub },
	{ "avalSub2Main", n_avalSub2Main },
	{ 0, 0 },
};
