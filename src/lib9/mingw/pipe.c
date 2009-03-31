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
		fprint(2, "%D\n", fd, f, "create");
	return fd;
}
static int npipes;
static QLock lk;
int
p9pipe(int fd[2])
{
	HANDLE	h0, h1;
	OVERLAPPED ov;
	char	name[40];
	int	n;

	qlock(&lk);
	++npipes;
	n = npipes;
	qunlock(&lk);

	snprint(name, sizeof name, "//./pipe/p9pipe-%08x-%08x", GetCurrentProcessId(), n);

	h1 = wincreatenamedpipe(name,
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1,
		4096,	// output buffer size 
		4096,	// input buffer size 
		0);		// client time-out 
	if (h1==INVALID_HANDLE_VALUE) {
		winerror("CreateNamedPipe");
		return -1;
	}

	h0 = wincreatefile(name,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		OPEN_EXISTING,
		FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OVERLAPPED);
	if (h0==INVALID_HANDLE_VALUE) {
		winerror("CreateFile");
		return -1;
	}

	memset(&ov, 0, sizeof ov);
	ov.hEvent = CreateEvent (nil, 1 /* manual reset */, 0 /* initial */, nil);
	if (ov.hEvent==INVALID_HANDLE_VALUE) {
		winerror("");
		return -1;
	}
	switch (winovresult(ConnectNamedPipe(h1, &ov), h1, &ov, nil, 1)) {
	case 0:
		werrstr("pipe: connect is expected to fail");
		return -1;
	case ERROR_PIPE_CONNECTED:
		break;
	default:
		winerror(nil);
		return -1;
	}

	fd[0] = alloc(h0, name, Fdtypepipecl);
	fd[1] = alloc(h1, name, Fdtypepipesrv);

	return 0;
}
