#include <u.h>
#include <mingw32.h>
#include <libc.h>
#include <thread.h>

#include "fdtab.h"
#include "util.h"

typedef
struct Arg
{
	int	fd;
	int	mode;
	HANDLE	h;
} Arg;

static
void
cleanup(void *v)
{
	Arg *a;

	a = v;
	CloseHandle(a->h);
	free(a);
}

static
void
exportproc(void *v)
{
	Arg *a;

	a = v;

	if(winconnectpipe(a->h, 0) == -1){
		CloseHandle(a->h);
		return;
	}
	wincreaterxtxproc(a->fd, a->mode, a->h);
}


int
winexportfd(char name[], int namesz, int fd, int mode)
{
	HANDLE h;
	Arg *a;

	wincreatepipename(name, namesz);

	if(wincreatenamedpipe(&h, name, mode==OREAD? OWRITE: OREAD, 1) == -1)
		return -1;

	a = malloc(sizeof(Arg));
	a->fd = fd;
	a->mode = mode;
	a->h = h;
	fdtonclose(fd, cleanup, a);
	proccreate(exportproc, a, 32768);

	return 0;
}
