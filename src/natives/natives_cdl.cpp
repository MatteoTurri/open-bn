/*
 * DnasExtension: Init/Exit and the Small/amx natives cdl* (DNAS download
 * queue), inst* (title installation) and auth* (DNAS authentication), in the
 * tables smallext_dnas and smallext_dnas_p.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <string>

#include "../../ref/small-fmt6/amx.h"
#include "natives/smallext.h"

#include "xml/xml.h"
#include "xml/xml_nnmap.h"
#include "xml/xml_element.h"
#include "xml/xmltagtable.h"

#include "xml/xml_filedescriptor.h"
#include "xml/sax.h"
#include "xml/xml_parser.h"
#include "xml/xml_scannerstate.h"
#include "xml/xml_inputstream.h"

#include <stdexcept>
#include "dnas/dnas2cdl.h"
#include "dnas/scednas2.h"
#include "lib/scepfsd.h"
#include "fs/bbfile.h"
#include "script/scenewindow.h"
#include "script/script.h"
#include "natives_cdl.h"
#include "app/stringutil.h"
#include "install/scetryinst.h"
#include "thread/thposixthread.h"
#include "thread/thjob.h"
#include "timeextension.h"

/* File-scope strings read by cdl_check_run. */
static string s_patName;
static string s_titleSubId;
static string s_patSize;

/* True when [p,q] lies entirely in the script heap or entirely in its stack. */
static inline int amx_addr_ok(AMX *amx, cell p, cell q)
{
	if (p >= 0 && p < amx->hea && q >= 0 && q < amx->hea)
		return 1;
	if (p >= amx->stk && p < amx->stp && q >= amx->stk && q < amx->stp)
		return 1;
	return 0;
}

/* Value of attribute name of an XML element, as a string ("" if absent) or as
   a decimal integer (0 if absent). */
static string dat_xml_attr_string(NamedNodeMap *attrs, const char *name)
{
	Node n = attrs->getNamedItem(XMLString(name, false));

	if (n != 0 && n.get_nodeType() != 0)
		return (const char *)n.get_nodeValue().GetBufferPointer();
	return "";
}

static int dat_xml_attr_int(NamedNodeMap *attrs, const char *name)
{
	Node n = attrs->getNamedItem(XMLString(name, false));

	if (n != 0 && n.get_nodeType() != 0)
		return strtol((const char *)n.get_nodeValue().GetBufferPointer(), 0, 10);
	return 0;
}

static int g_instRunning;  /* nonzero while an installation runs */
static thJob *g_cdlJob;  /* job of cdlCheckStart */
static thJob *g_authJob;  /* job of authStart */
static int g_cdlResult;  /* result of cdl_check_run */
static int g_cdlError;  /* raw code of an unexpected error */
static int g_authResult;  /* result of auth_job_run */
static int g_cdlDlSize;  /* download size of the entry */
static int g_cdlInstSize;  /* install size of the entry */

void DnasExtension_Init()
{
	g_instRunning = 0;
	g_cdlJob = 0;
	g_authJob = 0;
	g_cdlResult = 1;
	g_cdlError = 0;
	g_authResult = 0;
}

void DnasExtension_Exit()
{}

/* Body of the cdlCheckStart job: checks the version (sceTRYInstVerCheck, modes
   1 and 2) and the space (sceTRYInstSizeCheck) of the chosen entry.
   g_cdlResult becomes 1 on success or a negative reason; an unexpected error
   gives -1 with its raw code in g_cdlError. */
