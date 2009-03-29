#include <u.h>
#include <mingw32.h>
#include <libc.h>
#include <dirent.h>

#include "fdtab.h"

Fd **fdtab;
int	fdtabsz;
static int	nfd;

char *typestr[] = {
	[Fdtypenone]	"-",
	[Fdtypesock]	"socket",
	[Fdtypefile]	"file",
	[Fdtypepipecl]	"pipecl",
	[Fdtypepipesrv]	"pipesrv",
	[Fdtypedir]	"dir",
	[Fdtypestd]	"std",
	[Fdtypecons]	"cons",
	[Fdtypedevnull]	"null",
};

int	fdtdebug;
#define	dprint	if(fdtdebug>0)fprint
void
fdtprint(void)
{
	int	i, n;
	Fd	*f;

	dprint(2, "fdtab %s:\n", argv0);
	n = 0;
	for (i=0; i<fdtabsz && n<nfd; i++) {
		f = fdtab[i];
		if (f==nil)
			continue;

		dprint(2, "\t%d:\t%08p %d\t%s\t\"%s\"\n",
			i, f, f->users, typestr[f->type], f->name!=nil?f->name:"");
		++n;
		if (n==nfd)
			break;
	}
}
static
void
releasef(Fd *f, int type)
{
	/* don't call this function in a locked section */
		switch (type) {
		case Fdtypepipesrv:
			FlushFileBuffers(f->h); 
			DisconnectNamedPipe(f->h);
			break;
		case Fdtypefile:
		case Fdtypestd:
		case Fdtypecons:
		case Fdtypepipecl:
			CloseHandle(f->h);
			break;
		case Fdtypesock:
			closesocket(f->s);
			break;
		case Fdtypedir:
			_wclosedir(f->dir);
			break;
		}
		if (f->name!=nil)
			free(f->name);
		if (f->ev.r!=nil)
			CloseHandle(f->ev.r);
		if (f->ev.w!=nil)
			CloseHandle(f->ev.w);
		free(f);
}
static
void
cleanup(void)
{
	int	i, n;
	Fd	*f;

	fdtprint();
	n = 0;
	for (i=0; i<fdtabsz && n<nfd; i++) {
		f = fdtab[i];
		if (f==nil)
			continue;
		--f->users;
		if (f->users==0)
			releasef(f, f->type);
		fdtab[i] = nil;
		++n;
		if (n==nfd)
			break;
	}
}
static Fd*
initstdfd(char *s, int n)
{
	DWORD	mode;
	Fd *f;

	f = mallocz(sizeof(Fd), 1);
	f->name	= strdup(s);
	f->h = GetStdHandle(n);
	if (GetConsoleMode(f->h, &mode))
		f->type = Fdtypecons;
	else
		f->type = Fdtypestd;

	f->users = 1;
	if (f->h==nil)
		sysfatal("GetStdHandle %s", s);
	return f;
}
static
void
need(int n)
{
	int	oldsz;

	if (n>fdtabsz) {
		oldsz = fdtabsz;
		while(n>fdtabsz)
			fdtabsz += 32;
		fdtab = realloc(fdtab, fdtabsz * sizeof(Fd*));
		memset(fdtab+oldsz, 0, (fdtabsz-oldsz) * sizeof(Fd*));
	}
	if (fdtab==nil)
		sysfatal("fdtab realloc");
}

static QLock lk;

void
fdtabinit(void)
{
	char *d;

	d = getenv("FDTDEBUG");
	if (d!=nil) {
		fdtdebug= atoi(d);
		free(d);
	}
	nfd = 3;
	need(nfd);
	fdtab[2] = initstdfd("<stderr>", STD_ERROR_HANDLE);
	fdtab[0] = initstdfd("<stdin>", STD_INPUT_HANDLE);
	fdtab[1] = initstdfd("<stdout>", STD_OUTPUT_HANDLE);
	atexit(cleanup);
}

int
fdtalloc(Fd *dupf)
{
	Fd	*f;
	int	i;

	qlock(&lk);
	need(nfd+1);
	for (i=0; i<nfd; i++)
		if (fdtab[i]==nil)
			break;
	if (dupf != nil) {
		++dupf->users;
		f = dupf;
	} else {
		f = mallocz(sizeof(Fd), 1);
		if (f==nil)
			sysfatal("fdtab malloc");
		f->name = nil;
		f->users = 1;
	}
	fdtab[i] = f;
	++nfd;
	qunlock(&lk);

	return i;
}
int
fdtdup(int oldfd, int newfd)
{
	Fd	*oldf, *f, *rf;
	int	type;

	oldf = fdtget(oldfd);
	if (oldf==nil)
		return -1;
	if (newfd==-1)
		newfd = fdtalloc(oldf);
	else {
		rf = nil;
		type = 0;
		qlock(&lk);
		if (newfd >= fdtabsz)
			need(newfd+1);
		else {
			f = fdtab[newfd];
			if (f!=nil) {
				--f->users;
				if (f->users==0) {
					rf = f;
					type = f->type;
					f->type = 0;
				}
				--nfd;
			}
		}
		fdtab[newfd] = oldf;
		++nfd;
		++oldf->users;
		qunlock(&lk);
		if (rf!=nil)
			releasef(f, type);
	}
	return newfd;
}

int
fdtclose(int fd)
{
	Fd	*f, *rf;
	int	type;

	rf = nil;
	f = fdtget(fd);
	if (f==nil)
		return -1;

	qlock(&lk);
	fdtab[fd] = nil;
	--nfd;
	--f->users;
	if (f->users==0) {
		type = f->type;
		f->type = 0;
		rf = f;
	}
	qunlock(&lk);
	if (fdtdebug>1)
		fprint(2, "closing %d (%s, %d users) of %d\n", fd, f->name, f->users, nfd);
	if (rf!=nil)
		releasef(f, type);

	return 0;
}

static
void
initovev(HANDLE *hev)
{
	HANDLE	h;

	qlock(&lk);
	if (*hev==nil) {
		h = CreateEvent (nil, 1 /* manual reset */, 0 /* initial */, nil);
		if (h!=INVALID_HANDLE_VALUE)
			*hev = h;
	}
	qunlock(&lk);
}
Fd*
fdtget(int fd)
{
	Fd *f;

	if(fd<0 || fd>=fdtabsz) {
		werrstr("invalid file descriptor: %d", fd);
		return nil;
	}

	f = fdtab[fd];
	if (f==nil)
		werrstr("fd does not refer to an open file: %d", fd);
	else
		switch(f->type) {
		case Fdtypepipesrv:
		case Fdtypepipecl:
			if (f->ev.r==nil)
				initovev(&f->ev.r);
			if (f->ev.w==nil)
				initovev(&f->ev.w);
		}
	return f;
}