/*
 * Small/amx natives "img*" (table smallext_img): image decoding, trimming and
 * texture handling for scripts, plus the download queue of the image fetch
 * jobs (ImgExtension_*). The per-script state is an ImgData attached to the
 * Script as user data '_IMG'. Texture handles seen by scripts are xor-ed with
 * HANDLE_XOR (0 stays 0).
 */

#include "amx.h"
#include "natives/smallext.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <algorithm>
#include <iostream.h>
#include <string>
#include <vector>

#include "app/strutil.h"
#include "gfx/tm.h"

#include "app/resource.h"
#include "gfx/bt2ipu.h"

#include "fs/bbfile.h"

#include "script/script.h"
#include "natives_img.h"
#include "thread/thposixthread.h"
#include "thread/thjob.h"
#include "gfx/pltm.h"
#include "gfx/font.h"
#include "http/httpfile.h"
#include "image/thimagedecoder.h"
#include "gfx/texture.h"
#include "xml/xml_filedescriptor.h"

static const long TAG_PSCR = 0x52435350;   /* 'PSCR' */
static const long TAG_IMG = 0x474d495f;   /* '_IMG' */

/* Mask of the texture handles seen by scripts. */
static const int HANDLE_XOR = 0x89f974c3;

/* Job slots. Fetch job: m_str[0] URL, m_str[1] local path. Tim2 job:
   m_str[0] path, m_ptr[0] the file read. The m_int slots are below. */
enum {
	IMG_FETCH_CACHE_ONLY = 0, /* fetch job: nonzero = cache only */
	IMG_FETCH_STATUS = 1, /* fetch job: HTTP status */
	IMG_TIM2_SIZE = 0 /* Tim2 job: size */
};


/* Per-script state of the module, user data '_IMG' of the Script. */
struct ImgData {
	/* 0x00 */ thPosixThread *thread;  /* load thread: Tim2 job, thImageDecoder or string job */
	/* 0x04 */ thJob *fetchJob;  /* fetch job of an http: URL */
	/* 0x08 */ int reqWidth;  /* requested width; string options (<< 6 for PLTM) for a string job */
	/* 0x0C */ int reqHeight;
	/* 0x10 */ int reqMode;  /* thImageDecoder::ImageMode, default -1 */
	/* 0x14 */ int priority;  /* of the load thread, default 100 */
	/* 0x18 */ int unk_18;  /* not initialized by the constructor */
	/* 0x1C */ int flags;  /* bit 0 = keep the raw pixels instead of a texture */
	/* 0x20 */ int decState;  /* 1 = decoding done, 2 = failed; consumed by imgDecSync */
	/* 0x24 */ int trimState;  /* 1 = trim done; consumed by imgTrimSync */
	/* 0x28 */ int texID;
	/* 0x2C */ int texPixelFormat;
	/* 0x30 */ int texWidth;
	/* 0x34 */ int texHeight;
	/* 0x38 */ int texOrgWidth;
	/* 0x3C */ int texOrgHeight;
	/* 0x40 */ void *pixels;  /* decoded pixels (malloc) */
	/* 0x44 */ void *clut;  /* decoded palette (CLUT) */
	/* 0x48 */ int pixelFormat;
	/* 0x4C */ int width;
	/* 0x50 */ int height;
	/* 0x54 */ int orgWidth;
	/* 0x58 */ int orgHeight;
	/* 0x5C */ vector<int> keepTex;  /* textures kept by imgKeepTex */

	/* 0x68 */ PLTM_String pltm;  /* PLTM string to render */
	/* 0x98 */ int hasPltm;  /* 1 = pltm constructed, to be destroyed */
	/* 0x9C */ int pltmW;  /* default 64 */
	/* 0xA0 */ int pltmH;  /* default 64 */

	static int instance_count;

	ImgData()
	        : thread(0), fetchJob(0), reqWidth(0), reqHeight(0), reqMode(-1), priority(100), flags(0), decState(0),
	          trimState(0), texID(0), texPixelFormat(-1), texWidth(-1), texHeight(-1), texOrgWidth(-1),
	          texOrgHeight(-1), pixels(0), clut(0), pixelFormat(-1), width(-1), height(-1), orgWidth(-1),
	          orgHeight(-1), hasPltm(0), pltmW(64), pltmH(64)
	{
		++instance_count;
	}

	void ReleaseThread();
	void ReleasePLTMString();

	~ImgData();
}; /* size = 0xA4 */

int ImgData::instance_count = 0;

inline ImgData::~ImgData()
{
	unsigned int i;

	--instance_count;
	ReleaseThread();
	if (texID != 0)
		tmRelease(texID);
	for (i = 0; i < keepTex.size(); i++)
		tmRelease(keepTex[i]);
	if (pixels != 0)
		free(pixels);
	if (clut != 0)
		free(clut);
	ReleasePLTMString();
}

