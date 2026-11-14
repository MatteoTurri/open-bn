/*
 * Small/amx natives of the "browser" module (smallext_browser): save data and
 * folder operations svdat*, run on thFileOperation/thFileList threads, and the
 * icon loader icon*; plus wrapTitle, which splits a title over two lines.
 */

#include "../../ref/small-fmt6/amx.h"
#include "natives/smallext.h"
#include <iostream.h>
#include <list>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <queue>
#include "memcard/scemc.h"
#include "fs/fmfileinfo.h"
#include "script/script.h"
#include "app/bntime.h"
#include "fs/mcsaveproperty.h"

/* Heap entry of wrapTitle; operator< is reversed, so the priority_queue yields
 * the smallest pos first. */
struct title_lf_pos {
	/* 0x00 */ int id;
	/* 0x04 */ int pos;
};

int operator<(const title_lf_pos &a, const title_lf_pos &b)
{
	return a.pos > b.pos;
}

/* conv_str and reverse_str reverse the byte order inside every 4-byte group of
 * a string, in place, up to the first NUL; they differ only in which swapped
 * byte ends the loop. */
int conv_str(char *s)
{
	int cont = 1;
	char *p = s;

	do {
		int i;

		for (i = 0; i < 2; i++) {
			char front = p[i];
			char back = *(p - (i - 3));

			*(p - (i - 3)) = front;
			p[i] = back;
			if (back == 0) {
				cont = 0;
				break;
			}
			if (front == 0)
				cont = 0;
		}
		p += 4;
	} while (cont);
	return 0;
}

int reverse_str(char *s)
{
	int cont = 1;
	char *p = s;

	do {
		int i;

		for (i = 0; i < 2; i++) {
			char back = *(p - (i - 3));
			char front = p[i];

			*(p - (i - 3)) = front;
			p[i] = back;
			if (front == 0) {
				cont = 0;
				break;
			}
			if (back == 0)
				cont = 0;
		}
		p += 4;
	} while (cont);
	return 0;
}

/*
 * Splits the title of a save into two lines l1 and l2, cutting after cols
 * characters. With cols == 0 the cut is chosen here: ASCII spaces are trimmed at both ends
 * and ideographic spaces (UTF-8 E3 80 80) at the start and end, characters
 * (1 to 6 UTF-8 bytes) are counted and each space goes into a priority queue
 * by its distance from column 16; the first one leaving at most 16 characters
 * on each line wins, otherwise the cut is at 16. Returns -1 on an invalid
 * UTF-8 byte.
 */
static int wrapTitle(string title, int cols, string &l1, string &l2)
{
	priority_queue<title_lf_pos> q;
	title_lf_pos lf;
	const char *s;

	l1.erase(l1.begin(), l1.end());
	l2.erase(l2.begin(), l2.end());

	if (cols == 0) {
		string t = title;
		unsigned int pos;
		unsigned int i;
		unsigned int j;
		int cnt;

		pos = t.find_first_not_of(' ');
		if (pos != string::npos)
			t.erase(0, pos);
		pos = t.find_last_not_of(' ');
		if (pos != string::npos)
			t.erase(pos + 1);
		title = t;

		/* ideographic spaces (U+3000) at the start */
		pos = 0;
		s = title.c_str();
		for (i = 0; i < strlen(s) - 3; i += 3) {
			if (s[i] == (char)0xe3 && s[i + 1] == (char)0x80 && s[i + 2] == (char)0x80)
				pos = i + 3;
			else
				break;
		}
		if (pos)
			title.erase(0, pos);

		/* ... and at the end */
		s = title.c_str();
		pos = 0;
		for (j = strlen(s) - 3;; j -= 3) {
			if (s[j] == (char)0xe3 && s[j + 1] == (char)0x80 && s[j + 2] == (char)0x80)
				pos = j;
			else
				break;
		}
		if (pos)
			title.erase(pos);

		s = title.c_str();
		cnt = 0;
		for (pos = 0; s[pos] != 0;) {
			if ((s[pos] & 0x80) == 0) {
				cnt++;
				if (s[pos] == ' ') {
					lf.id = cnt;
					lf.pos = abs(16 - cnt);
					q.push(lf);
				}
				pos++;
			} else if ((s[pos] & 0xe0) == 0xc0) {
				cnt++;
				pos += 2;
			} else if ((s[pos] & 0xf0) == 0xe0) {
				cnt++;
				if (s[pos] == (char)0xe3 && s[pos + 1] == (char)0x80 && s[pos + 2] == (char)0x80) {
					lf.id = cnt;
					lf.pos = abs(16 - cnt);
					q.push(lf);
				}
				pos += 3;
			} else if ((s[pos] & 0xf8) == 0xf0) {
				cnt++;
				pos += 4;
			} else if ((s[pos] & 0xfc) == 0xf8) {
				cnt++;
				pos += 5;
			} else if ((s[pos] & 0xfe) == 0xfc) {
				cnt++;
				pos += 6;
			} else
				return -1;
		}

		if (cnt >= 16) {
			bool found = false;

			while (!q.empty()) {
				cols = q.top().id;
				if (cols <= 16 && cnt - cols < 16) {
					found = true;
					break;
				}
				q.pop();
			}
			if (!found)
				cols = 16;
		} else
			cols = 16;
	}

	s = title.c_str();
	int i = 0;
	bool second = false;
	int cnt = 0;

	while (s[i]) {
		if (cnt == cols)
			second = true;
		if ((s[i] & 0x80) == 0) {
			cnt++;
			if (!second)
				l1 += s[i++];
			else
				l2 += s[i++];
		} else if ((s[i] & 0xe0) == 0xc0) {
			cnt++;
			if (!second) {
				l1 += s[i++];
				l1 += s[i++];
			} else {
				l2 += s[i++];
				l2 += s[i++];
			}
		} else if ((s[i] & 0xf0) == 0xe0) {
			cnt++;
			if (!second) {
				l1 += s[i++];
				l1 += s[i++];
				l1 += s[i++];
			} else {
				l2 += s[i++];
				l2 += s[i++];
				l2 += s[i++];
			}
		} else if ((s[i] & 0xf8) == 0xf0) {
			cnt++;
			if (!second) {
				l1 += s[i++];
				l1 += s[i++];
				l1 += s[i++];
				l1 += s[i++];
			} else {
				l2 += s[i++];
				l2 += s[i++];
				l2 += s[i++];
				l2 += s[i++];
			}
		} else if ((s[i] & 0xfc) == 0xf8) {
			cnt++;
			if (!second) {
				l1 += s[i++];
				l1 += s[i++];
				l1 += s[i++];
				l1 += s[i++];
				l1 += s[i++];
			} else {
				l2 += s[i++];
				l2 += s[i++];
				l2 += s[i++];
				l2 += s[i++];
				l2 += s[i++];
			}
		} else if ((s[i] & 0xfe) == 0xfc) {
			cnt++;
			if (!second) {
				l1 += s[i++];
				l1 += s[i++];
				l1 += s[i++];
				l1 += s[i++];
				l1 += s[i++];
				l1 += s[i++];
			} else {
				l2 += s[i++];
				l2 += s[i++];
				l2 += s[i++];
				l2 += s[i++];
				l2 += s[i++];
				l2 += s[i++];
			}
		} else
			return -1;
	}
	return 0;
}

