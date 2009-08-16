#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#define NOPLAN9DEFINES
#include <libc.h>

#include "fdtab.h"
#include "util.h"

long
p9read(int fd, void *buf, long nbytes)
{
	OVERLAPPED ov;
	DWORD	nread;
	QLock	*lk;
	Fd *f;

	f = fdtget(fd);
	if (f==nil)
		return -1;

//	fprint(2, "Rd%d ", fd);
	lk = nil;
	nread = -1;
	switch (f->type) {
	case Fdtypedir:
		werrstr("reading from a directory fd not implemented");
		break;
	case Fdtypecons:
		nread = winreadcons(f->h, buf, nbytes);
		break;
	case Fdtypefile:
	case Fdtypestd:
		qlock(lk=&f->lk.rw);
		if (!ReadFile(f->h, buf, nbytes, &nread, nil))
			switch(GetLastError()) {
			case ERROR_BROKEN_PIPE:
			case ERROR_PIPE_NOT_CONNECTED:
				nread = 0;	/* in case the file is one of stdin/out/err pipes */
				break;
			default:
				winerror("ReadFile");
				qunlock(lk);
				return -1;
			}
		break;

	case Fdtypepipecl:
	case Fdtypepipesrv:
		memset(&ov, 0, sizeof ov);
		ov.hEvent = f->ev.r;
		qlock(lk=&f->lk.r);
//		fprint(2, "p9read: %p %p", f->ev.r, f->ev.w);
		switch (winovresult(ReadFile(f->h, buf, nbytes, &nread, &ov), f->h, &ov, &nread, 0)) {
		case ERROR_BROKEN_PIPE:
		case ERROR_PIPE_NOT_CONNECTED:
			nread = 0;		/* the other end has been closed */
		case 0:
			break;
		default:
			winerror("ReadFile");
			qunlock(lk);
			return -1;
		}
		break;

	case Fdtypesock:
		nread = recv(f->s, buf, nbytes, 0);
		if (nread == SOCKET_ERROR) {
			werrstr("WSA socket error code: %d", WSAGetLastError());
			return -1;
		}
		break;;

	case Fdtypedevnull:
		return 0;
	}

	if (lk!=nil)
		qunlock(lk);
	return nread;
}
