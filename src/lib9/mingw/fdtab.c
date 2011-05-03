#include <u.h>
#include <mingw32.h>
#include <libc.h>
#include <dirent.h>

#include "util.h"
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

static
int
Dconv(Fmt *fmt)
{
	char *fn;
	Fd *f;
	int fd;

	fd = va_arg(fmt->args, int);
	f = va_arg(fmt->args, Fd*);
	fn = va_arg(fmt->args, char*);
	
	return fmtprint(fmt, "[%d] %d /%d\t%s\t%s \"%s\"", getpid(),
		fd, f->users, fn, typestr[f->type], f->name!=nil?f->name:"");
}


int	fdtdebug;
#define	dprint	if(fdtdebug>0)fprint
void
fdtprint(void)
{
	int	i, n;
	Fd	*f;

	fmtinstall('\f', Dconv);
	dprint(2, "fdtab %s:\n", argv0);
	n = 0;
	for (i=0; i<fdtabsz && n<nfd; i++) {
		f = fdtab[i];
		if (f==nil)
			continue;

		dprint(2, "%\f\n", i, f, "");
		++n;
		if (n==nfd)
			break;
	}
}
static
void
releasef(Fd *f, int type, int closehandle)
{
if(closehandle)
	/* don't call this function in a locked section */
		switch (type) {
		case Fdtypestd:
			break;
		case Fdtypepipesrv:
			FlushFileBuffers(f->h); 
			DisconnectNamedPipe(f->h);
			/* fall through */
		case Fdtypefile:
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
		if (f->onclose!=nil)
			(*f->onclose)(f->onclosearg);
		free(f);
}
static
void
cleanup(void)
{
	int	i;

	fdtprint();
	for (i=0; i<fdtabsz; i++)
		if(fdtab[i] != nil)
		if(i != 2)
			close(i);
	if(fdtab[2]!=nil){
		if(nfd!=1)
			sysfatal("fdt cleanup: nfd still %d\n", nfd-1);
		close(2);
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
		if(fdtdebug)
			fmtinstall('\f', Dconv);
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
				dprint(2, "%\f: closing %d\n", oldfd, f, "dup", newfd);
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
			releasef(rf, type, 1);
	}
	dprint(2, "%\f ->%d\n", oldfd, oldf, "dup", newfd);
	return newfd;
}

void
fdtonclose(int fd, void (*fn)(void*), void *v)
{
	Fd	*f;

	f = fdtget(fd);
	if (f==nil)
		return;
	f->onclosearg = v;
	f->onclose = fn;	
}

int
fdtclose(int fd, int closehandle)
{
	Fd	*f, *rf, tf;
	int	i, fatal;

	rf = nil;
	f = fdtget(fd);
	if (f==nil)
		return -1;

	fatal = 0;
	qlock(&lk);
	fdtab[fd] = nil;
	--nfd;
	--f->users;
	tf = *f;
	if (f->users==0) {
		f->type = 0;
		rf = f;
	}else{
		/* consistency check: there must be at least one more reference */
		for (i=0; i<fdtabsz; i++)
			if(fdtab[i]==f)
				goto out;
		fatal = 1;
	}
out:
	qunlock(&lk);
	if (fatal)
		sysfatal("fdtclose: fd %d: no %d users\n", fd, f->users);
	if (fdtdebug>1)
	if (tf.type!=Fdtypenone)
		fprint(2, "%\f #%d\n", fd, &tf, "closed", nfd);
	if (rf!=nil)
		releasef(rf, tf.type, closehandle);

	return 0;
}

static
void
initovev(HANDLE *hev)
{
	qlock(&lk);
	if (*hev==nil)
		wincreatevent(hev, "initovev", 1 /* manual reset */, 0 /* initial */);
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
