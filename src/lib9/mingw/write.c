#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#define NOPLAN9DEFINES
#include <libc.h>

#include "fdtab.h"
#include "util.h"

#include <stdio.h>
long
p9write(int fd, void *buf, long nbytes)
{
	OVERLAPPED ov;
	DWORD	nw;
	QLock	*lk;
	Fd *f;

//	if (fd>2)
//		fprint(2, "Wr%d ", fd);
	f = fdtget(fd);
	if (f==nil)
		return -1;
	lk = nil;
	switch (f->type) {
	case Fdtypefile:
	case Fdtypestd:
		lk=&f->lk.rw;
		qlock(lk);
		if (!WriteFile(f->h, buf, nbytes, &nw, NULL))
			goto failed;
		break;

	case Fdtypecons:
		lk=&f->lk.w;
		qlock(lk);
		if (!winwritecons(f->h, buf, nbytes))
			goto failed;
		nw = nbytes;
		break;

	case Fdtypepipecl:
	case Fdtypepipesrv:
		memset(&ov, 0, sizeof ov);
		ov.hEvent = f->ev.w;
		lk=&f->lk.w;
		qlock(lk);
		if (winovresult(WriteFile(f->h, buf, nbytes, &nw, &ov), f->h, &ov, &nw, 0)!=0) {
		failed:
			winerror("WriteFile");
			qunlock(lk);
			return -1;
		}
		break;

	case Fdtypesock:
		nw = send(f->s, buf, nbytes, 0);
		if (nw == SOCKET_ERROR) {
			werrstr("WSA socket error code: %d", WSAGetLastError());
			return -1;
		}
		break;

	case Fdtypedevnull:
		nw = nbytes;
	}

	if (lk!=nil)
		qunlock(lk);
	return nw;
}