/* Init/Exit hooks of the module, empty. */
extern "C" {

void Init(void)
{}

void Exit(void)
{}

} /* extern "C" */

/* ---- the browser natives ---- */

/* One entry of dst_folder_list. */
struct folder_info {
	string a, b;
};

/* Folder that svdatSync leaves out of the svdatCountData total. */
static string s_iconFolderPath("");

/* Folder pairs handled by the svdat*Folder natives. */
vector<folder_info> dst_folder_list;

struct __amx;

#include "thread/thposixthread.h"

#include "fs/thfileoperation.h"

#include "fs/bburl.h"

static const long TAG_PSCR = 0x52435350;         /* "PSCR" */
static const unsigned int TAG_ICON = 0x4944335f; /* "_3DI" */

/* Block that iconLoad registers on the Script under tag "_3DI": the loading
   thread. */
struct IconJob {
	thPosixThread *th;

	IconJob() { th = 0; }
};

#include "fs/thloadicon.h"

#include "fs/thfilelist.h"

/* Job of an asynchronous native, returned to the script as a cell; svdatSync
   reads fo or fl according to op (1..9). */
struct SvdatJob {
	/* 0x00 */ thFileOperation *fo;
	/* 0x04 */ thFileList *fl;
	/* 0x08 */ int op;

	SvdatJob()
	{
		fo = 0;
		fl = 0;
		op = 0;
	}

	~SvdatJob() {}
};

/* True when [p,q] lies entirely in the script heap or entirely in its stack. */
static inline int amx_addr_ok(AMX *amx, cell p, cell q)
{
	if (p >= 0 && p < amx->hea && q >= 0 && q < amx->hea)
		return 1;
	if (p >= amx->stk && p < amx->stp && q >= amx->stk && q < amx->stp)
		return 1;
	return 0;
}

static inline Script *AmxScript(AMX *amx)
{
	Script *s;

	amx_GetUserData(amx, TAG_PSCR, (void **)&s);
	return s;
}

/* Closes the icon job: stops the thread if it has not finished, releases it
   and frees the block. */
static inline void IconJobClose(IconJob *job)
{
	if (job != 0) {
		if (job->th != 0) {
			if (!job->th->IsEnd())
				job->th->abort();
			job->th->release();
		}
		delete job;
	}
}

/* Starts an asynchronous copy (OPR_SVDAT_COPY) and returns its SvdatJob. */
static cell AMX_NATIVE_CALL n_svdatCopy(AMX *amx, cell *params)
{
	BBUrl url;
	string srcPath = CellToString(amx, params[1]);

	if (srcPath.length() == 0)
		return -1;
	url.setUrlString(srcPath);
	srcPath = url.getPathString();

	string dstPath = CellToString(amx, params[2]);

	if (dstPath.length() == 0)
		return -1;
	url.setUrlString(dstPath);
	dstPath = url.getPathString();

	int overwrite = params[3];
	SvdatJob *job = new SvdatJob;

	if (job == 0)
		return -1;
	job->op = 1;

	thFileOperation *fo = new thFileOperation;
	cell *ptr;

	fo->setSrcPath(srcPath);
	fo->m_dstPath = dstPath;
	if (overwrite == 1)
		fo->m_overwriteFlag = overwrite;
	else
		fo->m_overwriteFlag = 0;
	fo->m_oprCommand = thFileOperation::OPR_SVDAT_COPY;
	fo->addThreadIdleList(4, 2);
	job->fo = fo;

	if (amx_GetAddr(amx, params[4], &ptr) == 0)
		*ptr = (cell)job;
	return 0;
}

