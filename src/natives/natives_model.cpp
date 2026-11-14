/*
 * Small/amx natives "model*" (table smallext_model): loads a 3D model (.bmo)
 * on a background thJob and registers it in ModelTable. The per-script state
 * is a ModelLoadData attached to the Script as user data '_MDL'.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <string>
#include <vector>
#include "amx.h"
#include "natives/smallext.h"
#include "http/httpfile.h"
#include "gfx/tm.h"

/* Script__Abort3 decrypts an encrypted model (.bmo) in place. */
#include "script/script_abort_entry.h"

#include "gfx/bt2ipu.h"

#include "script/script.h"
#include "scene/hook.h"
#include "scene/modeldata.h"
#include "scene/objecttable.h"
#include "gfx/gm_graph.h"
#include "app/jobinit2.h"
#include "thread/thposixthread.h"
#include "thread/thjob.h"
#include "xml/xml_filedescriptor.h"
#include "script/scenewindow.h"

static const long TAG_PSCR = 0x52435350;   /* 'PSCR' */
static const long TAG_MDL = 0x4c444d5f;   /* '_MDL' */

/* Load job slots: m_str[0] URL, m_ptr[0] the file read (malloc), and the
   m_int slots below. */
enum {
	MODEL_JOB_OPTION = 0, /* load option */
	MODEL_JOB_ENCRYPTED = 1, /* nonzero = encrypted model (.bmo) */
	MODEL_JOB_SIZE = 2, /* size of the file read */
	MODEL_JOB_FROM_NET = 3 /* the script comes from the network */
};

/* Per-script state of the module, user data '_MDL' of the Script. */
class ModelLoadData {
public:
	/* 0x00 */ thJob *thread;               /* the loading job */
	/* 0x04 */ int loadState;               /* 1 = done, 2 = error */
	/* 0x08 */ int handle;                  /* loaded model (ModelTable handle) */
	/* 0x0C */ vector<ModelData *> keep;    /* models kept by modelKeep */

	static int instance_count;

	ModelLoadData() : thread(0), loadState(0), handle(0) { ++instance_count; }
	~ModelLoadData();

	void ReleaseThread();
	void ReleaseModel();
	void ReleaseKeptModel();
};

int ModelLoadData::instance_count = 0;

inline ModelLoadData::~ModelLoadData()
{
	--instance_count;
	ReleaseThread();
	ReleaseModel();
	ReleaseKeptModel();
}

inline void ModelLoadData::ReleaseThread()
{
	if (thread && thread->m_tid)
		pthread_kill(thread->m_tid, 16); /* SIGUSR1 */
	if (thread) {
		thread->abort();
		thread->release();
		thread = 0;
	}
}

inline void ModelLoadData::ReleaseModel()
{
	if (handle) {
		ModelData *m = ModelTable::Get(handle);
		if (--m->refcount == 0) {  /* NOTE: bug: the refcount is decremented before the null check */
			if (m)
				delete m;
		}
		handle = 0;
	}
}

inline void ModelLoadData::ReleaseKeptModel()
{
	vector<ModelData *>::iterator it;

	for (it = keep.begin(); it != keep.end(); ++it) {
		ModelData *m = *it;
		if (--m->refcount == 0) {  /* NOTE: bug: the refcount is decremented before the null check */
			if (m)
				delete m;
		}
	}
	keep.erase(keep.begin(), keep.end());
}

/* The model load state of a script, user data '_MDL'. */
static inline ModelLoadData *ScriptModelData(Script *owner)
{
	return (ModelLoadData *)owner->GetUserData(TAG_MDL);
}

/* The script that runs amx, user data 'PSCR' of the AMX. */
static inline Script *AmxScript(AMX *amx)
{
	Script *s;

	amx_GetUserData(amx, TAG_PSCR, (void **)&s);
	return s;
}

static inline ModelLoadData *AmxModel(AMX *amx)
{
	return ScriptModelData(AmxScript(amx));
}

/* Script model handle -> its ModelData (0 if not registered). */
static inline ModelData *ModelFromHandle(cell h)
{
	return ModelTable::Get(h);
}

static void ModelFetchUrl(thJob *job);
static bool ModelFetchDone(thJob *job, void *user);
static void ModelFreeBuf(thJob *job);

/* Delete callback of the user data, registered on the Script by ModelGetData. */
static void ModelDelete(AMX *amx)
{
	ModelLoadData *m;

	m = ScriptModelData(AmxScript(amx));
	if (m != 0)
		delete m;
}

