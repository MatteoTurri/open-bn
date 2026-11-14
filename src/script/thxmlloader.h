/*
 * thXmlLoader: thread that loads a PSML document (from the network or the
 * local cache) with its images and hands the window to create to a callback.
 */

#ifndef THXMLLOADER_H
#define THXMLLOADER_H

#include <string>
#include <map>
#include <stdlib.h>
#include <string.h>
#include "xml/xml_document.h"
#include "xml/xml_nnmap.h"
#include "scenewindow.h"
#include "app/buffer.h"
#include "thread/thposixthread.h"

/* thXmlLoader flags (m_flags). */
#define THXML_NO_HISTORY     0x01  /* not added to the history (a page reloaded from it) */
#define THXML_NO_MODAL_COUNT 0x02  /* a system window: not counted in for_modal_window_count */
#define THXML_DOCUMENT_ONLY  0x04  /* stop at the parsed document, no window and no history entry */
#define THXML_POST_FORM      0x08  /* POST the buffer as a form (PSML 1.1) */
#define THXML_POST_ESCAPED   0x10  /* POST the URL-escaped buffer (PSML 1.0) */

class thXmlLoader : public thPosixThread {
public:
	thXmlLoader(const string &url, const string &name, const string &script, int mode, int flags, void *cbArg,
	    void (*cb)(void *, int, SceneWindow::CreateArg *), Buffer buf);
	virtual ~thXmlLoader();

	/* 0x20 */ string m_uri;  /* the URL without its query string */
	/* 0x24 */ string m_dir;  /* directory of m_uri, up to the last '/' */
	/* 0x28 */ string m_query;  /* query string of the URL */
	/* 0x2C */ string m_name;
	/* 0x30 */ Buffer m_buffer;  /* the Buffer argument of the constructor */
	/* 0x34 */ int m_mode;  /* window mode: SceneWindow::MODE_* */
	/* 0x38 */ unsigned int m_flags;  /* THXML_* */
	/* 0x3C */ string m_script;  /* argument string of the page script */
	/* 0x40 */ string m_key;  /* key of the script modules */
	/* 0x44 */ int m_screenW;
	/* 0x48 */ int m_screenH;
	/* 0x4C */ int m_screenD;
	/* 0x50 */ int m_fadeoutTime;
	/* 0x54 */ void *m_cbArg;  /* first argument of the callback */
	/* 0x58 */ void (
	    *m_callback)(void *, int, SceneWindow::CreateArg *);  /* optional; gets the window to create, or 0 */
	/* 0x5C */ Document m_document;
	/* 0x64 */ std::map<string, FileImagePtr> m_imageMap;
	/* 0x70 */ std::map<string, string> m_uriMap;
	/* 0x7C */ int m_resultCode;  /* 200 = OK */

	/* 0x80 */ int m_psmlVersion;
	/* 0x84 */ int m_clientPsmlVersion;
	/* 0x88 */ int m_loginStage;  /* threadEvent: 0 open the login window, 1 wait for it */
	/* 0x8C */ int m_loginRetry;  /* "retry=" value passed to the login window */

	int threadProc();
	int threadEvent();
	int threadFinished();

	void abortProc();
	void releaseProc();

	int psml_version_check();
	int IsFromNetwork();
	void SetFadeoutTime(int t);
	const string &getURI() const;
	int PSMLVersion() const;
	void SetClientPSMLVersion(int v);

	static int IsLoadingModalWindow();
	static void SetCryptionState(int v);
	static int GetCryptionState();
	static int InstanceCount();

	static int for_modal_window_count;
	static int for_cryption_state;
	static int instance_count;
};  /* size = 0x90 */

/* Resolves path against the directory dir; the rules depend on the PSML version. */
string GetFullPath(const string &dir, const string &path, int psmlVersion);

#endif
