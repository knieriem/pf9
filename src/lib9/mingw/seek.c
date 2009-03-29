#include <u.h>
#include <mingwutil.h>
#include <mingw32.h>
#include <libc.h>

#include "fdtab.h"

static char Ebadarg[] = "bad arg in system call";
static char Eisdir[] = "file is a directory";
static char Eisstream[] = "seek on a stream";

vlong
seek(int fd, vlong offset, int whence)
{
	DWORD	meth;
	LONG	h, l;
	Fd	*f;

	f = fdtget(fd);
	if (f==nil)
		return -1;

	switch(f->type){
	case Fdtypefile:
		break;
	case Fdtypedir:
		werrstr(Eisdir);
		return -1;
	default:
		werrstr(Eisstream);
		return -1;
	}

	switch (whence) {
	case 0:	meth = FILE_BEGIN; break;
	case 1:	meth = FILE_CURRENT; break;
	case 2:	meth = FILE_END; break;
	default:
		werrstr(Ebadarg);
		return -1;
	}

	h = offset>>32 & 0xFFFFFFFF;
	l = offset & 0xFFFFFFFF;

	l = SetFilePointer(f->h, l, &h, meth);
	if (l==INVALID_SET_FILE_POINTER)
		if (GetLastError()!=NO_ERROR){
			winerror(nil);
			return -1;
		}
	return (vlong)h<<32 | l;
}
