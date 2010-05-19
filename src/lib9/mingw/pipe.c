#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#define NOPLAN9DEFINES
#include <libc.h>

#include "fdtab.h"
#include "util.h"

static
int
alloc(HANDLE h, char *name, int type)
{
	int	fd;
	Fd	*f;

	fd = fdtalloc(nil);
	f = fdtab[fd];
	f->h = h;
	f->type = type;
	f->name = strdup(name);
	if (fdtdebug>1)
		fprint(2, "%\f\n", fd, f, "create");
	return fd;
}

int
p9pipe(int fd[2])
{
	HANDLE	h0, h1;
	char	name[40];

	wincreatepipename(name, sizeof name);

	if(wincreatenamedpipe(&h1, name, ORDWR, 1) == -1)
		return -1;

	h0 = wincreatefile(name,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		OPEN_EXISTING,
		FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OVERLAPPED);
	if (h0==INVALID_HANDLE_VALUE) {
		winerror("CreateFile");
		return -1;
	}

	if(winconnectpipe(h1, 1) == -1){
		CloseHandle(h0);
		return -1;
	}

	fd[0] = alloc(h0, name, Fdtypepipecl);
	fd[1] = alloc(h1, name, Fdtypepipesrv);

	return 0;
}


static int npipes;
static QLock lk;
char*
wincreatepipename(char buf[], int sz)
{
	int	n;

	qlock(&lk);
	++npipes;
	n = npipes;
	qunlock(&lk);

	snprint(buf, sz, "//./pipe/p9pipe-%08x-%08x", GetCurrentProcessId(), n);
	return buf;
}
