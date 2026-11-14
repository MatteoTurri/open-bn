/*
 * Texture manager core: initialization, object allocation, reference counting
 * and deferred destruction. Released objects go to a pending list, and tmSync
 * runs their destroy callbacks one call later.
 */

#include <pthread.h>
#include <string.h>

#include "tm_int.h"

/* g_defaultInitInfo is the allocator used when a caller passes none;
   tmInitialize copies the caller's one into it. */
static struct TmObject *g_active_list;
static struct TmInitInfo g_defaultInitInfo;
static struct TmObject *g_pending_list;
static struct TmObject *g_processing_list;

static pthread_mutex_t g_active_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_pending_mutex = PTHREAD_MUTEX_INITIALIZER;

static void _tmProcessDestroyList(struct TmObject *list);

/* Walks a list calling every node's free hook and, if set, its deferred
   destroy callback. */
static void _tmProcessDestroyList(struct TmObject *list)
{
	struct TmObject *node = list;
	struct TmObject *next;

	if (node == 0)
		return;
	do {
		next = node->list_next;
		if (node->always_hook != 0)
			node->always_hook(node);
		if (node->destroy_cb != 0)
			node->destroy_cb(node->destroy_arg);
		node = next;
	} while (node != 0);
}

int tmInitialize(struct TmInitInfo *info)
{
	g_defaultInitInfo = *info;
	g_active_list = 0;
	g_pending_list = 0;
	g_processing_list = 0;
	return 0;
}

void tmShutdown(void)
{
	tmSync();
	tmSync();
	_tmProcessDestroyList(g_active_list);
}

void _tmCopyInitInfo(void *dest)
{
	*(struct TmInitInfo *)dest = g_defaultInitInfo;
}

/* Allocates and zeroes an object with `info` (the default allocator when 0)
   and puts it at the head of the active list. */
void *_tmAlloc(struct TmInitInfo *info, int allocSize, int clearSize)
{
	struct TmObject *obj;

	if (info == 0)
		info = &g_defaultInitInfo;
	obj = (struct TmObject *)info->allocFn(allocSize);
	if (obj == 0)
		return 0;
	memset(obj, 0, clearSize);
	obj->always_hook = info->freeFn;
	pthread_mutex_lock(&g_active_mutex);
	obj->list_next = g_active_list;
	g_active_list = obj;
	pthread_mutex_unlock(&g_active_mutex);
	return obj;
}

TmHandle tmDuplicate(TmHandle h)
{
	struct TmObject *obj = (struct TmObject *)h;

	if (obj != 0)
		obj->ref_count++;
	return h;
}

int tmRelease(TmHandle h)
{
	struct TmObject *obj = (struct TmObject *)h;
	int rc;
	struct TmObject *node, *prev;

	if (h <= 0)
		return -5;
	rc = obj->ref_count - 1;
	obj->ref_count = rc;
	if (rc > 0)
		return rc;

	if (obj->sync_free_cb != 0)
		obj->sync_free_cb((struct TmImage *)obj);

	pthread_mutex_lock(&g_active_mutex);
	prev = 0;
	for (node = g_active_list; node != obj; node = node->list_next)
		prev = node;
	if (prev != 0)
		prev->list_next = obj->list_next;
	else
		g_active_list = obj->list_next;
	pthread_mutex_unlock(&g_active_mutex);

	pthread_mutex_lock(&g_pending_mutex);
	obj->list_next = g_pending_list;
	g_pending_list = obj;
	pthread_mutex_unlock(&g_pending_mutex);

	return rc;
}

/* No handle check. */
void tmSetDestroyCallback(TmHandle h, void (*cb)(void *), void *arg)
{
	struct TmObject *obj = (struct TmObject *)h;

	obj->destroy_cb = cb;
	obj->destroy_arg = arg;
}

void tmSync(void)
{
	struct TmObject *pending;

	_tmProcessDestroyList(g_processing_list);
	pthread_mutex_lock(&g_pending_mutex);
	pending = g_pending_list;
	g_pending_list = 0;
	g_processing_list = pending;
	pthread_mutex_unlock(&g_pending_mutex);
}

int tmCheck(TmHandle h)
{
	struct TmObject *obj = (struct TmObject *)h;
	struct TmObject *node;

	if (h <= 0)
		return -5;
	pthread_mutex_lock(&g_active_mutex);
	node = g_active_list;
	while (node != 0 && node != obj)
		node = node->list_next;
	pthread_mutex_unlock(&g_active_mutex);
	return node ? 0 : -1;
}