inline void ImgData::ReleaseThread()
{
	if (fetchJob && fetchJob->m_tid)
		pthread_kill(fetchJob->m_tid, 16); /* SIGUSR1 */
	if (fetchJob) {
		fetchJob->abort();
		fetchJob->release();
		fetchJob = 0;
	}
	if (thread) {
		thread->abort();
		thread->release();
		thread = 0;
	}
}

inline void ImgData::ReleasePLTMString()
{
	if (hasPltm) {
		PLTM_DestroyString(&pltm);
		hasPltm = 0;
	}
}

/* The script that runs amx, user data 'PSCR' of the AMX. */
static inline Script *AmxScript(AMX *amx)
{
	Script *s;

	amx_GetUserData(amx, TAG_PSCR, (void **)&s);
	return s;
}

/* The image state of a script, user data '_IMG'. */
static inline ImgData *ScriptImgData(Script *owner)
{
	return (ImgData *)owner->GetUserData(TAG_IMG);
}

static inline ImgData *AmxImage(AMX *amx)
{
	return ScriptImgData(AmxScript(amx));
}

/* The first n characters of s. */
static inline StringUtil Prefix(const string &s, unsigned int n)
{
	return s.substr(0, n);
}

/* What is left of s after its first n characters. */
static inline StringUtil Suffix(const string &s, unsigned int n)
{
	return s.substr(n, s.length() - n);
}

/* Case-insensitive compare of pre with pat. */
static inline int SchemeCmp(const StringUtil &pre, StringUtil pat)
{
	StringUtil head(pre);
	head.MakeUpper();
	pat.MakeUpper();
	return head.compare(pat);
}

#include "script/scenewindow.h"

/* The last n characters of s. */
static inline StringUtil Right(const StringUtil &s, unsigned int n)
{
	return s.substr(s.length() - n, n);
}

/* Job callbacks, defined after the natives. */
static int ImgPublishDec(AMX *amx, int fmt, void *pixels, void *clut, int w, int h, int ow, int oh);
static int ImgPublishTex(AMX *amx, int fmt, void *pixels, void *clut, int w, int h, int ow, int oh);
static void ImgFetchUrl(thJob *job);
static bool ImgFetchDone(thJob *job, void *user);
static void Tim2LoadFile(thJob *t);
static int ImgPublishTim2(thJob *t, AMX *amx);
static void Tim2LoadFree(thJob *t);
static void ImgDecStringJobIdle(thJob *job);
static bool ImgDecStringJobRun(thJob *job, void *user);
static void ImgExtension_AddQueue(thJob *task, int order);

static void ImgDelete(AMX *amx);

/* Delete callback of the user data: destroys the ImgData of the script. */
static void ImgDelete(AMX *amx)
{
	ImgData *img;

	img = AmxImage(amx);
	if (img != 0)
		delete img;
}

/*
 * imgDecStart(url [, w, h, mode, prio, flags]), also imgLoadTim2Start: starts
 * decoding an image for the script. http: and https: URLs go through the
 * download queue (fetch job); local files are loaded directly, .tm2/.bt2 as
 * Tim2 and anything else by a thImageDecoder.
 */
static cell AMX_NATIVE_CALL n_imgDecStart(AMX *amx, cell *params)
{
	string url = CellToString(amx, params[1]);
	Script *sa;
	ImgData *img;

	GetSceneWindow(amx)->MakeFullPath(url);
	amx_GetUserData(amx, TAG_PSCR, (void **)&sa);
	img = ScriptImgData(sa);
	if (img != 0) {
		if (img->fetchJob != 0) {
			if (img->fetchJob->m_tid != 0)
				pthread_kill(img->fetchJob->m_tid, 16);
			if (img->fetchJob != 0) {
				img->fetchJob->abort();
				img->fetchJob->release();
				img->fetchJob = 0;
			}
		}
		if (img->thread != 0) {
			img->thread->abort();
			img->thread->release();
			img->thread = 0;
		}
	} else {
		img = new ImgData;
		AmxScript(amx)->SetUserData(TAG_IMG, img);
		AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_IMG, ImgDelete);
	}
	if (params[0] / sizeof(cell) == 6) {
		img->reqWidth = params[2];
		img->reqHeight = params[3];
		img->reqMode = params[4];
		img->priority = params[5];
		img->flags = params[6];
	}
	if (url.length() == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}

	StringUtil u = url;
	if (SchemeCmp(Prefix(u, 5), "http:") == 0 || SchemeCmp(Prefix(u, 6), "https:") == 0) {
		SceneWindow *sw = GetSceneWindow(amx);

		img->fetchJob = new thJob(amx, ImgFetchUrl, ImgFetchDone, 0);
		img->fetchJob->m_name = "imgDecStart";
		img->fetchJob->m_str[0] = url;
		img->fetchJob->m_int[IMG_FETCH_CACHE_ONLY] = sw->UsingImageList_Check(url);
		ImgExtension_AddQueue(img->fetchJob, img->priority);
	} else {
		bool tim2 = SchemeCmp(Right(u, 4), ".tm2") == 0 || SchemeCmp(Right(u, 4), ".bt2") == 0;
		if (tim2) {
			thJob *j =
			    new thJob(amx, Tim2LoadFile, (bool (*)(thJob *, void *))ImgPublishTim2, Tim2LoadFree);
			j->m_name = "imgDecStart";
			j->m_str[0] = url;
			img->thread = j;
			j->addThreadIdleList(1, 10000);
			img->decState = 0;
		} else {
			unsigned int opt;
			thImageDecoder *d = new thImageDecoder(amx, (DecodeFinishCallback)ImgPublishDec);
			if (d != 0) {
				d->setFileName(url);
				if (img->reqWidth > 0 && img->reqHeight > 0)
					d->setRequestSize(img->reqWidth, img->reqHeight);
				d->setRequestImageMode((thImageDecoder::ImageMode)img->reqMode);
				opt = img->flags & 2;
				if (img->flags & 4)
					opt |= 0x80000000;
				d->setDecodeOption(opt);
				img->thread = d;
				d->addThreadIdleList(0, img->priority);
				img->decState = 0;
			}
		}
	}
	return 0;
}