/* svdatCopy for a save folder (OPR_SVFOLDER_COPY). */
static cell AMX_NATIVE_CALL n_svdatCopyFolder(AMX *amx, cell *params)
{
	BBUrl url;
	string srcPath = CellToString(amx, params[1]);

	if (srcPath.length() == 0)
		return -1;
	url.setUrlString(srcPath);
	srcPath = url.getPathString();

	string dstPath = CellToString(amx, params[2]);

	if (dstPath.length() == 0)
		return -1;
	url.setUrlString(dstPath);
	dstPath = url.getPathString();

	int overwrite = params[3];
	SvdatJob *job = new SvdatJob;

	if (job == 0)
		return -1;
	job->op = 3;

	thFileOperation *fo = new thFileOperation;
	cell *ptr;

	fo->setSrcPath(srcPath);
	fo->m_dstPath = dstPath;
	if (overwrite == 1)
		fo->m_overwriteFlag = overwrite;
	else
		fo->m_overwriteFlag = 0;
	fo->m_oprCommand = thFileOperation::OPR_SVFOLDER_COPY;
	fo->addThreadIdleList(4, 2);
	job->fo = fo;

	if (amx_GetAddr(amx, params[4], &ptr) == 0)
		*ptr = (cell)job;
	return 0;
}

/* svdatCopy as a move (OPR_SVDAT_MOVE). */
static cell AMX_NATIVE_CALL n_svdatMove(AMX *amx, cell *params)
{
	BBUrl url;
	string srcPath = CellToString(amx, params[1]);

	if (srcPath.length() == 0)
		return -1;
	url.setUrlString(srcPath);
	srcPath = url.getPathString();

	string dstPath = CellToString(amx, params[2]);

	if (dstPath.length() == 0)
		return -1;
	url.setUrlString(dstPath);
	dstPath = url.getPathString();

	int overwrite = params[3];
	SvdatJob *job = new SvdatJob;

	if (job == 0)
		return -1;
	job->op = 2;

	thFileOperation *fo = new thFileOperation;
	cell *ptr;

	fo->setSrcPath(srcPath);
	fo->m_dstPath = dstPath;
	if (overwrite == 1)
		fo->m_overwriteFlag = overwrite;
	else
		fo->m_overwriteFlag = 0;
	fo->m_oprCommand = thFileOperation::OPR_SVDAT_MOVE;
	fo->addThreadIdleList(4, 2);
	job->fo = fo;

	if (amx_GetAddr(amx, params[4], &ptr) == 0)
		*ptr = (cell)job;
	return 0;
}

/* Asynchronous removal of one path (OPR_REMOVE). */
static cell AMX_NATIVE_CALL n_svdatDelete(AMX *amx, cell *params)
{
	BBUrl url;
	string srcPath = CellToString(amx, params[1]);

	if (srcPath.length() == 0)
		return -1;
	url.setUrlString(srcPath);
	srcPath = url.getPathString();

	SvdatJob *job = new SvdatJob;

	if (job == 0)
		return -1;
	job->op = 4;

	thFileOperation *fo = new thFileOperation;
	cell *ptr;

	fo->setSrcPath(srcPath);
	fo->m_oprCommand = thFileOperation::OPR_REMOVE;
	fo->addThreadIdleList(4, 2);
	job->fo = fo;

	if (amx_GetAddr(amx, params[2], &ptr) == 0)
		*ptr = (cell)job;
	return 0;
}

/* Asynchronous folder creation (OPR_SVFOLDER_CREATE). */
static cell AMX_NATIVE_CALL n_svdatMkdir(AMX *amx, cell *params)
{
	BBUrl url;
	string srcPath = CellToString(amx, params[1]);

	if (srcPath.length() == 0)
		return -1;
	url.setUrlString(srcPath);
	srcPath = url.getPathString();

	strncmp(srcPath.c_str(), "pfs:", 4);  /* redundant as written */

	SvdatJob *job = new SvdatJob;

	if (job == 0)
		return -1;
	job->op = 8;

	thFileOperation *fo = new thFileOperation;
	cell *ptr;

	fo->setSrcPath(srcPath);
	fo->m_oprCommand = thFileOperation::OPR_SVFOLDER_CREATE;
	fo->addThreadIdleList(4, 2);
	job->fo = fo;

	if (amx_GetAddr(amx, params[2], &ptr) == 0)
		*ptr = (cell)job;
	return 0;
}

/* Asynchronous property read (OPR_GETPROPERTY) of a path; trailing slashes
   survive the trip through BBUrl. */
