#include <windows.h>

#include "threadimpl.h"

#undef exits
#undef _exits

static CRITICAL_SECTION initmutex;

#define mutexptr(lk)	((CRITICAL_SECTION*)&(lk)->mutex)

static void
lockinit(Lock *lk)
{
	EnterCriticalSection(&initmutex);
	if(lk->init == 0){
		InitializeCriticalSection(mutexptr(lk));
		lk->init = 1;
	}
	LeaveCriticalSection(&initmutex);
}

int
_threadlock(Lock *lk, int block, ulong pc)
{
	if(!lk->init)
		lockinit(lk);
	if(block){
		EnterCriticalSection(mutexptr(lk));
		return 1;
	}else{
		if (TryEnterCriticalSection(mutexptr(lk)))
			return 1;
		return 0;
	}
}

void
_threadunlock(Lock *lk, ulong pc)
{
	LeaveCriticalSection(mutexptr(lk));
}

void
_procsleep(_Procrendez *r)
{
	/* r is protected by r->l, which we hold */
	r->cond = CreateSemaphore(NULL, 0, 1, NULL);
	if(r->cond == NULL)
		sysfatal("CreateSemaphore");
	r->asleep = 1;
//	print("sleep: %p\n", r->cond);
	unlock(r->l);
	WaitForSingleObject(r->cond, INFINITE);
	lock(r->l);
//	print("wake %p\n", r->cond);
	CloseHandle(r->cond);
	r->asleep = 0;
}

void
_procwakeup(_Procrendez *r)
{
	if(r->asleep){
//		print("wakeup: %p\n", r->cond); 
		r->asleep = 0;
		ReleaseSemaphore(r->cond, 1, NULL);
	}
}

void
_procwakeupandunlock(_Procrendez *r)
{
	if(r->asleep){
		r->asleep = 0;
//		print("wakeupu: %p\n", r->cond); 
		ReleaseSemaphore(r->cond, 1, NULL);
	}
	unlock(r->l);
}

static DWORD WINAPI
startprocfn(LPVOID v)
{
	void **a;
	void (*fn)(void*);
	Proc *p;

	a = (void**)v;
	fn = (void(*)(void*))a[0];
	p = a[1];
	free(a);
	p->osprocid = GetCurrentThreadId();

	(*fn)(p);

	ExitThread(0);
}

void
_procstart(Proc *p, void (*fn)(Proc*))
{
	HANDLE h;
	void **a;

	a = malloc(2*sizeof a[0]);
	if(a == nil)
		sysfatal("_procstart malloc: %r");
	a[0] = (void*)fn;
	a[1] = p;
	h = CreateThread(NULL, 8192, startprocfn, a, 0, NULL);

	if(h==NULL){
		fprint(2, "pthread_create: %r\n");
		abort();
	}
	CloseHandle(h);
}

static ulong tlsproc = ~0;

Proc*
_threadproc(void)
{
	if(tlsproc == ~0)
		return nil;
	return TlsGetValue(tlsproc);
}

void
_threadsetproc(Proc *p)
{
	TlsSetValue(tlsproc, p);
}

void
_pthreadinit(void)
{
	InitializeCriticalSection(&initmutex);
	tlsproc = TlsAlloc();
}

void
threadexitsall(char *msg)
{
	exits(msg);
}

void
_threadpexit(void)
{
	assert(0==1);
	/* seems to be unused */
}

int
getmcontext(mcontext_t *mcp)
{
	mcp->ContextFlags = CONTEXT_FULL;
	if (GetThreadContext(GetCurrentThread(), mcp))
		return 0;
	return -1;
}

void
setmcontext(mcontext_t *mcp)
{
	SetThreadContext(GetCurrentThread(), mcp);
}

void
makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
	int *sp;

	sp = (int*)ucp->uc_stack.ss_sp+ucp->uc_stack.ss_size/4;
	sp -= argc;
	sp = (void*)((uintptr_t)sp - (uintptr_t)sp%16);	/* 16-align for OS X */
	memmove(sp, &argc+1, argc*sizeof(int));

	*--sp = 0;		/* return address */
	ucp->uc_mcontext.Eip = (ulong)func;
	ucp->uc_mcontext.Esp = (ulong)sp;
	ucp->uc_mcontext.ContextFlags = CONTEXT_FULL;
}

int
swapcontext(ucontext_t *oucp, ucontext_t *ucp)
{
	if(getcontext(oucp) == 0)
		setcontext(ucp);
	return 0;
}