/*
 * imgTrimStart(x, y, w, h): cuts a rectangle out of the decoded pixels and
 * publishes it as a texture. Returns 0 for a pixel format other than PSMCT32,
 * PSMCT24, PSMCT16, PSMT8 and PSMT4.
 */
static cell AMX_NATIVE_CALL n_imgTrimStart(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);
	int x, y, w, h;
	int xoff, stride, rowbytes;
	char *buf;
	int i;

	if (img == 0) {
err:
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (img->pixels == 0)
		goto err;
	x = params[1];
	y = params[2];
	w = params[3];
	h = params[4];
	if (w <= 0)
		goto err;
	if (h <= 0)
		goto err;
	if (x < 0)
		goto err;
	if (y < 0)
		goto err;
	if (img->width < x + w)
		goto err;
	if (img->height < y + h)
		goto err;

	switch (img->pixelFormat) {
	case 0:
		xoff = x * 4;
		stride = img->width * 4;
		rowbytes = w * 4;
		break;
	case 1:
		xoff = x * 3;
		stride = img->width * 3;
		rowbytes = w * 3;
		break;
	case 2:
		xoff = x * 2;
		stride = img->width * 2;
		rowbytes = w * 2;
		break;
	case 19:
	case 20:
		xoff = x;
		stride = img->width;
		rowbytes = w;
		break;
	default:
		return 0;
	}

	if (x == 0 && w == img->width) {
		ImgPublishTex(amx, img->pixelFormat, (char *)img->pixels + y * stride, img->clut, w, h, w, h);
		return 0;
	}

	buf = (char *)malloc(h * rowbytes);
	for (i = 0; i < params[4]; i++)
		memcpy(buf + i * rowbytes, (char *)img->pixels + (y + i) * stride + xoff, rowbytes);
	ImgPublishTex(amx, img->pixelFormat, buf, img->clut, w, h, w, h);
	free(buf);
	return 0;
}

/*
 * imgDecStringStart(str, opt, w, h, flags): prepares the PLTM string and
 * starts the job that renders it into a texture. An existing ImgData is
 * cleaned up first; otherwise a new one is attached to the Script.
 */
static cell AMX_NATIVE_CALL n_imgDecStringStart(AMX *amx, cell *params)
{
	string str = CellToString(amx, params[1]);
	Script *sa;
	ImgData *img;
	thJob *job;

	amx_GetUserData(amx, TAG_PSCR, (void **)&sa);
	img = ScriptImgData(sa);
	if (img != 0) {
		if (img->fetchJob != 0) {
			if (img->fetchJob->m_tid != 0)
				pthread_kill(img->fetchJob->m_tid, 16);
			if (img->fetchJob != 0) {
				img->fetchJob->abort();
				img->fetchJob->release();
				img->fetchJob = 0;
			}
		}
		if (img->thread != 0) {
			img->thread->abort();
			img->thread->release();
			img->thread = 0;
		}
		if (img->hasPltm != 0) {
			PLTM_DestroyString(&img->pltm);
			img->hasPltm = 0;
		}
	} else {
		img = new ImgData;
		AmxScript(amx)->SetUserData(TAG_IMG, img);
		AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_IMG, ImgDelete);
	}
	PLTM_CreateStringFromUtf8(params[2], &img->pltm, str.c_str());
	img->hasPltm = 1;
	img->pltmW = params[3];
	img->pltmH = params[4];
	img->reqWidth = params[5];

	job = new thJob(amx, ImgDecStringJobIdle, ImgDecStringJobRun, 0);
	job->m_name = "imgDecStringStart";
	img->thread = job;
	job->start();
	img->decState = 0;
	return 0;
}

/*
 * imgDecSync(), also imgLoadTim2Sync: 0 while decoding, 1 when done, -1 on
 * failure or without an image. The state is consumed by the call.
 */