static cell AMX_NATIVE_CALL n_svdatInformation(AMX *amx, cell *params)
{
	int hadTail = 0;
	string tail;
	BBUrl url;
	string src = CellToString(amx, params[1]);
	int pos;
	cell *ptr;

	pos = src.find_last_not_of('/');
	if (pos != -1 && (unsigned int)pos < src.length() - 1) {
		hadTail = 1;
		tail = src;
		tail.erase(0, pos + 1);
	}
	if (src.length() == 0)
		return -1;

	url.setUrlString(src);
	src = url.getPathString();

	if (hadTail) {
		pos = src.find_last_not_of('/');
		if (pos != -1 && (unsigned int)pos == src.length() - 1) {
			src += tail;
		} else {
			src.erase(pos + 1);
			src += tail;
		}
	}

	SvdatJob *job = new SvdatJob;

	if (job == 0)
		return -1;
	job->op = 5;

	thFileOperation *fo = new thFileOperation;

	fo->setSrcPath(src);
	fo->m_oprCommand = thFileOperation::OPR_GETPROPERTY;
	fo->addThreadIdleList(4, 1);
	job->fo = fo;

	if (amx_GetAddr(amx, params[2], &ptr) == 0)
		*ptr = (cell)job;
	return 0;
}

/* Asynchronous memory card information (OPR_GETMCINFO); returns the job. */
static cell AMX_NATIVE_CALL n_svdatMCInfo(AMX *amx, cell *params)
{
	int port = params[1];
	int slot = params[2];
	SvdatJob *job;
	thFileOperation *fo;
	cell *ptr;

	job = new SvdatJob;
	if (job == 0)
		return -1;
	job->op = 9;

	fo = new thFileOperation;
	fo->m_mcPort = port;
	fo->m_mcSlot = slot;
	fo->m_oprCommand = thFileOperation::OPR_GETMCINFO;
	fo->start();
	job->fo = fo;

	if (amx_GetAddr(amx, params[3], &ptr) == 0)
		*ptr = (cell)job;
	return 0;
}

/* Polls a job: params[3] is the SvdatJob, params[1] receives the percentage
   and params[2] the final status. A finished job is released and freed,
   except a property read (op 5), still needed by svdatGetInfoData. */
static cell AMX_NATIVE_CALL n_svdatSync(AMX *amx, cell *params)
{
	SvdatJob *job = (SvdatJob *)params[3];
	cell *ptr;

	if (job == 0)
		return -1;

	switch (job->op) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 8: {
		thFileOperation *fo = job->fo;

		if (fo == 0) {
			job->op = 0;
			return -1;
		}
		if (amx_GetAddr(amx, params[1], &ptr) == 0)
			*ptr = fo->getPercent();
		if (fo->IsEnd()) {
			if (amx_GetAddr(amx, params[2], &ptr) == 0)
				*ptr = fo->getEndStatus();
			int isProperty = fo->getOPRCommand() == thFileOperation::OPR_GETPROPERTY;

			if (!isProperty) {
				job->op = 0;
				fo->release();
				delete job;
			}
			return 1;
		}
		if (amx_GetAddr(amx, params[2], &ptr) == 0)
			*ptr = 0;
		return 0;
	}
	case 9: {
		thFileOperation *fo = job->fo;

		if (fo == 0) {
			job->op = 0;
			return -1;
		}
		if (fo->IsEnd()) {
			job->op = 0;
			fo->release();
			delete job;
			return 1;
		}
		return 0;
	}
	case 7: {
		thFileList *fl = job->fl;
		int count;

		if (fl == 0) {
			job->op = 0;
			return -1;
		}
		if (fl->IsEnd()) {
			count = fl->m_entryList.size();
			if (s_iconFolderPath.length() != 0 && count > 0) {
				list<fmEntry>::iterator it = fl->m_entryList.begin();

				for (; it != fl->m_entryList.end(); it++) {
					if (strcmp(it->comment.c_str(), s_iconFolderPath.c_str()) == 0) {
						count--;
						break;
					}
				}
			}
			if (amx_GetAddr(amx, params[1], &ptr) == 0)
				*ptr = 100;
			if (amx_GetAddr(amx, params[2], &ptr) == 0)
				*ptr = count;
			job->op = 0;
			fl->release();
			delete job;
			return 1;
		}
		return 0;
	}
	}
	return 0;
}

/* Copies the file information and save property read by svdatInformation into
   the script buffers: two strings of at most 1023 bytes, the title split over
   two lines, three integers and the local date. */
