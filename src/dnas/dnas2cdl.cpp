/*
 * sceDNAS2CDL*: the DNAS2 content download queue and its download thread,
 * which fetches the entry at the head of the queue over HTTP.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "http/httpfile.h"

#include "dnas2cdl.h"
#include "thread/scesema.h"
#include "install/scetryinstdl.h"

extern "C" {

typedef struct DNAS2QueueEntry DNAS2QueueEntry;

struct DNAS2QueueEntry {
	DNAS2QueueEntry *next;
	DNAS2QueueEntry *prev;
	DNAS2DLTag info;
};

/* "downloader active" flag, cleared by sceDNAS2CDLStop and by the thread */
static int g_dnas2Ready = 0;
static DNAS2QueueEntry *g_cdlQueueHead = 0;

static int DNAS2CDLQueueAlloc(DNAS2DLTag *tag, DNAS2QueueEntry **out);
static int DNAS2CDLQueueFree(DNAS2QueueEntry *p);

/* Download methods, indexed by the first field of the DLTAG (entry 1 = the
 * installer, sceTRYInstDL*). write_cb and done_cb go into the HTTP request. */
typedef struct {
	/* 0x00 */ int (*error)(void);
	/* 0x04 */ int (*first_check)(DNAS2DLTag *tag, int z, DNAS2CDLWork *w);
	/* 0x08 */ int (*range_check)(DNAS2DLTag *tag, DNAS2CDLWork *w, int z);
	/* 0x0C */ int write_cb;
	/* 0x10 */ int (*done_cb)(int);
	/* 0x14 */ int (*complete)(DNAS2DLTag *tag, int z);
	/* 0x18 */ int (*close)(int fd);
	/* 0x1C */ int (*fail)(int fd, DNAS2DLTag *tag, int z);
} DNAS2CDLMethods;

static int DNAS2CDLErrno(void);

/* clang-format off */
static DNAS2CDLMethods g_cdlMethods[2] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { DNAS2CDLErrno, sceTRYInstDLFirstCheck, sceTRYInstDLRangeCheck,
      (int)sceTRYInstDLWrite, sceTRYInstDLRewind, sceTRYInstDLComplete,
      sceTRYInstDLClose, sceTRYInstDLFail },
};
/* clang-format on */

/* Download request built by sceDNAS2CDLStart (20 bytes, malloc'ed). */
typedef struct {
	/* 0x00 */ char *name;  /* HTTP user */
	/* 0x04 */ char *name2;  /* HTTP password */
	/* 0x08 */ int unused8;
	/* 0x0C */ DNAS2DLTag *tag;  /* DLTAG at the head of the queue */
	/* 0x10 */ int *cookie;  /* cookies are saved if > 0 */
} DNAS2CDLReq;

/* 0 if the Content-Type header is the expected one (or none is expected), else -25006. */
static int DNAS2CDLCheckContentType(sceHTTPClient *h, sceHttpFileRequestArg *req)
{
	sceHTTPResponse *resp;
	sceHTTPHeaderList *hdr;

	int r;

	resp = sceHTTPGetResponse(h);
	r = 0;
	if (req->ct_buf != 0) {
		hdr = resp->headers;
		while (hdr != 0) {
			if (strcmp("Content-Type", hdr->name) == 0) {
				r = strcmp(req->ct_buf, hdr->value) != 0 ? -25006 : 0;
				goto out;
			}
			hdr = hdr != 0 ? hdr->next : 0;
		}
		r = -25006;
	}
out:
	return r;
}

/* Downloader state: thread and semaphores, then the 40-byte public status
 * that sceDNAS2CDLGetStatus copies (DNAS2CDLStatus). */
typedef struct {
	/* 0x00 */ pthread_t thread_id;
	/* 0x04 */ int sema[4];
	/* 0x14 */ int pad20;
	/* 0x18 */ int status;  /* result reported by the thread */
	/* 0x1C */ int active;
	/* 0x20 */ DNAS2CDLStatus st;
} DNAS2CDLThreadCtx;