static void cdl_check_run(thJob *job)
{
	char dlsize[32];
	char instsize[32];
	int ver = 1;
	int inst = 1;
	int r;
	int res;

	r = sceTRYInstVerCheck(s_patName.c_str(), (char *)s_titleSubId.c_str(), 1);
	if (r != 0) {
		switch (r) {
		case 1:
			ver = -100;
			break;
		case 2:
			ver = -101;
			break;
		case -1:
		default:
			g_cdlResult = -1;
			g_cdlError = r;
			return;
		}
	}
	r = sceTRYInstVerCheck(s_patName.c_str(), (char *)s_titleSubId.c_str(), 2);
	if (r != 0) {
		switch (r) {
		case 1:
			inst = -200;
			break;
		case 2:
			inst = -201;
			break;
		case -1:
		default:
			g_cdlResult = -1;
			g_cdlError = r;
			return;
		}
	}
	if (ver == -101 || inst == -201) {
		g_cdlResult = -101;
		return;
	}
	if (ver == -100 && inst == -200) {
		g_cdlResult = -100;
		return;
	}
	sprintf(dlsize, "%d", g_cdlDlSize);
	sprintf(instsize, "%d", g_cdlInstSize);
	r = sceTRYInstSizeCheck(dlsize, s_patName.c_str(), s_patSize.c_str(), instsize);
	if (r < 0)
		switch (r) {
		case -27705:
			res = -300;
			break;
		case -27708:
			res = -301;
			break;
		case -27709:
			res = -302;
			break;
		default:
			g_cdlResult = -1;
			g_cdlError = r;
			return;
		}
	else
		res = 1;
	g_cdlResult = res;
}

/* Stops and releases the cdlCheckStart job; always true. */
static bool cdl_check_poll(thJob *job, void *arg)
{
	if (g_cdlJob != 0) {
		g_cdlJob->abort();
		g_cdlJob->release();
		g_cdlJob = 0;
	}
	return true;
}

static void auth_job_run(thJob *job);
static bool auth_job_poll(thJob *job, void *arg);