static cell AMX_NATIVE_CALL n_svdatGetInfoData(AMX *amx, cell *params)
{
	SvdatJob *job = (SvdatJob *)params[1];

	if (job == 0)
		return -1;

	thFileOperation *fo = job->fo;

	if (fo == 0) {
		cout << "pthFileOperation NULL" << endl;
		job->op = 0;
		return -1;
	}

	fmFileInfo info = fo->m_fileProperty;
	MCSaveProperty prop = fo->m_mcsvProperty;
	cell *ptr;
	int year, mon, day, hour, min, sec;

	if (info.name.length() < 1024) {
		if (!amx_addr_ok(amx, params[16], info.name.length() + params[16])) {
			amx_RaiseError(amx, AMX_ERR_MEMACCESS);
			return 0;
		}
		amx_GetAddr(amx, params[2], &ptr);
		amx_SetString(ptr, (char *)info.name.c_str(), 1);
	}
	if (info.dir.length() < 1024) {
		/* NOTE: bug: the guard for dir uses the length of name */
		if (!amx_addr_ok(amx, params[17], info.name.length() + params[17])) {
			amx_RaiseError(amx, AMX_ERR_MEMACCESS);
			return 0;
		}
		amx_GetAddr(amx, params[3], &ptr);
		amx_SetString(ptr, (char *)info.dir.c_str(), 1);
	}
	if (amx_GetAddr(amx, params[4], &ptr) == 0)
		*ptr = info.size;

	if (prop.name.length() < 1024) {
		int cols = (short)prop.titleBreakOffset / 2;

		if ((unsigned int)cols > 16)
			cols = 16;

		string l1, l2;

		wrapTitle(prop.name, cols, l1, l2);

		if (!amx_addr_ok(amx, params[18], l1.length() + params[18])) {
			amx_RaiseError(amx, AMX_ERR_MEMACCESS);
			return 0;
		}
		amx_GetAddr(amx, params[5], &ptr);
		amx_SetString(ptr, (char *)l1.c_str(), 1);

		if (!amx_addr_ok(amx, params[19], l2.length() + params[19])) {
			amx_RaiseError(amx, AMX_ERR_MEMACCESS);
			return 0;
		}
		amx_GetAddr(amx, params[6], &ptr);
		amx_SetString(ptr, (char *)l2.c_str(), 1);
	}

	if (amx_GetAddr(amx, params[7], &ptr) == 0)
		*ptr = info.saveKind;
	if (amx_GetAddr(amx, params[8], &ptr) == 0) {
		if (info.noCopy)
			*ptr = 1;
		else
			*ptr = 0;
	}

	long long t = t2c(info.modified.Year, info.modified.Month, info.modified.Day, info.modified.Hour,
	    info.modified.Min, info.modified.Sec);

	t = _calc_localtime(t);
	t = clip_time(t);
	c2t(t, &year, &mon, &day, &hour, &min, &sec);

	if (amx_GetAddr(amx, params[9], &ptr) == 0)
		*ptr = year;
	if (amx_GetAddr(amx, params[10], &ptr) == 0)
		*ptr = mon;
	if (amx_GetAddr(amx, params[11], &ptr) == 0)
		*ptr = day;
	if (amx_GetAddr(amx, params[12], &ptr) == 0)
		*ptr = hour;
	if (amx_GetAddr(amx, params[13], &ptr) == 0)
		*ptr = min;
	if (amx_GetAddr(amx, params[14], &ptr) == 0)
		*ptr = sec;
	if (amx_GetAddr(amx, params[15], &ptr) == 0) {
		if (prop.damaged)
			*ptr = 1;
		else
			*ptr = 0;
	}

	job->op = 0;
	fo->release();
	delete job;
	return 0;
}

static cell AMX_NATIVE_CALL n_svdatGetMCInfo(AMX *amx, cell *params)
{
	int cardType, freeBlocks, formatted, status;
	cell *ptr;

	if (thFileOperation::getMCInfo(params[1], params[2], cardType, freeBlocks, formatted, status) < 0)
		return -1;
	if (amx_GetAddr(amx, params[3], &ptr) == 0)
		*ptr = cardType;
	if (amx_GetAddr(amx, params[4], &ptr) == 0)
		*ptr = freeBlocks;
	if (amx_GetAddr(amx, params[5], &ptr) == 0)
		*ptr = formatted;
	if (amx_GetAddr(amx, params[6], &ptr) == 0)
		*ptr = status;
	return 0;
}

/* Asynchronous memory card format (OPR_MCFORMAT). */
static cell AMX_NATIVE_CALL n_svdatMCFormat(AMX *amx, cell *params)
{
	unsigned int port = params[1];
	unsigned int slot = params[2];
	SvdatJob *job;
	thFileOperation *fo;
	cell *ptr;

	if (port >= 2)
		return -1;
	if (slot >= 4)
		return -1;

	job = new SvdatJob;
	if (job == 0)
		return -1;
	job->op = 6;

	fo = new thFileOperation;
	fo->m_mcPort = port;
	fo->m_mcSlot = slot;
	fo->m_oprCommand = thFileOperation::OPR_MCFORMAT;
	fo->addThreadIdleList(4, 2);
	job->fo = fo;

	if (amx_GetAddr(amx, params[3], &ptr) == 0)
		*ptr = (cell)job;
	return 0;
}

/* Free space in KiB, saturated at 0xFFFFFFFF; only mode 0 (params[1]) passes
   the path through BBUrl. */
static cell AMX_NATIVE_CALL n_svdatGetFreeSize(AMX *amx, cell *params)
{
	int mode = params[1];
	int port = params[2];
	int slot = params[3];
	string path = CellToString(amx, params[4]);
	unsigned long long size;
	cell *ptr;

	if (mode == 0) {
		BBUrl url;

		url.setUrlString(path);
		path = url.getPathString();
	}
	if (thFileOperation::getFreeSize(mode, port, slot, path, size) < 0)
		return -1;
	if (amx_GetAddr(amx, params[5], &ptr) == 0) {
		size >>= 10;
		if (size > 0xffffffffULL)
			size = 0xffffffff;
		*ptr = size;
	}
	return 0;
}