static DNAS2CDLThreadCtx g_cdlThread;

/* Progress callback of the request: records the Content-Length of the reply
 * only once (-1 = not yet); the position `v` is always stored. */
static void DNAS2CDLNoteContentLength(sceHTTPClient *h, long long v, sceHttpFileRequestArg *arg)
{
	if (g_cdlThread.st.clen == -1)
		g_cdlThread.st.clen = sceHTTPGetContentLength(h);
	g_cdlThread.st.pos = v;
}

/* Starts a detached POSIX thread with the given stack size; every failing
 * step prints perror and destroys the attributes. `name` and the last
 * argument are not used. */
static int DNAS2StartThread(pthread_t *th, void *(*fn)(void *), void *name, int stacksize, int prio, void *arg)
{
	unsigned char attr[48];
	int r;

	r = pthread_attr_init((pthread_attr_t *)attr);
	if (r < 0) {
		perror("pthread_attr_init():");
		return r;
	}
	if (stacksize != 0) {
		r = pthread_attr_setstacksize((pthread_attr_t *)attr, stacksize);
		if (r < 0) {
			perror("pthread_attr_setstacksize():");
			goto destroy;
		}
	}
	r = pthread_create(th, (pthread_attr_t *)attr, fn, arg);
	if (r < 0) {
		perror("pthread_create:");
		goto destroy;
	}
	r = pthread_detach(*th);
	if (r == 0)
		return 0;
	perror("pthread_detach:");
destroy:
	pthread_attr_destroy((pthread_attr_t *)attr);
	return r;
}

/* strdup. */
static char *DNAS2CDLStrdup(char *s)
{
	char *p;

	p = (char *)malloc(strlen(s) + 1);
	if (p == 0)
		return 0;
	strncpy(p, s, strlen(s) + 1);
	return p;
}

/* Frees a queue entry and the strings of its DLTAG. */
static int DNAS2CDLQueueFree(DNAS2QueueEntry *p)
{
	if (p->info.id != 0)
		free(p->info.id);
	if (p->info.ver != 0)
		free(p->info.ver);
	if (p->info.title != 0)
		free(p->info.title);
	if (p->info.title_id != 0)
		free(p->info.title_id);
	if (p->info.title_sub_id != 0)
		free(p->info.title_sub_id);
	if (p->info.do_id != 0)
		free(p->info.do_id);
	if (p->info.nextref != 0)
		free(p->info.nextref);
	if (p->info.cref != 0)
		free(p->info.cref);
	if (p->info.dist_id != 0)
		free(p->info.dist_id);
	if (p->info.unk_4C != 0)
		free(p->info.unk_4C);
	if (p->info.cpage != 0)
		free(p->info.cpage);
	if (p->info.kref != 0)
		free(p->info.kref);
	if (p->info.kpage != 0)
		free(p->info.kpage);
	if (p->info.ackref != 0)
		free(p->info.ackref);
	if (p->info.pat_name != 0)
		free(p->info.pat_name);
	if (p->info.pat_size != 0)
		free(p->info.pat_size);
	if (p->info.thumbnail_image != 0)
		free(p->info.thumbnail_image);
	if (p->info.application_name != 0)
		free(p->info.application_name);
	free(p);
	return 0;
}

/* A DLTAG is valid if it is a CONTENT tag (type 0) with the NOKEY flag (1)
 * and a non-empty URL (`desc`). */
static int DNAS2DLTagValidate(DNAS2DLTag *tag)
{
	if (tag->tag_type != 0) {
		fprintf(stderr, "tag type must be sceDNAS2_DLTAG_TYPE_CONTENT only.\n");
		return -25012;
	}
	if (tag->tag_flag == 1) {
		if (!tag->cref || strlen(tag->cref) == 0)
			return -25012;
	} else {
		fprintf(stderr, "tag_flag must be sceDNAS2_DLTAG_FLAG_NOKEY only.\n");
		return -25012;
	}
	return 0;
}