/* Returns the user data of the module, stopping any previous job, or creates it. */
static ModelLoadData *ModelGetData(AMX *amx)
{
	ModelLoadData *m = AmxModel(amx);

	if (m != 0) {
		if (m->thread != 0) {
			if (m->thread->m_tid != 0)
				pthread_kill(m->thread->m_tid, 16);
			if (m->thread != 0) {
				m->thread->abort();
				m->thread->release();
				m->thread = 0;
			}
		}
	} else {
		m = new ModelLoadData;
		AmxScript(amx)->SetUserData(TAG_MDL, m);
		AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_MDL, ModelDelete);
	}
	return m;
}

/*
 * Fixes the extension of path: false when it is already bad, true when it is
 * already ext; otherwise appends ext and returns true.
 */
static bool ModelFixExt(string &path, const string &bad, const string &ext)
{
	unsigned int pos = path.find_last_of(".");

	if (pos != string::npos) {
		string e = path.substr(pos);

		if (e == bad)
			return false;
		if (e == ext)
			return true;
	}
	path += ext;
	return true;
}

/*
 * modelLoadStart(url, opt): starts loading the model; a .gmo name is refused
 * (AMX_ERR_NATIVE), any other name is loaded as .bmo.
 */
static cell AMX_NATIVE_CALL n_modelLoadStart(AMX *amx, cell *params)
{
	string url = CellToString(amx, params[1]);
	int opt = params[2];
	bool net = GetSceneWindow(amx)->IsFromNetwork();
	bool enc = ModelFixExt(url, ".gmo", ".bmo");
	ModelLoadData *m;

	if (!enc) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	GetSceneWindow(amx)->MakeFullPath(url);
	m = ModelGetData(amx);
	m->thread = new thJob(amx, ModelFetchUrl, ModelFetchDone, ModelFreeBuf);
	m->thread->m_str[0] = url;
	m->thread->m_int[MODEL_JOB_OPTION] = opt;
	m->thread->m_int[MODEL_JOB_ENCRYPTED] = enc;
	m->thread->m_int[MODEL_JOB_FROM_NET] = net;
	m->thread->addThreadIdleList(1, 10000);
	m->loadState = 0;
	return 0;
}

/*
 * modelLoadSync(): 0 while loading, 1 once loaded, -1 on error or with nothing
 * loaded. The result is consumed: the next call returns -1.
 */
static cell AMX_NATIVE_CALL n_modelLoadSync(AMX *amx, cell *params)
{
	ModelLoadData *m = AmxModel(amx);
	int st;

	if (m == 0)
		return -1;
	if (m->thread != 0)
		return 0;
	st = m->loadState;
	if (st != 1) {
		m->loadState = 0;
		return -1;
	}
	m->loadState = 0;
	return 1;
}

/*
 * modelGetInfo(index, &value): index 0 = handle of the loaded model; any other
 * index raises AMX_ERR_NATIVE.
 */