/* Empties dst_folder_list and reserves room for 260 entries. */
static cell AMX_NATIVE_CALL n_svdatFolderClear(AMX *amx, cell *params)
{
	vector<folder_info>::iterator it = dst_folder_list.begin();

	dst_folder_list.erase(it, dst_folder_list.end());
	dst_folder_list.reserve(260);
	return 0;
}

static cell AMX_NATIVE_CALL n_svdatGetNumFolder(AMX *amx, cell *params)
{
	return dst_folder_list.size();
}

/* Appends the pair (params[1], params[2]) to dst_folder_list. */
static cell AMX_NATIVE_CALL n_svdatAddFolder(AMX *amx, cell *params)
{
	folder_info fi;

	fi.a = CellToString(amx, params[1]);
	fi.b = CellToString(amx, params[2]);
	if (dst_folder_list.size() >= 260)
		return -1;
	dst_folder_list.push_back(fi);
	return 0;
}

/* Copies entry params[1] of dst_folder_list into the two script buffers. */
static cell AMX_NATIVE_CALL n_svdatGetFolder(AMX *amx, cell *params)
{
	folder_info fi;
	cell *ptr;

	if ((unsigned int)params[1] >= 260)
		return -1;
	fi = dst_folder_list[params[1]];

	if (!amx_addr_ok(amx, params[4], fi.a.length() + params[4])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[2], &ptr);
	amx_SetString(ptr, (char *)fi.a.c_str(), 1);

	if (!amx_addr_ok(amx, params[5], fi.b.length() + params[5])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[3], &ptr);
	amx_SetString(ptr, (char *)fi.b.c_str(), 1);
	return 0;
}

/* Starts a thFileList on the path; s_iconFolderPath names the folder that
   svdatSync leaves out of the count. */
static cell AMX_NATIVE_CALL n_svdatCountData(AMX *amx, cell *params)
{
	BBUrl url;
	string src = CellToString(amx, params[1]);

	if (src.length() == 0)
		return -1;
	url.setUrlString(src);

	string path = url.getPathString();

	s_iconFolderPath = CellToString(amx, params[2]);

	SvdatJob *job = new SvdatJob;

	if (job == 0)
		return -1;
	job->op = 7;

	thFileList *fl = new thFileList(0, 0);
	cell *ptr;

	fl->setUrl(path.c_str());
	fl->m_noParentDirFlag = 1;
	fl->start();
	fl->addThreadIdleList(4, 2);
	job->fl = fl;

	if (amx_GetAddr(amx, params[3], &ptr) == 0)
		*ptr = (cell)job;
	return 0;
}

/* Truncates the string to params[1] characters (UTF-8 aware), adding "..."
   when it was longer. */
static cell AMX_NATIVE_CALL n_svdatStringCut(AMX *amx, cell *params)
{
	BBUrl unused;
	int n = params[1];
	string src = CellToString(amx, params[2]);
	const unsigned char *q = (const unsigned char *)src.c_str();
	string out;
	int i = 0;
	cell *ptr;

	while (*q != 0) {
		unsigned char c;

		if (i >= n) {
			out += "...";
			break;
		}
		c = *q;
		if ((c & 0x80) == 0) {
			i++;
			out += (char)*q++;
		} else if ((c & 0xe0) == 0xc0) {
			i++;
			out += (char)*q++;
			out += (char)*q++;
		} else if ((c & 0xf0) == 0xe0) {
			i++;
			out += (char)*q++;
			out += (char)*q++;
			out += (char)*q++;
		} else if ((c & 0xf8) == 0xf0) {
			i++;
			out += (char)*q++;
			out += (char)*q++;
			out += (char)*q++;
			out += (char)*q++;
		} else if ((c & 0xfc) == 0xf8) {
			i++;
			out += (char)*q++;
			out += (char)*q++;
			out += (char)*q++;
			out += (char)*q++;
			out += (char)*q++;
		} else if ((c & 0xfe) == 0xfc) {  /* NOTE: bug: a 6-byte sequence copies only 5 bytes */
			i++;
			out += (char)*q++;
			out += (char)*q++;
			out += (char)*q++;
			out += (char)*q++;
			out += (char)*q++;
		} else {
			return -1;
		}
	}

	if (!amx_addr_ok(amx, params[4], out.length() + params[4])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[3], &ptr);
	amx_SetString(ptr, (char *)out.c_str(), 1);
	return 0;
}

/* Truncates the string to at most params[1] bytes without splitting a UTF-8
   character. */