static char *DNAS2CDLStrdup(char *s);

/* Validates the DLTAG and makes a deep copy of it in a new queue entry. */
static int DNAS2CDLQueueAlloc(DNAS2DLTag *tag, DNAS2QueueEntry **out)
{
	DNAS2QueueEntry *e;
	int r;

	r = DNAS2DLTagValidate(tag);
	if (r < 0)
		return r;
	e = (DNAS2QueueEntry *)malloc(128);
	if (e == 0)
		return -25004;
	memset(e, 0, 128);
	e->info.tag_flag = tag->tag_flag;
	e->info.tag_type = tag->tag_type;
	e->info.dldata_size = tag->dldata_size;
	e->info.inst_size = tag->inst_size;
	e->info.inst = tag->inst;
	e->info.dist_no = tag->dist_no;
	e->info.unk_50 = tag->unk_50;
	e->info.jump_flag = tag->jump_flag;
	if (tag->id != 0) {
		e->info.id = DNAS2CDLStrdup(tag->id);
		if (e->info.id == 0)
			goto fail;
	}
	if (tag->ver != 0) {
		e->info.ver = DNAS2CDLStrdup(tag->ver);
		if (e->info.ver == 0)
			goto fail;
	}
	if (tag->title != 0) {
		e->info.title = DNAS2CDLStrdup(tag->title);
		if (e->info.title == 0)
			goto fail;
	}
	if (tag->title_id != 0) {
		e->info.title_id = DNAS2CDLStrdup(tag->title_id);
		if (e->info.title_id == 0)
			goto fail;
	}
	if (tag->title_sub_id != 0) {
		e->info.title_sub_id = DNAS2CDLStrdup(tag->title_sub_id);
		if (e->info.title_sub_id == 0)
			goto fail;
	}
	if (tag->do_id != 0) {
		e->info.do_id = DNAS2CDLStrdup(tag->do_id);
		if (e->info.do_id == 0)
			goto fail;
	}
	if (tag->nextref != 0) {
		e->info.nextref = DNAS2CDLStrdup(tag->nextref);
		if (e->info.nextref == 0)
			goto fail;
	}
	if (tag->cref != 0) {
		e->info.cref = DNAS2CDLStrdup(tag->cref);
		if (e->info.cref == 0)
			goto fail;
	}
	if (tag->dist_id != 0) {
		e->info.dist_id = DNAS2CDLStrdup(tag->dist_id);
		if (e->info.dist_id == 0)
			goto fail;
	}
	if (tag->unk_4C != 0) {
		e->info.unk_4C = DNAS2CDLStrdup(tag->unk_4C);
		if (e->info.unk_4C == 0)
			goto fail;
	}
	if (tag->cpage != 0) {
		e->info.cpage = DNAS2CDLStrdup(tag->cpage);
		if (e->info.cpage == 0)
			goto fail;
	}
	if (tag->kref != 0) {
		e->info.kref = DNAS2CDLStrdup(tag->kref);
		if (e->info.kref == 0)
			goto fail;
	}
	if (tag->kpage != 0) {
		e->info.kpage = DNAS2CDLStrdup(tag->kpage);
		if (e->info.kpage == 0)
			goto fail;
	}
	if (tag->ackref != 0) {
		e->info.ackref = DNAS2CDLStrdup(tag->ackref);
		if (e->info.ackref == 0)
			goto fail;
	}
	if (tag->pat_name != 0) {
		e->info.pat_name = DNAS2CDLStrdup(tag->pat_name);
		if (e->info.pat_name == 0)
			goto fail;
	}
	if (tag->pat_size != 0) {
		e->info.pat_size = DNAS2CDLStrdup(tag->pat_size);
		if (e->info.pat_size == 0)
			goto fail;
	}
	if (tag->thumbnail_image != 0) {
		e->info.thumbnail_image = DNAS2CDLStrdup(tag->thumbnail_image);
		if (e->info.thumbnail_image == 0)
			goto fail;
	}
	if (tag->application_name != 0) {
		e->info.application_name = DNAS2CDLStrdup(tag->application_name);
		if (e->info.application_name == 0)
			goto fail;
	}
	*out = e;
	return 0;
fail:
	DNAS2CDLQueueFree(e);
	return -25004;
}

