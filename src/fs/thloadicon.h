/*
 * thLoadIcon: thread that loads the icon of a save (memory card, PFS, PS1
 * virtual memory card) and writes it to the icon cache.
 */

#ifndef THLOADICON_H
#define THLOADICON_H

#include <list>
#include <vector>
#include <string>
#include "bburl.h"
#include "bbfile.h"

#include "thread/thposixthread.h"

class thLoadIcon : public thPosixThread {
public:
	typedef bool (*Callback)(void *, int, void *);

	thLoadIcon(void *userData, Callback cb);
	virtual ~thLoadIcon();

	/* 0x20 */ void *m_userData;  /* argument of the completion callback */
	/* 0x24 */ Callback m_completionCb;
	/* 0x28 */ int unk_28;
	/* 0x2C */ int unk_2C;

	/* icon.sys of the save being loaded */
	/* 0x30 */ BBMCIconSysDataPS2 m_iconSys;

	/* 0x230 */ string m_title;  /* UTF-8 title of the save */
	/* 0x234 */ short m_titleBreak;  /* byte offset of the second title line */
	/* 0x238 */ int m_damaged;  /* the save data is damaged */

	/* 0x23C */ string m_dataName;
	/* 0x240 */ int unk_240;  /* second argument of the completion callback */
	/* 0x244 */ void *m_buffer;  /* loaded icon */
	/* 0x248 */ int m_bufferSize;

	/* 0x24C */ string m_cacheName;
	/* 0x250 */ int m_loadNormalFlag;
	/* 0x254 */ int m_loadCopyFlag;
	/* 0x258 */ int m_loadDeleteFlag;
	/* 0x25C */ int m_endStatus;  /* 0 ok, 1 memory card error, 101 PFS error -5 */

	void init();
	int threadProc();
	int threadFinished();
	int threadEvent();

	void setDataName(const string &name);
	void setCacheName(const string &name);
	void setLoadNormalFlag(bool b);
	void setLoadCopyFlag(bool b);
	void setLoadDeleteFlag(bool b);
	void *getBuffer();
	int getEndStatus();
	void setFinishCallback(Callback cb);

	void abortProc();
	void releaseProc();

	static int InstanceCount();

	int checkIconSize(char *p, int limit);
	int CheckMemoryCard(string &path);
	int CheckMemoryCard(int code);
	int CheckPFS(int code);

	void cachewrite(unsigned char *data, int len, int mode);

	void data_load();

	int loadIconPS1(char *path, unsigned char **out);
	int loadPS1VMCSaveData(string &path);

	int loadMCSaveDataPS1(string &path);
	int loadIconPS2(string path, unsigned char **out);
	int loadDamagedData(string &path);
	int loadIconsysPS2(char *path, unsigned char **out, BBMCIconSysDataPS2 *sys);
	int loadPFSSaveDataPS1(string &path);
	int loadMCSaveDataPS2(string &path);
	int loadPFSSaveDataPS2(string &path);
	int loadIcon(BBFSType &fsType, string &a, string &b, string &c);
	int loadYourSysIconData(string &path);
	int IsDamagedDataPS2(string path, BBMCIconSysDataPS2 *sys, bool flag);

	static int instance_count;
};

/* thloadicon.cpp defines THLOADICON_IMPLEMENTATION and provides the
   out-of-line copies. */
#ifndef THLOADICON_IMPLEMENTATION
inline void thLoadIcon::setDataName(const string &name)
{
	m_dataName = name;
}

inline void thLoadIcon::setCacheName(const string &name)
{
	m_cacheName = name;
}

inline int thLoadIcon::getEndStatus()
{
	return m_endStatus;
}
#endif

#endif