static cell AMX_NATIVE_CALL n_modelGetInfo(AMX *amx, cell *params)
{
	ModelLoadData *m = AmxModel(amx);

	if (m == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	int ok = 0;
	string s;
	cell *dest;
	cell *ip;
	cell len, addr, end;
	int val = 0;
	int valid;

	switch (params[1]) {
	case 0:
		val = m->handle;
		break;
	default:
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (ok) {  /* redundant as written */
		if (params[3] * 4 < s.length() + 1)
			s = string(s, 0, params[3] * 4 - 1);
		len = s.length();
		addr = params[2];
		end = len + addr;
		if (addr >= 0 && addr < amx->hea && end >= 0 && end < amx->hea)
			valid = 1;
		else
			valid = addr >= amx->stk && addr < amx->stp && end >= amx->stk && end < amx->stp;
		if (!valid) {
			amx_RaiseError(amx, AMX_ERR_MEMACCESS);
			return 0;
		}
		amx_GetAddr(amx, params[2], &dest);
		amx_SetString(dest, (char *)s.c_str(), 1);
	} else {
		if (amx_GetAddr(amx, params[2], &ip) == 0)
			*ip = val;
	}
	return 0;
}

/* modelKeep(handle): keeps a reference to the model until modelRelease. */
static cell AMX_NATIVE_CALL n_modelKeep(AMX *amx, cell *params)
{
	ModelLoadData *m = AmxModel(amx);

	if (m == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (params[1] != 0) {
		ModelData *d = ModelFromHandle(params[1]);

		if (d != 0) {
			m->keep.push_back(d);
			d->refcount++;
		}
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_modelRelease(AMX *amx, cell *params)
{
	ModelLoadData *m = AmxModel(amx);
	ModelData *d;
	vector<ModelData *>::iterator it;

	if (m == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (params[1] != 0) {
		d = ModelFromHandle(params[1]);
		if (d != 0) {
			for (it = m->keep.begin(); it != m->keep.end(); ++it) {
				if (*it == d) {
					if (--d->refcount == 0)
						delete d;
					m->keep.erase(it);
					return 0;
				}
			}
		}
	}
	return 0;
}

AMX_NATIVE_INFO smallext_model[] = {
	{ "modelLoadStart", n_modelLoadStart },
	{ "modelLoadSync", n_modelLoadSync },
	{ "modelGetInfo", n_modelGetInfo },
	{ "modelKeep", n_modelKeep },
	{ "modelRelease", n_modelRelease },
	{ 0, 0 },
};

/* 'HNDL': user data tag holding the handle of the model */
static const unsigned int TAG_HNDL = 0x4c444e48;

/* Destroy callback set by ModelRegist: drops the handle of the model from ModelTable. */
static void ModelRemoveByHook(Hook *h, void *, void *)
{
	void *p;

	if (h == 0)
		p = 0;
	else
		p = h->GetUserData(TAG_HNDL);
	ModelTable::Remove((int)p);
}

/* Registers a new ModelData and returns its handle; destroys it when refused. */
static int ModelRegist(ModelData *d, bool net)
{
	int h;

	d->SetCallback(0, ModelRemoveByHook, 0);
	h = ModelTable::Regist(d, net);
	d->SetUserData(TAG_HNDL, (void *)h);
	if (h == 0)
		delete d;
	return h;
}

/*
 * Second job callback (main thread): releases the previous model, builds the
 * ModelData from the file read, turns its Tim2 images into textures and sets
 * loadState.
 */
static bool ModelFetchDone(thJob *job, void *user)
{
	AMX *amx = (AMX *)user;
	string url = job->m_str[0];
	bool net = job->m_int[MODEL_JOB_FROM_NET];
	int size = job->m_int[MODEL_JOB_SIZE];
	void *buf = job->m_ptr[0];
	ModelLoadData *m = AmxModel(amx);

	if (m->handle != 0) {
		ModelData *d = ModelFromHandle(m->handle);

		if (--d->refcount == 0)
			delete d;
		m->handle = 0;
	}
	if (buf != 0) {
		ModelData *d;
		int i, h;

		g_graph.SetPacketFence(0, 0x40000);
		d = new ModelData(buf, size, url.c_str());
		g_graph.SetPacketFence(0, 0);
		for (i = 0; i < d->get_texture_count(); i++) {
			gmo_texture *t = d->get_texture(i);

			int *img = t->image_data;
			int len = t->image_len;

			if (img != 0) {
				int id = Tim2_GetTexID(img, len, net);

				t->set_texture_handle(id);
				t->set_file_image(0, 0);
				if (id != 0)
					tmRelease(id);
			}
		}
		h = ModelRegist(d, net);
		if (h != 0) {
			m->handle = h;
			m->loadState = 1;
		} else
			m->loadState = 2;
	} else
		m->loadState = 2;
	if (m->thread != 0) {
		m->thread->abort();
		m->thread->release();
		m->thread = 0;
	}
	return true;
}

/* Reads a whole file into a malloc buffer; 0 on error. */
static void *ModelLoadFile(const char *path, int *size)
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
			if (fread(buf, 1, *size, fp) == (unsigned)*size) {
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

/*
 * First job callback (worker thread): resolves the URL to a local path
 * (HTTP download, "file:" or plain path), reads the file and decrypts it for
 * a .bmo model.
 */
static void ModelFetchUrl(thJob *job)
{
	string url = job->m_str[0];
	bool enc = job->m_int[MODEL_JOB_ENCRYPTED];
	HttpFile hf;
	sceHttpFileRequestArg req;
	int cookie;
	int ret;

	memset(&req, 0, sizeof(req));
	req.done_flag = &cookie;
	req.timeout = 30000;
	string path;
	if (strncasecmp(url.c_str(), "http:", 5) == 0 || strncasecmp(url.c_str(), "https:", 6) == 0) {
		ret = hf.Request(url, 0, 0, &req);
		if (cookie)
			hf.save_cookie();
		path = (const char *)hf;
	} else if (strncasecmp(url.c_str(), "file:", 5) == 0) {
		ret = 200;
		path = url.substr(5);
	} else {
		ret = 200;
		path = url;
	}
	if (ret == 200) {
		int size;
		void *buf = ModelLoadFile(path.c_str(), &size);

		if (buf != 0) {
			if (enc) {
				if (Script__Abort3(size, buf, &size) >= 0) {
					job->m_ptr[0] = buf;
					job->m_int[MODEL_JOB_SIZE] = size;
					return;
				}
				return;  /* NOTE: bug: the file buffer leaks when decoding fails */
			}
			job->m_ptr[0] = buf;
			job->m_int[MODEL_JOB_SIZE] = size;
			return;
		}
	}
	job->m_ptr[0] = 0;
	job->m_int[MODEL_JOB_SIZE] = 0;
}

/* Third job callback: frees the file read. */
static void ModelFreeBuf(thJob *job)
{
	if (job->m_ptr[0] != 0)
		free(job->m_ptr[0]);
}
