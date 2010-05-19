#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#define NOPLAN9DEFINES
#include <libc.h>

#include "util.h"
#include "fdtab.h"

int
p9create(char *name, int mode, ulong perm)
{
	int	fd, rdwr;
	int	trytrunc;
	DWORD	da, share, flags, cmode;
	HANDLE	h;
	char *path;
	Fd	*f;

	rdwr = mode&3;
	da = 0;
	trytrunc = 0;
	h = INVALID_HANDLE_VALUE;
	switch (rdwr) {
	case OREAD:
		da |= GENERIC_READ;
		break;
	case ORDWR:
		da |= GENERIC_READ;
	case OWRITE:
 		da |= GENERIC_WRITE;
		trytrunc = 1;
	}
	mode &= ~(OCEXEC|OLOCK);
	share = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE;
	flags = 0;

	name = winadjustcons(name, rdwr, &da);
	if (name==nil)
		return -1;
	path = winpathdup(name);
	if (path==nil)
		return -1;
	fd = fdtalloc(nil);
	f = fdtab[fd];
	f->name = path;
	
	if (!strcmp("/dev/null", name)) {
		f->type = Fdtypedevnull;
		return fd;
	}

	/* XXX should get mode mask right? */
	if(perm&DMDIR){
		if(mode != OREAD){
			werrstr("bad mode in directory create");
			goto fail;
		}
		if(!wincreatedir(path))
			goto fail;
	}else{
		mode &= ~(3|OTRUNC);
		if(mode&OEXCL){
			share = 0;
			mode &= ~OEXCL;
		}
		if(mode&OAPPEND){
			da = FILE_APPEND_DATA;
			mode ^= OAPPEND;
		}
		if(mode&ODIRECT){
			flags |= FILE_FLAG_WRITE_THROUGH;
			mode ^= ODIRECT;
		}
		if(mode&ORCLOSE){
			flags |= FILE_FLAG_DELETE_ON_CLOSE;
			mode ^= ORCLOSE;
		}
		if(mode){
			werrstr("unsupported mode in create");
			goto fail;
		}
		if (flags==0)
			flags = FILE_ATTRIBUTE_NORMAL;
		if (trytrunc)
			h = wincreatefile(path, da, share, TRUNCATE_EXISTING, flags);
		if (h==INVALID_HANDLE_VALUE) {
			h = wincreatefile(path, da, share, CREATE_NEW, flags);
			if (h==INVALID_HANDLE_VALUE) {
				winerror("CreateFile");
			fail:
				if (fdtdebug>1)
					fprint(2, "create %s failed: %r\n", path);
				fdtclose(fd, 1);
				return -1;
			}
		}
	}

	if (GetConsoleMode(h, &cmode))
		f->type = Fdtypecons;
	else
		f->type = Fdtypefile;
	f->h = h;
	if (fdtdebug>1)
		fprint(2, "%\f h:%p\n", fd, f, "create", h);
	return fd;
}