int sceDNAS2CDLQueueInsertAt(DNAS2DLTag *tag, int pos)
{
	DNAS2QueueEntry *newNode;
	DNAS2QueueEntry *pred;
	int r;
	int count;

	pred = 0;
	r = DNAS2CDLQueueAlloc(tag, &newNode);
	if (r < 0)
		return r;

	if (pos < 0)
		return -25003;

	count = sceDNAS2CDLQueueGetTotalEntryNum();
	if (count + 1 < pos)
		return -25003;

	if (pos == 0) {
		newNode->next = g_cdlQueueHead;
		newNode->prev = 0;
		if (g_cdlQueueHead != 0)
			g_cdlQueueHead->prev = newNode;
		g_cdlQueueHead = newNode;
	} else {
		DNAS2QueueEntry *p = g_cdlQueueHead;
		int i;

		for (i = 0; i < pos; i++) {
			pred = p;
			p = p->next;
		}

		if (pred != 0)
			newNode->next = pred->next;
		newNode->prev = pred;
		pred->next = newNode;
		if (newNode->next != 0)
			newNode->next->prev = newNode;
	}

	return 0;
}

int sceDNAS2CDLQueueRemove(int index)
{
	DNAS2QueueEntry *p;
	int count;
	int i;
	int r;

	if (index < 0)
		return -25003;

	count = sceDNAS2CDLQueueGetTotalEntryNum();
	if (count <= index)
		return -25003;

	p = g_cdlQueueHead;
	if (index > 0) {
		i = index;
		do {
			p = p->next;
		} while (--i);
	}

	if (p->prev != 0)
		p->prev->next = p->next;
	else
		g_cdlQueueHead = p->next;

	if (p->next != 0)
		p->next->prev = p->prev;

	r = DNAS2CDLQueueFree(p);
	if (r >= 0)
		return 0;
	return r;
}

int sceDNAS2CDLQueueGetTotalEntryNum(void)
{
	int n = 0;
	DNAS2QueueEntry *p;

	for (p = g_cdlQueueHead; p != 0; p = p->next)
		n++;

	return n;
}

/* NOTE: idx is ignored: the entry returned is always the head of the queue. */
int sceDNAS2CDLQueueGetEntryInfo(DNAS2DLTag *out, int idx)
{
	*out = g_cdlQueueHead->info;
	return 0;
}

/* Installer error code as a DNAS2 error (base 26000). */
static int DNAS2CDLErrno(void)
{
	return try_instdl_errno - 26000;
}

static HttpFile *g_cdlObj;

/* One HTTP request of the download: clears the public status, prepares the
 * destination with the method of the DLTAG type, fills the request (expected
 * Content-Type, Content-Length callback, resume position, write and done
 * callbacks) and calls HttpFile::Request on the DLTAG URL; then confirms
 * (200) or cancels. */
