#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#include <libc.h>

#include "fdtab.h"

struct Msg {
	int	pid;
	HANDLE h;
};
int
sendfd(int s, int fd)
{
	struct Msg m;
	Fd	*f;

	f = fdtget(fd);
	if (f==nil)
		return -1;
	switch (f->type) {
	default:
		werrstr("not implemented");
		return -1;
	case Fdtypepipecl:
	case Fdtypepipesrv:
		break;
	}
	m.pid = getpid();
	m.h = f->h;
	write(s, &m, sizeof m);
	fdtclose(fd, 0);
	return 0;
}
extern int fdtdebug;
int
recvfd(int s)
{
	HANDLE ht, hsp, htp;
	struct Msg m;
	Fd	*f;
	int	fd;

	if(read(s, &m, sizeof m) != sizeof m)
		return -1;

	hsp = OpenProcess(PROCESS_DUP_HANDLE, 0, m.pid);
	if(hsp==nil)
		goto err;

	htp = OpenProcess(PROCESS_DUP_HANDLE, 0, getpid());
	if(htp==nil){
	errcs:
		CloseHandle(hsp);
	err:
		winerror(nil);
		return -1;
	}
	if(!DuplicateHandle(hsp, m.h, htp, &ht, 0, 0, DUPLICATE_SAME_ACCESS|DUPLICATE_CLOSE_SOURCE)){
		CloseHandle(htp);
		goto errcs;
	}
	CloseHandle(hsp);
	CloseHandle(htp);

	fd = fdtalloc(nil);
	f = fdtab[fd];
	f->name = strdup("recvfd");
	f->type = Fdtypepipecl;
	f->h = ht;

	if (fdtdebug>1)
		fprint(2, "%\f\n", fd, f, "recvfd");
	return fd;
}