static cell AMX_NATIVE_CALL n_imgDecSync(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);
	int st;

	if (img == 0)
		return -1;
	if (img->fetchJob != 0 || img->thread != 0)
		return 0;
	st = img->decState;
	if (st != 1) {
		img->decState = 0;
		return -1;
	}
	img->decState = 0;
	return 1;
}

/* imgTrimSync(): 1 when the trim is done (consumed by the call), else -1. */
static cell AMX_NATIVE_CALL n_imgTrimSync(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);
	int st;

	if (img == 0)
		return -1;
	st = img->trimState;
	if (st != 1) {
		img->trimState = 0;
		return -1;
	}
	img->trimState = 0;
	return 1;
}

/* Accessors of the image: AMX_ERR_NATIVE and 0 when there is no image. */
static cell AMX_NATIVE_CALL n_imgWidth(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return img->width;
}

static cell AMX_NATIVE_CALL n_imgHeight(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return img->height;
}

static cell AMX_NATIVE_CALL n_imgOrgWidth(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return img->orgWidth;
}

static cell AMX_NATIVE_CALL n_imgOrgHeight(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return img->orgHeight;
}

static cell AMX_NATIVE_CALL n_imgPixelFormat(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return img->pixelFormat;
}

static cell AMX_NATIVE_CALL n_imgTexWidth(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return img->texWidth;
}

static cell AMX_NATIVE_CALL n_imgTexHeight(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return img->texHeight;
}

static cell AMX_NATIVE_CALL n_imgTexOrgWidth(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return img->texOrgWidth;
}

static cell AMX_NATIVE_CALL n_imgTexOrgHeight(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return img->texOrgHeight;
}

static cell AMX_NATIVE_CALL n_imgTexPixelFormat(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return img->texPixelFormat;
}

/* imgTexID(): the texture handle, xor-ed with HANDLE_XOR. */
static cell AMX_NATIVE_CALL n_imgTexID(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);
	int id;

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	id = img->texID;
	if (id == 0)
		return 0;
	return id ^ HANDLE_XOR;
}

/* imgKeepTex(handle): keeps a reference to the texture (tmDuplicate); 0 is a no-op. */
static cell AMX_NATIVE_CALL n_imgKeepTex(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);
	int id;

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (params[1] != 0) {
		id = params[1] ^ HANDLE_XOR;
		img->keepTex.push_back(id);
		tmDuplicate(id);
	}
	return 0;
}

/* imgReleaseTex(handle): releases a texture kept by imgKeepTex. */
static cell AMX_NATIVE_CALL n_imgReleaseTex(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);
	int id;

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (params[1] != 0) {
		id = params[1] ^ HANDLE_XOR;
		for (vector<int>::iterator i = img->keepTex.begin(); i != img->keepTex.end(); ++i) {
			if (*i == id) {
				tmRelease(id);
				img->keepTex.erase(i);
				return 0;
			}
		}
	}
	return 0;
}

/* imgGetResource(id): xor-ed handle of a resource texture; needs no image. */
static cell AMX_NATIVE_CALL n_imgGetResource(AMX *amx, cell *params)
{
	int id = GetResource(params[1]);

	if (id == 0)
		return 0;
	return id ^ HANDLE_XOR;
}

/*
 * imgGetPixels(x, y, w, h, buf): copies a rectangle of the decoded pixels to
 * the script buffer; returns 0 for an unsupported pixel format.
 */
static cell AMX_NATIVE_CALL n_imgGetPixels(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);
	cell *dest;
	unsigned char *buf;
	int x, y, w, h;
	int xb, pitch, rowb;
	unsigned int size;
	unsigned int need;
	int i;

	if (img == 0)
		goto err;
	if (img->pixels == 0)
		goto err;
	x = params[1];
	y = params[2];
	w = params[3];
	h = params[4];
	if (w <= 0)
		goto err;
	if (h <= 0)
		goto err;
	if (x < 0)
		goto err;
	if (y < 0)
		goto err;
	if (img->width < x + w)
		goto err;
	if (img->height < y + h)
		goto err;
	switch (img->pixelFormat) {
	case 0:   /* PS2_GS_PSMCT32 */
		xb = x * 4;
		pitch = img->width * 4;
		rowb = w * 4;
		break;

	case 1:   /* PS2_GS_PSMCT24 */
		xb = x * 3;
		pitch = img->width * 3;
		rowb = w * 3;
		break;

	case 2:   /* PS2_GS_PSMCT16 */
		xb = x * 2;
		pitch = img->width * 2;
		rowb = w * 2;
		break;

	case 19:  /* PS2_GS_PSMT8 */
	case 20:  /* PS2_GS_PSMT4 */
		xb = x;
		pitch = img->width;
		rowb = w;
		break;

	default:
		return 0;
	}
	size = h * rowb;
	buf = (unsigned char *)malloc(size);
	if (x == 0 && w == img->width) {
		memcpy(buf, (unsigned char *)img->pixels + y * pitch, size);
	} else {
		for (i = 0; i < params[4]; i++)
			memcpy(buf + i * rowb, (unsigned char *)img->pixels + (y + i) * pitch + xb, rowb);
	}
	need = h * rowb;
	if ((unsigned int)(params[6] * 4) < need)
		goto err;
	amx_GetAddr(amx, params[5], &dest);
	memcpy(dest, buf, need);
	free(buf);
	return 0;
