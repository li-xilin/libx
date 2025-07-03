#include "x/tss.h"
#include "x/errno.h"
#include "x/detect.h"
#include "x/list.h"
#include "x/memory.h"
#ifdef X_OS_WIN
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

#ifdef X_OS_WIN
#define TLS_MAXIIUM_AVAILABLE 1088

typedef struct {
	x_link Link;
	DWORD key;
	x_tss_free_f *pfnFreeProc;
} TLS_NODE, *PTLS_NODE;

static struct _TLS_CONTEXT {
	x_list TlsList;
	CRITICAL_SECTION Lock;
} *s_TlsContext = NULL;

static void TryInitTlsContext(void)
{
	if (s_TlsContext)
		return;
	struct _TLS_CONTEXT *ctx = x_malloc(NULL, sizeof *ctx);
	InitializeCriticalSection(&ctx->Lock);
	x_list_init(&ctx->TlsList);
	if (InterlockedCompareExchangePointer((void *volatile *)&s_TlsContext, ctx, NULL))
		x_free(ctx);
}

int x_tss_init(x_tss *key, x_tss_free_f *free_cb)
{
	int rc = -1;
	assert(key);
	TryInitTlsContext();

	EnterCriticalSection(&s_TlsContext->Lock);
	PTLS_NODE pNode = x_malloc(NULL, sizeof *pNode);

	DWORD k = TlsAlloc();
	x_assert (k < TLS_OUT_OF_INDEXES, "the number of Tls entries limit to %d", TLS_MAXIIUM_AVAILABLE);
	key->key = k;
	pNode->key = k;
	pNode->pfnFreeProc = free_cb;
	x_list_add_back(&s_TlsContext->TlsList, &pNode->Link);

	rc = 0;
	LeaveCriticalSection(&s_TlsContext->Lock);
	return rc;
}

void x_tss_remove(x_tss *key)
{
	assert(key != NULL);
	assert(s_TlsContext != NULL);
	EnterCriticalSection(&s_TlsContext->Lock);
	TlsFree(key->key);
	x_link *cur;
	x_list_foreach(cur, &s_TlsContext->TlsList) {
		PTLS_NODE pNode = x_container_of(cur, TLS_NODE, Link);
		if (pNode->key == key->key) {
			x_list_del(cur);
			x_free(pNode);
			goto out;
		}
	}
out:
	LeaveCriticalSection(&s_TlsContext->Lock);
}

void __x_tss_free_all_win32(void)
{
	if (!s_TlsContext)
		return;
	x_tss_free_f *aFuncList[1024];
	LPVOID aArgList[1024];
	DWORD dwCount = 0;
	EnterCriticalSection(&s_TlsContext->Lock);
	x_link *cur;
	x_list_foreach(cur, &s_TlsContext->TlsList) {
		PTLS_NODE pNode = x_container_of(cur, TLS_NODE, Link);
		LPVOID pSlotValue = TlsGetValue(pNode->key);
		if (!pNode->pfnFreeProc)
			continue;
		aFuncList[dwCount] = pNode->pfnFreeProc;
		aArgList[dwCount] = pSlotValue;
		dwCount++;
	}
	LeaveCriticalSection(&s_TlsContext->Lock);
	for (int i = 0; i < dwCount; i++)
		aFuncList[i](aArgList[i]);
}

void *x_tss_get(x_tss *key)
{
	assert(key != NULL);
	void *val;;
	if (!(val = TlsGetValue(key->key)))
		x_eval_errno();
	return val;
}

int x_tss_set(x_tss *key, void *value)
{
	assert(key != NULL);
	if (!TlsSetValue(key->key, value)) {
		x_eval_errno();
		return -1;
	}
	return 0;
}

#else

int x_tss_init(x_tss *key, x_tss_free_f *free_cb)
{
	assert(key != NULL);
	return pthread_key_create(&key->key, free_cb);
}

void x_tss_remove(x_tss *key)
{
	assert(key != NULL);
	(void)pthread_key_delete(key->key);
}

void *x_tss_get(x_tss *key)
{
	assert(key != NULL);
	return pthread_getspecific(key->key);
}

int x_tss_set(x_tss *key, void *value)
{
	assert(key != NULL);
	int err = pthread_setspecific(key->key, value);
	if (err) {
		errno = err;
		return -1;
	}
	return 0;
}

#endif

