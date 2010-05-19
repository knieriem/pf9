#include <u.h>
#include <mingw32.h>
#include <libc.h>
#include <thread.h>

#include "util.h"

typedef
struct Commap {
	HANDLE h;
	int	fd;
} Commap;

static
void
rxhelper(void*v)
{
	Commap *m;
	uchar buf[4096];
	DWORD	n;
	int	fd;

	m = v;
	fd = m->fd;
	for (;;) {
//		fprint(2, "Rx");
		if (!ReadFile(m->h, buf, sizeof buf, &n, NULL))
			switch(GetLastError()) {
			case ERROR_BROKEN_PIPE:
			case ERROR_PIPE_NOT_CONNECTED:
				goto out;
			}
		if (fd!=-1)
			if (write(fd, buf, n) != n)
				break;
	}
out:
//	fprint(2, "[%d]\tRexit", getpid());
	if (fd!=-1)
		close(fd);
	CloseHandle(m->h);
	free(v);
}
static
void
txhelper(void*v)
{
	Commap *m;
	uchar buf[4096];
	DWORD	nw;
	int	n;
	int	fd;

	m = v;
	fd = m->fd;
	for (;;) {
//		fprint(2, "Tx");
		if (fd==-1)
			n = 0;
		else
			n = read(m->fd, buf, sizeof buf);
		if (n<=0)
			break;
		if (!WriteFile(m->h, buf, n, &nw, NULL))
			break;
		if (n != nw)
			break;
	}
//	fprint(2, "[%d]\tTexit", getpid());
	if (fd!=-1)
		close(fd);
	CloseHandle(m->h);
	free(v);
}

void
wincreaterxtxproc(int fd, int mode, HANDLE h)
{
	Commap *m;

	m = malloc(sizeof(Commap));
	if(m == nil)
		sysfatal("crxtx oom");
	m->fd = fd;
	m->h = h;
	proccreate(mode==OWRITE? rxhelper: txhelper, m, 32768);
}
