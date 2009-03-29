#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#include <libc.h>

#include "util.h"


/*
 * some stuff stolen from Inferno's utils/rcsh/Nt.c, some from 9pm
 */

enum {
	Nchild	= 64,
	NOHANG	= 1,
};

typedef
struct Child {
	int	pid;
	HANDLE	handle;
} Child;

static Child child[Nchild];

static QLock lk;

int
winaddchild(int pid, HANDLE handle)
{
	int i;

//	fprint(2, "addchi: %d %p\n", pid, handle);
	for(i=0; i<Nchild; i++) {
		if(child[i].handle == 0) {
			qlock(&lk);
			child[i].handle = handle;
			child[i].pid = pid;
			qunlock(&lk);
			return 0;
		}
	}
	werrstr("child table full");
	return -1;
}


static
ulong
ft2ms(FILETIME *ft)
{
	uvlong t;

	t = ((uvlong)ft->dwLowDateTime) + (((uvlong)ft->dwHighDateTime)<<32);
	return t/10000;
}

typedef
struct Times
{
	ulong	u, s, elap;
} Times;
static
void
gettimes(HANDLE h, Times *t)
{
	FILETIME	ftc, ftex, ftu, fts;

	if (!GetProcessTimes(h, &ftc, &ftex, &fts, &ftu))
		memset(t, 0, sizeof(Times));
	else {
		t->elap = ft2ms(&ftex) - ft2ms(&ftc);
		t->s = ft2ms(&fts);
		t->u = ft2ms(&ftu);
	}
}

static int
wait3(DWORD *status, int nohang, Times *t)
{
	HANDLE h, hwait[Nchild];
	DWORD ret;
	int pid;
	int i, n;

	pid = -1;
	n = 0;
	qlock(&lk);
	for(i = 0; i < Nchild; i++)
		if (child[i].handle != nil)
			hwait[n++] = child[i].handle;
	qunlock(&lk);

	if (n==0)
		return -1;

	assert(Nchild<=MAXIMUM_WAIT_OBJECTS);
	ret = WaitForMultipleObjects(n, hwait, 0, nohang? 0: INFINITE);
	switch (ret) {
	case WAIT_TIMEOUT:
		if (nohang)
			return 0;
	case WAIT_FAILED:
		winerror(nil);
		return -1;
	}
	if (ret < WAIT_OBJECT_0 && ret >= WAIT_OBJECT_0+n)
		werrstr("unexpected WFMO result: %d", ret);

	h = hwait[ret-WAIT_OBJECT_0];

	for(i = 0; i < Nchild; i++){
		if(child[i].handle == h){
			qlock(&lk);
			pid = child[i].pid;
			child[i].pid = 0;
			child[i].handle = 0;
			qunlock(&lk);
			break;
		}
	}

	gettimes(h, t);
	if(!GetExitCodeProcess(h, status)) {
		winerror(nil);
		*status = 1;
	}
	CloseHandle(h);

	return pid;
}

static int
wait4(uint pid, DWORD *status, int nohang, Times *t)
{
	HANDLE h;
	DWORD ret;
	int i;
	
	if(pid == 0)
		return -1;

	h = 0;
	for(i = 0; i < Nchild; i++){
		if(child[i].pid == pid){
			qlock(&lk);
			h = child[i].handle;
			child[i].pid = 0;
			child[i].handle = 0;
			qunlock(&lk);
			break;
		}
	}

	if(h == 0){	/* we don't know about this one - let the system try to find it */
		fprint(2, "oops %d\n", pid);
		h = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
		if(h == 0)
			return -1;		/* can't find it */
	}

	ret = WaitForSingleObject(h, nohang? 0: INFINITE);
	switch (ret) {
	case WAIT_TIMEOUT:
		if (nohang)
			return 0;
	case WAIT_FAILED:
		winerror(nil);
		return -1;
	}

	gettimes(h, t);
	if(!GetExitCodeProcess(h, status)) {
		winerror(nil);
		*status = 1;
	}
	CloseHandle(h);

	return pid;
}

static int
_await(int pid4, char *str, int n, int nohang)
{
	DWORD status;
	int pid;
	char buf[128];
	Times t;

	for(;;){
		if(pid4 == -1)
			pid = wait3(&status, nohang, &t);
		else
			pid = wait4(pid4, &status, nohang, &t);
		if(pid <= 0)
			return -1;
		if(status)
			snprint(buf, sizeof buf, "%d %lud %lud %lud %d", pid, t.u, t.s, t.elap, status);
		else
			snprint(buf, sizeof buf, "%d %lud %lud %lud ''", pid, t.u, t.s, t.elap, status);
		strecpy(str, str+n, buf);
		return strlen(str);
	}
}

int
await(char *str, int n)
{
	return _await(-1, str, n, 0);
}

int
awaitnohang(char *str, int n)
{
	return _await(-1, str, n, NOHANG);
}

int
awaitfor(int pid, char *str, int n)
{
	return _await(pid, str, n, 0);
}