err:
	amx_RaiseError(amx, 10);
	return 0;
}

/* imgGetPalettes(buf): copies the decoded palette (CLUT) to the script buffer. */
static cell AMX_NATIVE_CALL n_imgGetPalettes(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);
	cell *cptr;
	int size;

	if (img == 0)
		goto err;
	if (img->clut == 0)
		goto err;
	size = Texture::CalcSizeofClut(img->pixelFormat);
	if ((unsigned int)(params[2] * 4) < (unsigned int)size)
		goto err;
	amx_GetAddr(amx, params[1], &cptr);
	memcpy(cptr, img->clut, size);
	return 0;
err:
	amx_RaiseError(amx, 10);
	return 0;
}

/*
 * imgCreateTex(w, h, fmt, pixels, clut): creates the image texture from two
 * script arrays and stores its parameters in the tex* fields.
 */
static cell AMX_NATIVE_CALL n_imgCreateTex(AMX *amx, cell *params)
{
	ImgData *img = AmxImage(amx);
	cell *pixels;
	cell *clut;
	int w, h, fmt;
	int mode;
	int id;

	if (img == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	w = params[1];
	h = params[2];
	fmt = params[3];
	amx_GetAddr(amx, params[4], &pixels);
	amx_GetAddr(amx, params[5], &clut);
	if (pixels == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (fmt == 20 && clut == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (img->texID != 0) {
		tmRelease(img->texID);
		img->texID = 0;
	}
	mode = (AmxScript(amx)->flags >> 1) & 1;
	id = Texture::Create(w, h, fmt, 0, 1, pixels, clut, true, mode);
	img->texID = id;
	if (id != 0) {
		img->texPixelFormat = fmt;
		img->texWidth = w;
		img->texHeight = h;
		img->texOrgWidth = w;
		img->texOrgHeight = h;
		img->decState = 1;
	} else {
		img->texPixelFormat = -1;
		img->texWidth = -1;
		img->texHeight = -1;
		img->texOrgWidth = -1;
		img->texOrgHeight = -1;
		img->decState = 2;
	}
	return 0;
}

AMX_NATIVE_INFO smallext_img[] = {
	{ "imgDecStart", n_imgDecStart },
	{ "imgTrimStart", n_imgTrimStart },
	{ "imgDecSync", n_imgDecSync },
	{ "imgTrimSync", n_imgTrimSync },
	{ "imgDecStringStart", n_imgDecStringStart },
	{ "imgWidth", n_imgWidth },
	{ "imgHeight", n_imgHeight },
	{ "imgOrgWidth", n_imgOrgWidth },
	{ "imgOrgHeight", n_imgOrgHeight },
	{ "imgPixelFormat", n_imgPixelFormat },
	{ "imgTexWidth", n_imgTexWidth },
	{ "imgTexHeight", n_imgTexHeight },
	{ "imgTexOrgWidth", n_imgTexOrgWidth },
	{ "imgTexOrgHeight", n_imgTexOrgHeight },
	{ "imgTexPixelFormat", n_imgTexPixelFormat },
	{ "imgTexID", n_imgTexID },
	{ "imgKeepTex", n_imgKeepTex },
	{ "imgReleaseTex", n_imgReleaseTex },
	{ "imgGetPixels", n_imgGetPixels },
	{ "imgGetPalettes", n_imgGetPalettes },
	{ "imgCreateTex", n_imgCreateTex },
	{ "imgLoadTim2Start", n_imgDecStart },
	{ "imgLoadTim2Sync", n_imgDecSync },
	{ "imgGetResource", n_imgGetResource },
	{ 0, 0 },
};

/*
 * Decoder callback: with bit 0 of flags keeps copies of the raw pixels and
 * palette, otherwise creates the texture. Sets decState and stops the decoder
 * thread. Returns 1.
 */
static int ImgPublishDec(AMX *amx, int fmt, void *pixels, void *clut, int w, int h, int ow, int oh)
{
	ImgData *img = AmxImage(amx);
	int id;
	int mode;

	if (img->flags & 1) {
		if (pixels != 0) {
			if (img->pixels != 0) {
				free(img->pixels);
				img->pixels = 0;
			}
			if (img->clut != 0) {
				free(img->clut);
				img->clut = 0;
			}
			img->pixels = malloc(Texture::CalcSizeofImage(fmt, w, h, 1));
			memcpy(img->pixels, pixels, Texture::CalcSizeofImage(fmt, w, h, 1));
			if (clut != 0) {
				img->clut = malloc(Texture::CalcSizeofClut(fmt));
				memcpy(img->clut, clut, Texture::CalcSizeofClut(fmt));
			}
			img->pixelFormat = fmt;
			img->width = w;
			img->height = h;
			img->orgWidth = ow;
			img->orgHeight = oh;
			img->decState = 1;
			goto done;
		}
		if (img->pixels != 0) {
			free(img->pixels);
			img->pixels = 0;
		}
		if (img->clut != 0) {
			free(img->clut);
			img->clut = 0;
		}
		img->pixelFormat = -1;
		img->width = -1;
		img->height = -1;
		img->orgWidth = -1;
		img->orgHeight = -1;
		img->decState = 2;
		goto done;
	}
	if (pixels != 0) {
		if (img->texID != 0) {
			tmRelease(img->texID);
			img->texID = 0;
		}
		mode = (AmxScript(amx)->flags >> 1) & 1;
		id = Texture::Create(w, h, fmt, 0, 1, pixels, clut, true, mode);
		img->texID = id;
		if (id != 0) {
			img->texPixelFormat = fmt;
			img->texWidth = w;
			img->texHeight = h;
			img->texOrgWidth = ow;
			img->texOrgHeight = oh;
			img->decState = 1;
			goto done;
		}
	} else {
		if (img->texID != 0) {
			tmRelease(img->texID);
			img->texID = 0;
		}
	}
	img->texPixelFormat = -1;
	img->texWidth = -1;
	img->texHeight = -1;
	img->texOrgWidth = -1;
	img->texOrgHeight = -1;
	img->decState = 2;
done:
	if (img->thread != 0) {
		img->thread->abort();
		img->thread->release();
		img->thread = 0;
	}
	return 1;
}

/* Publishes the result of a trim as the image texture and sets trimState. */
static int ImgPublishTex(AMX *amx, int fmt, void *pixels, void *clut, int w, int h, int ow, int oh)
{
	ImgData *img = AmxImage(amx);
	int id;
	int mode;

	if (pixels != 0) {
		if (img->texID != 0) {
			tmRelease(img->texID);
			img->texID = 0;
		}
		mode = (AmxScript(amx)->flags >> 1) & 1;
		id = Texture::Create(w, h, fmt, 0, 1, pixels, clut, true, mode);
		img->texID = id;
		if (id != 0) {
			img->texPixelFormat = fmt;
			img->texWidth = w;
			img->texHeight = h;
			img->texOrgWidth = ow;
			img->texOrgHeight = oh;
			img->trimState = 1;
			goto done;
		}
	} else {
		if (img->texID != 0) {
			tmRelease(img->texID);
			img->texID = 0;
		}
	}
	img->texPixelFormat = -1;
	img->texWidth = -1;
	img->texHeight = -1;
	img->texOrgWidth = -1;
	img->texOrgHeight = -1;
	img->trimState = 2;
done:
	return 1;
}

/*
 * First callback of the fetch job (worker thread): resolves job->m_str[0] to a
 * local path (HTTP download, "file:" or plain path). The path is set only for
 * status 200; the status goes to job->m_int[IMG_FETCH_STATUS].
 */
static void ImgFetchUrl(thJob *job)
{
	HttpFile hf;
	sceHttpFileRequestArg req;
	int cookie;
	int ret;

	memset(&req, 0, sizeof(req));
	req.done_flag = &cookie;
	req.timeout = 30000;
	string path;
	{
		string u = job->m_str[0];
		if (SchemeCmp(Prefix(u, 5), "http:") == 0 || SchemeCmp(Prefix(u, 6), "https:") == 0) {
			if (job->m_int[IMG_FETCH_CACHE_ONLY])
				req.mode = 2;
			ret = hf.Request(job->m_str[0], 0, 0, &req);
			if (cookie)
				hf.save_cookie();
			path = (const char *)hf;
		} else if (SchemeCmp(Prefix(u, 5), "file:") == 0) {
			ret = 200;
			const string &rest = Suffix(u, 5);
			path = rest;
		} else {
			ret = 200;
			path = job->m_str[0];
		}
	}
	if (ret == 200) {
		job->m_str[1] = path;
		job->m_int[IMG_FETCH_STATUS] = ret;
	} else {
		job->m_str[1] = "";
		job->m_int[IMG_FETCH_STATUS] = ret;
	}
}

/*
 * Second callback of the fetch job (main thread): on status 200 starts the
 * actual load, a Tim2 job for .tm2/.bt2 and a thImageDecoder for anything
 * else; otherwise sets decState = 2.
 */
static bool ImgFetchDone(thJob *job, void *user)
{
	AMX *amx = (AMX *)user;
	ImgData *img = AmxImage(amx);
	string path = job->m_str[1];
	int ret = job->m_int[IMG_FETCH_STATUS];

	if (img->fetchJob != 0) {
		img->fetchJob->abort();
		img->fetchJob->release();
		img->fetchJob = 0;
	}
	if (ret == 200) {
		GetSceneWindow(amx)->UsingImageList_Add(job->m_str[0]);
		if (img->fetchJob != 0) {
			if (img->fetchJob->m_tid != 0)
				pthread_kill(img->fetchJob->m_tid, 16);
			if (img->fetchJob != 0) {
				img->fetchJob->abort();
				img->fetchJob->release();
				img->fetchJob = 0;
			}
		}
		if (img->thread != 0) {
			img->thread->abort();
			img->thread->release();
			img->thread = 0;
		}
		bool tim2 = SchemeCmp(Right(path, 4), ".tm2") == 0 || SchemeCmp(Right(path, 4), ".bt2") == 0;
		if (tim2) {
			thJob *j =
			    new thJob(amx, Tim2LoadFile, (bool (*)(thJob *, void *))ImgPublishTim2, Tim2LoadFree);
			j->m_name = "cbDownloadFinished";
			j->m_str[0] = path;
			img->thread = j;
			j->addThreadIdleList(1, 10000);
			img->decState = 0;
		} else {
			unsigned int opt;
			thImageDecoder *d = new thImageDecoder(amx, (DecodeFinishCallback)ImgPublishDec);
			if (d != 0) {
				d->setFileName(path);
				if (img->reqWidth > 0 && img->reqHeight > 0)
					d->setRequestSize(img->reqWidth, img->reqHeight);
				d->setRequestImageMode((thImageDecoder::ImageMode)img->reqMode);
				opt = img->flags & 2;
				if (img->flags & 4)
					opt |= 0x80000000;
				d->setDecodeOption(opt);
				img->thread = d;
				d->addThreadIdleList(0, img->priority);
				img->decState = 0;
			}
		}
	} else if (img->flags & 1) {
		if (img->pixels != 0) {
			free(img->pixels);
			img->pixels = 0;
		}
		if (img->clut != 0) {
			free(img->clut);
			img->clut = 0;
		}
		img->pixelFormat = -1;
		img->width = -1;
		img->height = -1;
		img->orgWidth = -1;
		img->orgHeight = -1;
		img->decState = 2;
	} else {
		if (img->texID != 0) {
			tmRelease(img->texID);
			img->texID = 0;
		}
		img->texPixelFormat = -1;
		img->texWidth = -1;
		img->texHeight = -1;
		img->texOrgWidth = -1;
		img->texOrgHeight = -1;
		img->decState = 2;
	}
	return true;
}

/* Reads the whole file of t->m_str[0] ("file:" prefix allowed) into a 16-byte aligned buffer. */
static void Tim2LoadFile(thJob *t)
{
	string path = t->m_str[0];
	int fd;
	int size;
	void *buf;

	{
		string url = path;

		if (SchemeCmp(Prefix(url, 5), "file:") == 0) {
			const string &rest = Suffix(url, 5);
			path = rest;
		}
	}
	fd = BBopenCPP(path.c_str(), 0);
	if (fd < 0)
		return;
	size = BBlseekCPP(fd, 0, 2);
	BBlseekCPP(fd, 0, 0);
	if (size <= 0)  /* NOTE: bug: the file stays open on these error paths */
		return;
	buf = memalign(16, size);
	if (buf == 0)
		return;
	if (BBreadCPP(fd, buf, size) < size) {
		BBcloseCPP(fd);
		free(buf);
		return;
	}
	BBcloseCPP(fd);
	t->m_ptr[0] = buf;
	t->m_int[IMG_TIM2_SIZE] = size;
}

/* Publishes a texture built from the Tim2 file in t (data at +84, size at +68). */
static int ImgPublishTim2(thJob *t, AMX *amx)
{
	void *data = t->m_ptr[0];
	unsigned int size = t->m_int[IMG_TIM2_SIZE];
	ImgData *img = AmxImage(amx);
	int id = 0;

	if (t->m_ptr[0] != 0) {
		id = Tim2_GetTexID(data, size, (AmxScript(amx)->flags >> 1) & 1);
	}
	if (id != 0) {
		if (img->texID != 0) {
			tmRelease(img->texID);
			img->texID = 0;
		}
		img->texID = id;
		img->texPixelFormat = tmGetPixelFormat(id);
		img->texWidth = tmGetWidth(id);
		img->texHeight = tmGetHeight(id);
		img->texOrgWidth = tmGetWidth(id);
		img->texOrgHeight = tmGetHeight(id);
		img->decState = 1;
	} else {
		if (img->texID != 0) {
			tmRelease(img->texID);
			img->texID = 0;
		}
		img->texPixelFormat = -1;
		img->texWidth = -1;
		img->texHeight = -1;
		img->texOrgWidth = -1;
		img->texOrgHeight = -1;
		img->decState = 2;
	}
	if (img->thread != 0) {
		img->thread->abort();
		img->thread->release();
		img->thread = 0;
	}
	return 1;
}

/* Third callback of the Tim2 job: frees the file read. */
static void Tim2LoadFree(thJob *t)
{
	if (t->m_ptr[0] != 0)
		free(t->m_ptr[0]);
}

/* Idle callback of the imgDecStringStart job: nothing to do. */
static void ImgDecStringJobIdle(thJob *job)
{}

/*
 * Body of the imgDecStringStart job: renders the PLTM string into a texture.
 * user is the AMX. Returns false only on error -101 (retried on the next
 * call); other errors from -99 to -1 are printed.
 */
static bool ImgDecStringJobRun(thJob *job, void *user)
{
	AMX *amx = (AMX *)user;
	ImgData *img = AmxImage(amx);
	int tex = 0;
	int id;
	int r;

	r = PLTM_CreateImageString(Font::Get(0), img->pltmW, img->pltmH, img->reqWidth << 6, &img->pltm,
	    Texture::Manager(), 0, &tex);
	if (r == -101)
		return false;
	if (r >= 0) {
		if (img->texID != 0) {
			tmRelease(img->texID);
			img->texID = 0;
		}
		if (Texture::RegistStringTexture(tex, (AmxScript(amx)->flags >> 1) & 1) == 0) {
			tmRelease(tex);
			tex = 0;
		} else {
			id = tex;
			img->texID = id;
			if (id != 0) {
				img->texPixelFormat = tmGetPixelFormat(tex);
				img->texWidth = tmGetWidth(tex);
				img->texHeight = tmGetHeight(tex);
				img->texOrgWidth = tmGetWidth(tex);
				img->texOrgHeight = tmGetHeight(tex);
			} else {
				img->texPixelFormat = -1;
				img->texWidth = -1;
				img->texHeight = -1;
				img->texOrgWidth = -1;
				img->texOrgHeight = -1;
			}
			img->decState = 1;
			goto done;
		}
	}
	if (r >= -99 && r < 0)
		Texture::PrintTMError(r);
	if (img->texID != 0) {
		tmRelease(img->texID);
		img->texID = 0;
	}
	img->texPixelFormat = -1;
	img->texWidth = -1;
	img->texHeight = -1;
	img->texOrgWidth = -1;
	img->texOrgHeight = -1;
	img->decState = 2;
done:
	if (img->hasPltm != 0) {
		PLTM_DestroyString(&img->pltm);
		img->hasPltm = 0;
	}
	if (img->thread != 0) {
		img->thread->abort();
		img->thread->release();
		img->thread = 0;
	}
	return true;
}

/*
 * Download queue of the image fetch jobs: at most three run at the same time,
 * the others wait in s_downloadQueue sorted by order.
 */

/* Entry of the download queue. */
struct DLQueItem {
	/* 0x00 */ thJob *task;
	/* 0x04 */ int order;  /* sort key */
	DLQueItem() {}
	DLQueItem(thJob *t, int o) : task(t), order(o) {}
	~DLQueItem() {}  /* redundant as written */
};

static bool operator<(const DLQueItem &a, const DLQueItem &b)
{
	return a.order < b.order;
}

static vector<DLQueItem> s_downloadQueue;
static thJob *s_downloading[3];

void ImgExtension_Init()
{
	int i;

	s_downloadQueue.erase(s_downloadQueue.begin(), s_downloadQueue.end());
	for (i = 0; i < 3; i++)
		s_downloading[i] = 0;
}

void ImgExtension_Exit()
{
	int i;

	s_downloadQueue.erase(s_downloadQueue.begin(), s_downloadQueue.end());
	for (i = 0; i < 3; i++)
		s_downloading[i] = 0;
}

void ImgExtension_Dump()
{
	unsigned int i;

	for (i = 0; i < s_downloadQueue.size(); i++) {
		cout << "download_queue[" << i << "] = " << s_downloadQueue[i].task->m_str[0]
		     << ", state = " << s_downloadQueue[i].task->m_status << endl;
	}
	for (i = 0; i < 3; i++) {
		if (s_downloading[i]) {
			cout << "downloading[" << i << "] = " << s_downloading[i]->m_str[0]
			     << ", state = " << s_downloading[i]->m_status << endl;
		}
	}
}

/*
 * Drives the queue: frees the slots of finished jobs, drops released
 * jobs from the head of the queue and starts queued jobs in the free slots.
 */
void ImgExtension_Run()
{
	int i;
	int j;

	for (i = 0; i < 3; i++) {
		if (s_downloading[i]) {
			if (s_downloading[i]->IsCallFinished() || s_downloading[i]->IsEnd()) {
				s_downloading[i]->release();
				s_downloading[i] = 0;
			} else if (s_downloading[i]->IsRelease()) {
				s_downloading[i] = 0;
			}
		}
	}

	while (s_downloadQueue.size() != 0 && s_downloadQueue.begin()->task->IsRelease())
		s_downloadQueue.erase(s_downloadQueue.begin());

	for (j = 0; j < 3; j++) {
		if (s_downloadQueue.size() != 0 && s_downloading[j] == 0) {
			s_downloading[j] = s_downloadQueue.begin()->task;
			s_downloadQueue.erase(s_downloadQueue.begin());
			s_downloading[j]->start();
		}
	}
}

/* Adds a job to the download queue, kept sorted by order. */
static void ImgExtension_AddQueue(thJob *task, int order)
{
	s_downloadQueue.push_back(DLQueItem(task, order));
	sort(s_downloadQueue.begin(), s_downloadQueue.end());
}