static int DNAS2CDLRequest(DNAS2CDLReq *req, DNAS2CDLWork *w)
{
	sceHttpFileRequestArg arg;
	char ctype[128];
	int type;
	int fd;
	int r;
	int t;
	int got;

	type = req->tag->tag_flag;
	memset(&g_cdlThread.st, 0, sizeof(g_cdlThread.st));
	g_cdlThread.st.clen = -1;
	g_cdlThread.st.state = 1;
	r = (*g_cdlMethods[type].first_check)(req->tag, 0, w);
	if (r < 0) {
		r = r - 26000;
		goto out;
	}
	fd = r;
	memset(&arg, 0, sizeof(arg));
	strcpy(ctype, "binary/pscontent");
	arg.no_accept = 1;
	if (*req->cookie > 0)
		arg.done_flag = &got;
	arg.unk_30 = 0;
	arg.progress_cb = (int)DNAS2CDLNoteContentLength;
	arg.ct_buf = ctype;
	arg.ct_len = 128;
	arg.user = req->name;
	arg.fd = fd;
	arg.pass = req->name2;
	arg.resume_pos = w->pos;
	arg.write_cb = g_cdlMethods[type].write_cb;
	arg.done_cb = (void (*)(int))g_cdlMethods[type].done_cb;
	r = g_cdlObj->Request(req->tag->cref, 0, 0, &arg);
	if (r == 200) {
		t = (*g_cdlMethods[type].close)(fd);
		r = t;
		r = r - 26000;
		if (t < 0)
			goto out;
		r = 0;
		if (*req->cookie > 0 && got > 0)
			g_cdlObj->save_cookie();
	} else {
		(*g_cdlMethods[type].fail)(fd, req->tag, 0);
		switch (r) {
		case SCE_HTTP_ERR_WRITE:
			r = (*g_cdlMethods[type].error)();
			break;
		case SCE_HTTP_ERR_ABORTED:
			if (arg.unk_20 < 0)
				r = arg.unk_20;
			break;
		}
	}
out:
	return r;
}

/* Opens the stream, pumps it until the buffer counter becomes positive, then
 * closes it; errors of the two methods become -26000 + r. */
static int DNAS2CDLPump(DNAS2CDLReq *ctx)
{
	DNAS2CDLWork w;
	int type;
	int r;
	int t;
	int i;

	type = ctx->tag->tag_flag;
	r = (*g_cdlMethods[type].range_check)(ctx->tag, &w, 0);
	if (r < 0) {
		r = r - 26000;
		goto out;
	}
	memset(&w, 0, 24);
	i = w.n;
	while (i <= 0) {
		r = DNAS2CDLRequest(ctx, &w);
		if (r != 0)
			goto out;
		i++;
		w.n = i;
	}
	t = (*g_cdlMethods[type].complete)(ctx->tag, 0);
	r = t;
	r = r - 26000;
	r = (t >= 0) ? 0 : r;
out:
	return r;
}

static void *DNAS2CDLThread(void *arg);

static inline int DNAS2CDLCreateSema(int init)
{
	SemaParam sp;

	memset(&sp, 0, sizeof(sp));
	sp.initCount = init;
	sp.maxCount = 1;
	return sce_sema_createsema(&sp);
}

/* Download thread: runs the request and reports the result. */
static void *DNAS2CDLThread(void *arg)
{
	DNAS2CDLReq *req = (DNAS2CDLReq *)arg;
	int r;
	int t;

	if (g_cdlObj == 0)
		g_cdlObj = new HttpFile;
	sce_sema_signalsema(g_cdlThread.sema[0]);
	if (req->tag->tag_type != 0)
		r = 5;
	else {
		t = DNAS2CDLPump(req);
		if (t == 0)
			r = 5;
		else if (t == SCE_HTTP_ERR_ABORTED)
			r = 6;
		else
			r = t;
	}
	g_dnas2Ready = 0;
	if (req->name != 0)
		free(req->name);
	if (req->name2 != 0)
		free(req->name2);
	free(req);
	g_cdlThread.st.state = r;
	return 0;
}

/* Starts the download thread: creates the four semaphores (the second with
 * count 1), starts the thread with the request and waits on the first
 * semaphore for its result; on error it kills the thread and deletes the
 * semaphores. */
