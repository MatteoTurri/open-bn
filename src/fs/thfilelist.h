/*
 * thFileList: worker thread that lists a directory, memory card, PS1 virtual
 * memory card or PFS partition into a sorted list of fmEntry.
 * Interface header: thfilelist.cpp emits the inline members.
 */

#ifndef THFILELIST_H
#define THFILELIST_H

#pragma interface

#include <list>
#include <string>
#include "bburl.h" // for BBFSType
#include "fmentry.h"
#include "thread/thposixthread.h"

class thFileList : public thPosixThread {
public:
	thFileList(void *userData, bool (*completionCb)(void *, void *))
	        : m_userData(userData), m_completionCb((int (*)(void *, thFileList *))completionCb)
	{
		init();
	}
	virtual ~thFileList();

	/* 0x20 */ void *m_userData;  /* completion callback argument */
	/* 0x24 */ int (*m_completionCb)(void *, thFileList *);  /* called by threadFinished with (userData, this) */

	/* 0x28 */ string m_url;
	/* 0x2C */ list<unsigned int> m_findContentsTypeList;
	/* 0x30 */ int m_onlyDirFlag;
	/* 0x34 */ int m_onlyFileFlag;
	/* 0x38 */ int m_noParentDirFlag;
	/* 0x3C */ int m_recursiveFlag;
	/* 0x40 */ fmentry_order m_entryListOrder;
	/* 0x44 */ list<fmEntry> m_entryList;
	/* 0x48 */ int m_result;  /* 0, or -1 when the listing failed */

	void init();
	void setUrl(const char *url);

	int threadProc();
	int threadFinished();

	void setOnlyDirFlag() { m_onlyDirFlag = 1; }
	void resetOnlyDirFlag() { m_onlyDirFlag = 0; }
	void setOnlyFileFlag() { m_onlyFileFlag = 1; }
	void resetOnlyFileFlag() { m_onlyFileFlag = 0; }
	void setNoParentDirFlag() { m_noParentDirFlag = 1; }
	void reseNoParentDirFlag() { m_noParentDirFlag = 0; }
	void setRecursiveFlag() { m_recursiveFlag = 1; }
	void resetRecursiveFlag() { m_recursiveFlag = 0; }
	void clearFindContentsTypeList() { m_findContentsTypeList.clear(); }
	void addFindContentsTypeList(unsigned int type) { m_findContentsTypeList.push_back(type); }
	void setEntryListOrder(fmentry_order order) { m_entryListOrder = order; }
	void abortProc() {}
	void releaseProc() {}

	unsigned int judgeContentsType(unsigned int type);

	int CheckFileName(char *name);
	void entry_sort();

	int IsDamagedData(fmEntry &entry, string &outMsg);
	void MakeFileListUrl(string &urlOut);
	int CheckFileType(string &path, unsigned int flags);
	int findFile(string &url, BBFSType &fsType);
	void MakePS1VMCListUrl(string &urlOut);
	void MakeMCListUrl(string &urlOut);
	void MakePFSListUrl(string &url1, string &url2);
};

#endif /* THFILELIST_H */