static cell AMX_NATIVE_CALL n_cdlCheckStart(AMX *amx, cell *params)
{
	DNAS2DLTag info;

	if (params[1] < 0 || params[1] >= sceDNAS2CDLQueueGetTotalEntryNum()) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	sceDNAS2CDLQueueGetEntryInfo(&info, params[1]);
	if (sceTRYLibVersionCheck(&info) >= 0) {
		g_cdlResult = 0;
		g_cdlError = 0;
		g_cdlJob = new thJob(0, cdl_check_run, cdl_check_poll, 0);
		s_patName = info.pat_name;
		s_titleSubId = info.title_sub_id;
		g_cdlDlSize = info.dldata_size;
		s_patSize = info.pat_size;
		g_cdlInstSize = info.inst_size;
		g_cdlJob->start();
	} else {
		g_cdlResult = -1;
		g_cdlError = -27713;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_cdlCheckSync(AMX *amx, cell *params)
{
	if (g_cdlJob != 0)
		return 0;
	return g_cdlResult;
}

static cell AMX_NATIVE_CALL n_cdlCheckGetError(AMX *amx, cell *params)
{
	return g_cdlError;
}

static cell AMX_NATIVE_CALL n_cdlInit(AMX *amx, cell *params)
{
	int r;

	BBFileAccessExitCPP();
	r = scepfsInit();
	fprintf(stderr, "ret of scepfsInit() = %d\n", r);
	return 0;
}

static cell AMX_NATIVE_CALL n_cdlExit(AMX *amx, cell *params)
{
	int r;

	if (g_instRunning != 0) {
		r = sceTRYInstExit();
		fprintf(stderr, "ret of sceTRYInstExit() = %d\n", r);
		g_instRunning = 0;
	}
	scepfsExit();
	BBFileAccessInitCPP();
	return 0;
}

/* Queues a download: reads the attributes of XML element params[1] (a handle
   of the script node table), converts the enumerated values, makes the paths
   absolute with SceneWindow::MakeFullPath and inserts the entry at the head of
   the queue. An unknown type, content_type or inst_alloc_type raises
   AMX_ERR_NATIVE and returns 0. */
/* True when the script handle resolves to a node. */
static inline bool IsTagNode(XmlTagTable *list, unsigned handle)
{
	return GetTagNode(list, handle) != 0;
}

static cell AMX_NATIVE_CALL n_cdlQueAdd(AMX *amx, cell *params)
{
	NamedNodeMap attrs;
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (!IsTagNode(list, handle)) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	{
		Element e = GetTagNode(list, handle);

		attrs = e.get_attributes();
	}

	string id, ver, title, title_id, title_sub_id, do_id, dist_id;
	string nextref, cref, cpage, kref, kpage, ackref;
	string pat_name, pat_size, thumbnail_image, application_name;
	SceneWindow *win = GetSceneWindow(amx);
	int type;
	unsigned int dldata_size, inst_size;
	int content_type, inst_alloc_type;
	int dist_no;
	int jump_flag;

	id = dat_xml_attr_string(&attrs, "id");
	ver = dat_xml_attr_string(&attrs, "ver");
	{
		string s = dat_xml_attr_string(&attrs, "type");

		if (s == "CONTENT")
			type = 0;
		else if (s == "KEY")
			type = 1;
		else if (s == "AUTO_CONT_KEY")
			type = 2;
		else if (s == "AUTO_KEY_CONT")
			type = 3;
		else if (s == "REGDATA")
			type = 4;
		else {
			amx_RaiseError(amx, 10);
			return 0;
		}
	}
	title = dat_xml_attr_string(&attrs, "title");
	{
		StringUtil t = title;

		StringUtil &r = t;
		r.Replace("\\n", "\n");
		r.Replace("\\\\", "\\");
		title = r;
	}
	title_id = dat_xml_attr_string(&attrs, "title_id");
	title_sub_id = dat_xml_attr_string(&attrs, "title_sub_id");
	dldata_size = dat_xml_attr_int(&attrs, "dldata_size");
	inst_size = dat_xml_attr_int(&attrs, "inst_size");
	{
		string s = dat_xml_attr_string(&attrs, "content_type");

		if (s == "PS2CD")
			content_type = 0;
		else if (s == "PS2DVD")
			content_type = 1;
		else if (s == "PS1CD")
			content_type = 2;
		else if (s == "PS2HDD")
			content_type = 3;
		else {
			amx_RaiseError(amx, 10);
			return 0;
		}
	}
	{
		string s = dat_xml_attr_string(&attrs, "inst_alloc_type");

		if (s == "NORMAL")
			inst_alloc_type = 0;
		else if (s == "FLAT")
			inst_alloc_type = 1;
		else {
			amx_RaiseError(amx, 10);
			return 0;
		}
	}
	do_id = dat_xml_attr_string(&attrs, "do_id");
	dist_id = dat_xml_attr_string(&attrs, "dist_id");
	dist_no = dat_xml_attr_int(&attrs, "dist_no");
	nextref = dat_xml_attr_string(&attrs, "nextref");
	win->MakeFullPath(nextref);
	cref = dat_xml_attr_string(&attrs, "cref");
	win->MakeFullPath(cref);
	cpage = dat_xml_attr_string(&attrs, "cpage");
	win->MakeFullPath(cpage);
	kref = dat_xml_attr_string(&attrs, "kref");
	win->MakeFullPath(kref);
	kpage = dat_xml_attr_string(&attrs, "kpage");
	win->MakeFullPath(kpage);
	ackref = dat_xml_attr_string(&attrs, "ackref");
	win->MakeFullPath(ackref);
	pat_name = dat_xml_attr_string(&attrs, "pat_name");
	pat_size = dat_xml_attr_string(&attrs, "pat_size");
	thumbnail_image = dat_xml_attr_string(&attrs, "thumbnail_image");
	win->MakeFullPath(thumbnail_image);
	application_name = dat_xml_attr_string(&attrs, "application_name");
	{
		string s = dat_xml_attr_string(&attrs, "jump_flag");

		if (s == "game.ch" || s == "0")
			jump_flag = 0;
		else if (s == "movie.ch" || s == "1")
			jump_flag = 1;
		else if (s == "boot")
			jump_flag = 2;
		else if (s == "donothing")
			jump_flag = 3;
		else
			jump_flag = 3;
	}

	DNAS2DLTag tag;

	memset(&tag, 0, sizeof(tag));
	tag.tag_flag = 1;
	tag.id = (char *)id.c_str();
	tag.ver = (char *)ver.c_str();
	tag.tag_type = type;
	tag.title = (char *)title.c_str();
	tag.title_id = (char *)title_id.c_str();
	tag.title_sub_id = (char *)title_sub_id.c_str();
	tag.dldata_size = dldata_size;
	tag.inst_size = inst_size;
	tag.inst.content_type = content_type;
	tag.inst.inst_alloc_type = inst_alloc_type;
	tag.do_id = (char *)do_id.c_str();
	tag.nextref = (char *)nextref.c_str();
	tag.cref = (char *)cref.c_str();
	tag.dist_id = (char *)dist_id.c_str();
	tag.dist_no = dist_no;
	tag.unk_50 = 1;
	tag.cpage = (char *)cpage.c_str();
	tag.kref = (char *)kref.c_str();
	tag.kpage = (char *)kpage.c_str();
	tag.ackref = (char *)ackref.c_str();
	tag.pat_name = (char *)pat_name.c_str();
	tag.pat_size = (char *)pat_size.c_str();
	tag.thumbnail_image = (char *)thumbnail_image.c_str();
	tag.application_name = (char *)application_name.c_str();
	tag.jump_flag = jump_flag;
	return sceDNAS2CDLQueueInsertAt(&tag, 0);
}

static cell AMX_NATIVE_CALL n_cdlQueSize(AMX *amx, cell *params)
{
	return sceDNAS2CDLQueueGetTotalEntryNum();
}

/* cdlQueGetData(index, field, addr, size): copies field `field` (0..24, in the
   order cdlQueAdd reads them) of queue entry `index` into the script; strings
   are cut to size*4-1 bytes. */
static cell AMX_NATIVE_CALL n_cdlQueGetData(AMX *amx, cell *params)
{
	DNAS2DLTag info;

	if (params[1] < 0 || params[1] >= sceDNAS2CDLQueueGetTotalEntryNum()) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	sceDNAS2CDLQueueGetEntryInfo(&info, params[1]);

	int is_str = 0;
	string v;
	int num = 0;

	switch (params[2]) {
	case 0:
		v = info.id;
		is_str = 1;
		break;
	case 1:
		v = info.ver;
		is_str = 1;
		break;
	case 2:
		num = info.tag_type;
		break;
	case 3:
		v = info.title;
		is_str = 1;
		break;
	case 4:
		v = info.title_id;
		is_str = 1;
		break;
	case 5:
		v = info.title_sub_id;
		is_str = 1;
		break;
	case 6:
		num = info.dldata_size;
		break;
	case 7:
		num = info.inst_size;
		break;
	case 8:
		num = info.inst.content_type;
		break;
	case 9:
		num = info.inst.inst_alloc_type;
		break;
	case 10:
		v = info.do_id;
		is_str = 1;
		break;
	case 11:
		v = info.dist_id;
		is_str = 1;
		break;
	case 12:
		num = info.dist_no;
		break;
	case 13:
		num = info.unk_50;
		break;
	case 14:
		v = info.nextref;
		is_str = 1;
		break;
	case 15:
		v = info.cref;
		is_str = 1;
		break;
	case 16:
		v = info.cpage;
		is_str = 1;
		break;
	case 17:
		v = info.kref;
		is_str = 1;
		break;
	case 18:
		v = info.kpage;
		is_str = 1;
		break;
	case 19:
		v = info.ackref;
		is_str = 1;
		break;
	case 20:
		v = info.pat_name;
		is_str = 1;
		break;
	case 21:
		v = info.pat_size;
		is_str = 1;
		break;
	case 22:
		v = info.thumbnail_image;
		is_str = 1;
		break;
	case 23:
		v = info.application_name;
		is_str = 1;
		break;
	case 24:
		num = info.jump_flag;
		break;
	default:
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (is_str) {
		cell *cptr;

		if ((unsigned int)(params[4] * 4) < v.length() + 1)
			v = v.substr(0, params[4] * 4 - 1);
		if (!amx_addr_ok(amx, params[3], v.length() + params[3])) {
			amx_RaiseError(amx, 5);
			return 0;
		}
		amx_GetAddr(amx, params[3], &cptr);
		amx_SetString(cptr, (char *)v.c_str(), 1);
	} else {
		cell *cptr;

		if (amx_GetAddr(amx, params[3], &cptr) == 0)
			*cptr = num;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_cdlQueRemove(AMX *amx, cell *params)
{
	cell r;

	if (params[1] < 0 || params[1] >= sceDNAS2CDLQueueGetTotalEntryNum()) {
		amx_RaiseError(amx, 10);
		r = 0;
	} else {
		r = sceDNAS2CDLQueueRemove(params[1]);
	}
	return r;
}

static cell AMX_NATIVE_CALL n_cdlStart(AMX *amx, cell *params)
{
	int cookie;
	cell r;

	if (sceDNAS2CDLQueueGetTotalEntryNum() != 0) {
		memset(&cookie, 0, sizeof(cookie));
		cookie = 1;
		r = sceDNAS2CDLStart(0, 0, &cookie);
	} else {
		amx_RaiseError(amx, 10);
		r = 0;
	}
	return r;
}

static cell AMX_NATIVE_CALL n_cdlGetStatus(AMX *amx, cell *params)
{
	DNAS2CDLStatus st;
	cell *cptr;
	int r;

	r = sceDNAS2CDLGetStatus(&st);
	if (amx_GetAddr(amx, params[1], &cptr) == 0)
		*cptr = st.state;
	if (amx_GetAddr(amx, params[2], &cptr) == 0)
		*cptr = st.clen;
	if (amx_GetAddr(amx, params[3], &cptr) == 0)
		*cptr = st.pos;
	return r;
}

static cell AMX_NATIVE_CALL n_cdlAbort(AMX *amx, cell *params)
{
	return sceDNAS2CDLAbort();
}

static cell AMX_NATIVE_CALL n_cdlSuspend(AMX *amx, cell *params)
{
	int tid;

	tid = sceDNAS2CDLGetThreadID();
	if (params[1] != 0)
		pthread_kill(tid, 23);
	else
		pthread_kill(tid, 25);
	return 0;
}

static cell AMX_NATIVE_CALL n_instStart(AMX *amx, cell *params)
{
	int r;

	g_instRunning = 1;
	r = sceTRYInstPreCheck();
	if (r < 0)
		return r;
	r = sceTRYInstInit();
	if (r < 0) {
		fprintf(stderr, "### Error: sceTRYInstInit failed.\n");
		return r;
	}
	r = sceTRYInstStart();
	if (r < 0) {
		fprintf(stderr, "### Error: install start failed.\n");
		return r;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_instGetStatus(AMX *amx, cell *params)
{
	sceTRYInstStatus st;
	cell *cptr;

	sceTRYInstGetStatus(&st);
	if (amx_GetAddr(amx, params[1], &cptr) == 0)
		*cptr = st.state;
	if (amx_GetAddr(amx, params[2], &cptr) == 0)
		*cptr = st.result;
	if (amx_GetAddr(amx, params[3], &cptr) == 0)
		*cptr = st.bytesTotal;
	if (amx_GetAddr(amx, params[4], &cptr) == 0)
		*cptr = st.accum2;
	if (amx_GetAddr(amx, params[5], &cptr) == 0)
		*cptr = st.trailerTotal;
	if (amx_GetAddr(amx, params[6], &cptr) == 0)
		*cptr = st.trailer;
	return 0;
}

static cell AMX_NATIVE_CALL n_authStart(AMX *amx, cell *params)
{
	g_authResult = 0;
	g_authJob = new thJob(0, auth_job_run, auth_job_poll, 0);
	g_authJob->start();
	return 0;
}

static cell AMX_NATIVE_CALL n_authGetStatus(AMX *amx, cell *params)
{
	cell *cptr;

	if (g_authJob != 0)
		return 0;
	if (amx_GetAddr(amx, params[1], &cptr) == 0)
		*cptr = g_authResult;
	if (g_authResult == 0)
		return 1;
	return -1;
}

/* The two native tables of the module, read by script/script.cpp. */
AMX_NATIVE_INFO smallext_dnas[] = {
	{ "cdlQueAdd", n_cdlQueAdd },
	{ 0, 0 },
};

AMX_NATIVE_INFO smallext_dnas_p[] = {
	{ "cdlInit", n_cdlInit },
	{ "cdlExit", n_cdlExit },
	{ "cdlCheckStart", n_cdlCheckStart },
	{ "cdlCheckSync", n_cdlCheckSync },
	{ "cdlCheckGetError", n_cdlCheckGetError },
	{ "cdlQueSize", n_cdlQueSize },
	{ "cdlQueGetData", n_cdlQueGetData },
	{ "cdlQueRemove", n_cdlQueRemove },
	{ "cdlStart", n_cdlStart },
	{ "cdlGetStatus", n_cdlGetStatus },
	{ "cdlAbort", n_cdlAbort },
	{ "cdlSuspend", n_cdlSuspend },
	{ "instStart", n_instStart },
	{ "instGetStatus", n_instGetStatus },
	{ "authStart", n_authStart },
	{ "authGetStatus", n_authGetStatus },
	{ 0, 0 },
};

/* Parameter of sceDNAS2Init and status of sceDNAS2GetStatus share one buffer. */
union DnasAuthWork {
	DNAS2InitConf param;
	DNAS2Status status;
};

/* Loads a whole file into memory and stores its length in *size. */
static void *dnas_load_file(const char *path, int *size)
{
	FILE *fp;
	void *buf;

	fp = fopen(path, "rb");
	if (fp != 0) {
		fseek(fp, 0, SEEK_END);
		*size = ftell(fp);
		rewind(fp);
		buf = malloc(*size);
		if (buf != 0) {
			if (fread(buf, 1, *size, fp) == *size) {
				fclose(fp);
				return buf;
			}
			free(buf);
			fclose(fp);
			return 0;
		}
		fclose(fp);
	}
	return 0;
}

/* Body of the authStart job, a state machine: 1 = sceDNAS2Init with the DNAS
   file data, 3 = sceDNAS2AuthInstall, 4 = wait on the status (sceDNAS2Abort
   after 30 s without progress), 2 = close, or retry from 3 after "different
   HDD". g_authResult gets 0 on success, otherwise the DNAS code - 20000. */
static void auth_job_run(thJob *job)
{
	char path[128];
	DnasAuthWork w;
	int r = sceTRYInstGetDNASDatapath(path);
	void *data = 0;
	unsigned int startTick = 0;
	int canceled = 0;
	int size = 0;
	int last = 0;
	int retry;
	int state;

	switch (r) {
	case 0:
		data = dnas_load_file(path, &size);
		break;
	case -27951:
		g_authResult = r;
		goto done;
	case -27952:
	case -27953:
	case -27954:
		g_authResult = r;
		goto done;
	}
	retry = 0;
	state = 1;
	do {
		switch (state) {
		case 1:
			memset(&w.param, 0, sizeof(w.param));
			w.param.flags = 1;
			w.param.pass_phrase = (char *)data;
			w.param.pass_phrase_len = 8;
			w.param.auth_data = (char *)data + 8;
			w.param.auth_data_len = 0x10000;
			fprintf(stderr, "before sceDNAS2Init()\n");
			r = sceDNAS2Init(&w.param, 48, 0x8000, 0, 0);
			if (r < 0) {
				fprintf(stderr, "return value of sceDNAS2Init(): %d\n", r);
				state = 0;
				g_authResult = r - 20000;
			} else
				state = 3;
			break;
		case 3:
			fprintf(stderr, "before sceDNAS2AuthInstall()\n");
			r = sceDNAS2AuthInstall();
			if (r < 0) {
				fprintf(stderr, "return value of sceDNAS2AuthInstall() = %d\n", r);
				state = 2;
				g_authResult = r - 20000;
			} else {
				startTick = tickcount();
				state = 4;
			}
			break;
		case 4:
			memset(&w.status, 0, sizeof(w.status));
			if (sceDNAS2GetStatus(&w.status) >= 0 && w.status.code != 5 && w.status.code != 8) {
				if (w.status.code == 6 && w.status.sub_code == -109) {
					fprintf(stderr, "different HDD\n");
					retry = 1;
				} else if (w.status.code == 6 || w.status.code == 7) {
					fprintf(stderr, "state: %d, sub: %d\n", w.status.code, w.status.sub_code);
					g_authResult = w.status.sub_code - 20000;
				} else if (w.status.code == 2) {
					fprintf(stderr, "DNAS aborted ...\n");
					g_authResult = -20601;
				} else {
					if (!canceled) {
						if (last != w.status.progress) {
							last = w.status.progress;
							startTick = tickcount();
						} else if (tickcount() - startTick > 30000) {
							fprintf(stderr, "canceled ...\n");
							canceled = 1;
							sceDNAS2Abort();
						}
					}
					usleep(63);
					break;
				}
			}
			/* the end of the wait continues as state 2 */
		case 2:
			if (retry) {
				state = 3;
				retry = 0;
			} else {
				sceDNAS2Shutdown();
				state = 0;
			}
			break;
		}
	} while (state != 0);
done:
	if (data != 0)
		free(data);
}

static bool auth_job_poll(thJob *job, void *arg)
{
	if (g_authJob != 0) {
		g_authJob->abort();
		g_authJob->release();
		g_authJob = 0;
	}
	return true;
}