static int DNAS2CDLSubmit(DNAS2CDLReq *req)
{
	int r;

	if (g_dnas2Ready > 0)
		return -25000;
	memset(&g_cdlThread, 0, sizeof(g_cdlThread));
	g_dnas2Ready = 1;

	if ((g_cdlThread.sema[0] = DNAS2CDLCreateSema(0)) < 0)
		goto fail;
	if ((g_cdlThread.sema[1] = DNAS2CDLCreateSema(1)) < 0)
		goto fail;
	if ((g_cdlThread.sema[2] = DNAS2CDLCreateSema(0)) < 0)
		goto fail;
	if ((g_cdlThread.sema[3] = DNAS2CDLCreateSema(0)) < 0)
		goto fail;

	if (DNAS2StartThread(&g_cdlThread.thread_id, DNAS2CDLThread, 0, 0, 0, req) < 0) {
fail:
		r = -25010;
	} else {
		sce_sema_waitsema(g_cdlThread.sema[0]);
		if (g_cdlThread.status >= 0)
			return 0;
		r = g_cdlThread.status;
	}
	if (g_cdlThread.thread_id != 0)
		pthread_cancel(g_cdlThread.thread_id);
	if (g_cdlThread.sema[0] > 0)
		sce_sema_deletesema(g_cdlThread.sema[0]);
	if (g_cdlThread.sema[1] > 0)
		sce_sema_deletesema(g_cdlThread.sema[1]);
	if (g_cdlThread.sema[2] > 0)
		sce_sema_deletesema(g_cdlThread.sema[2]);
	if (g_cdlThread.sema[3] > 0)
		sce_sema_deletesema(g_cdlThread.sema[3]);
	return r;
}

static int DNAS2CDLStopWorker(void)
{
	return 1;
}

/* Stops the downloader: cancels the thread, deletes its semaphores and the HttpFile. */
int sceDNAS2CDLStop(void)
{
	if (!g_dnas2Ready)
		return 0;

	if (g_cdlThread.active > 0)
		return 0;

	if (DNAS2CDLStopWorker() == 0)
		return -25000;

	if (g_cdlThread.thread_id != 0)
		pthread_cancel(g_cdlThread.thread_id);

	sce_sema_deletesema(g_cdlThread.sema[0]);
	sce_sema_deletesema(g_cdlThread.sema[1]);
	sce_sema_deletesema(g_cdlThread.sema[2]);
	sce_sema_deletesema(g_cdlThread.sema[3]);

	if (g_cdlObj != 0) {
		delete g_cdlObj;
		g_cdlObj = 0;
	}

	g_dnas2Ready = 0;

	return 0;
}

/* DLTAG of the first queue entry.
 * NOTE: bug: no check for an empty queue, so the caller's null test never fires. */
static DNAS2DLTag *DNAS2CDLQueueHeadInfo(void)
{
	return &g_cdlQueueHead->info;
}

/* Starts the download of the entry at the head of the queue.
 * NOTE: bug: if malloc fails, the error path dereferences the null request. */
int sceDNAS2CDLStart(char *name, char *name2, int *cookie)
{
	DNAS2CDLReq *p;
	DNAS2DLTag *tag;
	int r;

	tag = DNAS2CDLQueueHeadInfo();
	if (tag == 0)
		return -25002;

	p = (DNAS2CDLReq *)malloc(20);

	if (p == 0 || (memset(p, 0, 20), (name != 0 && (p->name = DNAS2CDLStrdup(name)) == 0))
	    || (name2 != 0 && (p->name2 = DNAS2CDLStrdup(name2)) == 0)) {
		r = -25004;
	} else {
		p->tag = tag;
		p->cookie = cookie;

		r = DNAS2CDLSubmit(p);
		if (r >= 0)
			return 0;
	}

	if (p->name != 0)
		free(p->name);

	if (p->name2 != 0)
		free(p->name2);

	if (p != 0)
		free(p);

	sceDNAS2CDLStop();

	return r;
}

/* Kills the download thread if a download is in progress. */
int sceDNAS2CDLAbort(void)
{
	if (g_cdlThread.st.state == 1)
		pthread_kill(g_cdlThread.thread_id, SIGUSR1);

	return 0;
}

int sceDNAS2CDLGetThreadID(void)
{
	return g_cdlThread.thread_id;
}

int sceDNAS2CDLGetStatus(DNAS2CDLStatus *out)
{
	*out = g_cdlThread.st;
	return 0;
}

} /* extern "C" */