static cell AMX_NATIVE_CALL n_svdatStringCutByte(AMX *amx, cell *params)
{
	BBUrl unused;
	int n = params[1];
	string src = CellToString(amx, params[2]);
	const unsigned char *q = (const unsigned char *)src.c_str();
	string out;
	int total = 0;
	int i = 0;
	cell *ptr;

	while (q[i] != 0) {
		if ((q[i] & 0x80) == 0) {
			total += 1;
			if (total > n)
				break;
			out += (char)q[i++];
		} else if ((q[i] & 0xe0) == 0xc0) {
			total += 2;
			if (total > n)
				break;
			out += (char)q[i++];
			out += (char)q[i++];
		} else if ((q[i] & 0xf0) == 0xe0) {
			total += 3;
			if (total > n)
				break;
			out += (char)q[i++];
			out += (char)q[i++];
			out += (char)q[i++];
		} else if ((q[i] & 0xf8) == 0xf0) {
			total += 4;
			if (total > n)
				break;
			out += (char)q[i++];
			out += (char)q[i++];
			out += (char)q[i++];
			out += (char)q[i++];
		} else if ((q[i] & 0xfc) == 0xf8) {
			total += 5;
			if (total > n)
				break;
			out += (char)q[i++];
			out += (char)q[i++];
			out += (char)q[i++];
			out += (char)q[i++];
			out += (char)q[i++];
		} else if ((q[i] & 0xfe) == 0xfc) {  /* NOTE: bug: counts 6 bytes but copies only 5 */
			total += 6;
			if (total > n)
				break;
			out += (char)q[i++];
			out += (char)q[i++];
			out += (char)q[i++];
			out += (char)q[i++];
			out += (char)q[i++];
		} else {
			return -1;
		}
	}

	if (!amx_addr_ok(amx, params[4], out.length() + params[4])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[3], &ptr);
	amx_SetString(ptr, (char *)out.c_str(), 1);
	return 0;
}

/* Checks a folder name: -1 if 251 bytes or longer or not valid UTF-8, -3 for
   "." and "..", -2 if it holds one of the eleven forbidden characters, else 0. */
static cell AMX_NATIVE_CALL n_svdatChkFolderName(AMX *amx, cell *params)
{
	BBUrl unused;
	string name = CellToString(amx, params[1]);
	const unsigned char *p = (const unsigned char *)name.c_str();
	int i;

	if (strlen((const char *)p) >= 251)
		return -1;
	if (strcmp((const char *)p, ".") == 0)
		return -3;
	if (strcmp((const char *)p, "..") == 0)
		return -3;

	i = 0;
	while (p[i] != 0) {
		if ((p[i] & 0x80) == 0) {
			if (p[i] == '\\' || p[i] == '/' || p[i] == ':' || p[i] == ';' || p[i] == ',' || p[i] == '*'
			    || p[i] == '?' || p[i] == '"' || p[i] == '<' || p[i] == '>' || p[i] == '|')
				return -2;
			i++;
		} else if ((p[i] & 0xe0) == 0xc0) {
			i += 2;
		} else if ((p[i] & 0xf0) == 0xe0) {
			i += 3;
		} else if ((p[i] & 0xf8) == 0xf0) {
			i += 4;
		} else if ((p[i] & 0xfc) == 0xf8) {
			i += 5;
		} else if ((p[i] & 0xfe) == 0xfc) {
			i += 6;
		} else {
			return -1;
		}
	}
	return 0;
}

/* Strips leading and trailing spaces and writes the string back. */
static cell AMX_NATIVE_CALL n_svdatStrTrmSpc(AMX *amx, cell *params)
{
	BBUrl unused;  /* unused, but part of the layout */
	string s = CellToString(amx, params[1]);
	unsigned int n;
	cell *ptr;

	n = s.find_first_not_of(' ', 0);
	if (n != 0)
		s.erase(0, n);
	n = s.find_last_not_of(' ');
	if (n < s.length() - 1)
		s.erase(n + 1);

	if (!amx_addr_ok(amx, params[3], s.length() + params[3])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[2], &ptr);
	amx_SetString(ptr, (char *)s.c_str(), 1);
	return 0;
}

/* Returns the last component of the path (everything after the last '/'). */
static cell AMX_NATIVE_CALL n_svdatGetName(AMX *amx, cell *params)
{
	BBUrl unused;
	string src = CellToString(amx, params[1]);
	string out;
	int pos;
	cell *ptr;

	pos = src.find_last_of('/');
	if (pos == -1) {
		out = src;
	} else {
		out = src;
		out.erase(0, pos + 1);
	}

	if (!amx_addr_ok(amx, params[3], out.length() + params[3])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[2], &ptr);
	amx_SetString(ptr, (char *)out.c_str(), 1);
	return 0;
}

/* The icon job of a script, user data '_3DI'. */
static inline IconJob *ScriptIconJob(Script *owner)
{
	return (IconJob *)owner->GetUserData(TAG_ICON);
}

/* Removes it from the script and returns it. */
static inline IconJob *TakeScriptIconJob(Script *owner)
{
	return (IconJob *)owner->DelUserData(TAG_ICON);
}

/* Script destroy callback registered by iconLoad: closes the icon job. */
static void iconScriptDestroy(__amx *amx)
{
	IconJobClose(ScriptIconJob(AmxScript((AMX *)amx)));
}

/* Starts the icon loading thread and registers its IconJob on the Script (tag
   "_3DI") with the destroy callback. Each flag passes only when exactly 1. */
static cell AMX_NATIVE_CALL n_iconLoad(AMX *amx, cell *params)
{
	char buf[1024];
	int flag1 = params[4];
	int flag2 = params[5];
	int flag3 = params[6];

	sprintf(buf, "/opt0/cache/ictmp%04d", params[1]);

	string path(buf);
	string url = CellToString(amx, params[2]);
	thLoadIcon *th = new thLoadIcon(0, 0);
	IconJob *job;
	cell *ptr;

	th->setDataName(url);
	th->setCacheName(path);
	if (flag1 == 0)
		th->m_loadNormalFlag = 0;
	else if (flag1 == 1)
		th->m_loadNormalFlag = flag1;
	else
		th->m_loadNormalFlag = 0;
	if (flag2 == 0)
		th->m_loadCopyFlag = 0;
	else if (flag2 == 1)
		th->m_loadCopyFlag = flag2;
	else
		th->m_loadCopyFlag = 0;
	if (flag3 == 0)
		th->m_loadDeleteFlag = 0;
	else if (flag3 == 1)
		th->m_loadDeleteFlag = flag3;
	else
		th->m_loadDeleteFlag = 0;
	th->addThreadIdleList(0, 1);

	if (amx_GetAddr(amx, params[3], &ptr) == 0)
		*ptr = (cell)th;

	job = new IconJob;
	if (job == 0)
		return -1;
	job->th = th;
	AmxScript(amx)->SetUserData(TAG_ICON, job);
	AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_ICON, iconScriptDestroy);
	return 0;
}

/* Polls the thLoadIcon returned by iconLoad (params[1]); when loading is done
   writes the icon title split over two lines, or whole into params[4] when
   params[5] is not a valid script address. Closes the job and returns 1. */
static cell AMX_NATIVE_CALL n_iconSync(AMX *amx, cell *params)
{
	thLoadIcon *th = (thLoadIcon *)params[1];
	cell *ptr;

	if (th == 0)
		return -1;
	if (th->m_abortFlag) {
		th->release();
		return -1;
	}
	if (th->IsRelease())
		return -1;
	if (!th->IsEnd())
		return 0;

	if (amx_GetAddr(amx, params[2], &ptr) == 0)
		*ptr = th->getEndStatus();
	if (amx_GetAddr(amx, params[3], &ptr) == 0) {
		if (th->m_damaged)
			*ptr = 1;
		else
			*ptr = 0;
	}
	if (amx_GetAddr(amx, params[5], &ptr) == 0) {
		int cols = th->m_titleBreak / 2;

		if ((unsigned int)cols > 16)
			cols = 16;

		string l1, l2;

		wrapTitle(th->m_title, cols, l1, l2);

		if (!amx_addr_ok(amx, params[6], l1.length() + params[6])) {
			amx_RaiseError(amx, AMX_ERR_MEMACCESS);
			return 0;
		}
		amx_GetAddr(amx, params[4], &ptr);
		amx_SetString(ptr, (char *)l1.c_str(), 1);

		if (!amx_addr_ok(amx, params[7], l2.length() + params[7])) {
			amx_RaiseError(amx, AMX_ERR_MEMACCESS);
			return 0;
		}
		amx_GetAddr(amx, params[5], &ptr);
		amx_SetString(ptr, (char *)l2.c_str(), 1);
	} else {
		if (!amx_addr_ok(amx, params[6], th->m_title.length() + params[6])) {
			amx_RaiseError(amx, AMX_ERR_MEMACCESS);
			return 0;
		}
		amx_GetAddr(amx, params[4], &ptr);
		amx_SetString(ptr, (char *)th->m_title.c_str(), 1);
	}

	if (AmxScript(amx)->GetUserData(TAG_ICON) == 0)
		return -1;
	IconJobClose(TakeScriptIconJob(AmxScript(amx)));
	return 1;
}

/* params[1] is the thLoadIcon pointer returned by iconLoad. */
static cell AMX_NATIVE_CALL n_iconCancel(AMX *amx, cell *params)
{
	thPosixThread *th = (thPosixThread *)params[1];

	if (th == 0)
		return -1;
	if (th->m_abortFlag) {
		th->release();
		return -1;
	}
	if (th->IsRelease())
		return -1;
	if (AmxScript(amx)->GetUserData(TAG_ICON) == 0)
		return -1;
	IconJobClose(TakeScriptIconJob(AmxScript(amx)));
	return 0;
}

AMX_NATIVE_INFO smallext_browser[] = {
	{ "svdatCopy", n_svdatCopy },
	{ "svdatMove", n_svdatMove },
	{ "svdatCopyFolder", n_svdatCopyFolder },
	{ "svdatDelete", n_svdatDelete },
	{ "svdatInformation", n_svdatInformation },
	{ "svdatMCInfo", n_svdatMCInfo },
	{ "svdatMkdir", n_svdatMkdir },
	{ "svdatSync", n_svdatSync },
	{ "svdatGetInfoData", n_svdatGetInfoData },
	{ "svdatGetMCInfo", n_svdatGetMCInfo },
	{ "svdatMCFormat", n_svdatMCFormat },
	{ "svdatGetFreeSize", n_svdatGetFreeSize },
	{ "svdatFolderClear", n_svdatFolderClear },
	{ "svdatGetNumFolder", n_svdatGetNumFolder },
	{ "svdatAddFolder", n_svdatAddFolder },
	{ "svdatGetFolder", n_svdatGetFolder },
	{ "svdatCountData", n_svdatCountData },
	{ "svdatStringCut", n_svdatStringCut },
	{ "svdatStringCutByte", n_svdatStringCutByte },
	{ "svdatStrTrmSpc", n_svdatStrTrmSpc },
	{ "svdatChkFolderName", n_svdatChkFolderName },
	{ "svdatGetName", n_svdatGetName },
	{ "iconLoad", n_iconLoad },
	{ "iconSync", n_iconSync },
	{ "iconCancel", n_iconCancel },
	{ 0, 0 },
};
