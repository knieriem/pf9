#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#include <libc.h>

#include <dirent.h>
#include "util.h"
#include "fdtab.h"

int
open(char *name, int mode)
{
	char buf[MAX_PATH];
	WCHAR	*wname;
	Fd	*f;
	int fd, rdwr;
	DWORD	da, share, flags, dis, attr, cmode;
	HANDLE	h;

	da = 0;
	rdwr = mode&3;
	switch (rdwr) {
	case OREAD:
		da |= GENERIC_READ;
		break;
	case ORDWR:
		da |= GENERIC_READ;
	case OWRITE:
		da |= GENERIC_WRITE;
	}
	mode &= ~(3|OCEXEC|OLOCK);
	dis = OPEN_EXISTING;
	share = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE;
	flags = 0;
	if(mode&OTRUNC){
		dis = TRUNCATE_EXISTING;
		mode ^= OTRUNC;
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
		werrstr("mode 0x%x not supported", mode);
		return -1;
	}

	if (!strcmp(name, "/dev/stdin"))
		return dup(0, -1);
	if (!strcmp(name, "/dev/fd/0"))
		return dup(0, -1);
	name = winadjustcons(name, rdwr, &da);
	if (name==nil)
		return -1;

	if (!strcmp(name, "."))
		name = getwd(buf, sizeof buf);

	fd = fdtalloc(nil);
	f = fdtab[fd];
	f->name = winpathdup(name);

	if (!strcmp("/dev/null", name)) {
		f->type = Fdtypedevnull;
		return fd;
	}

	if(strncmp(f->name, "//./pipe", 8)==0){
		f->type = Fdtypepipecl;
		flags = FILE_FLAG_OVERLAPPED;
	}else{
		wname = winutf2wpath(f->name);
		attr = GetFileAttributesW(wname);
		if (attr!=INVALID_FILE_ATTRIBUTES)
		if (attr & FILE_ATTRIBUTE_DIRECTORY) {
			f->dir = _wopendir(wname);
			free(wname);
			if (f->dir==nil) {
				werrstr("could not open directory: %s", name);
				goto failed;
			}
			f->type = Fdtypedir;
			return fd;
		}/* else
		  * f->type is still unset
		  */

		free(wname);
	}

	if (flags==0)
		flags = FILE_ATTRIBUTE_NORMAL;
	h = wincreatefile(f->name, da, share, dis, flags);
	if (h==INVALID_HANDLE_VALUE) {
		winerror(nil);
	failed:
		if (fdtdebug>1)
			fprint(2, "open %s failed: %r\n", name);
		fdtclose(fd, 1);
		return -1;
	}

	if (GetConsoleMode(h, &cmode))
		f->type = Fdtypecons;
	else if(!f->type)
		f->type = Fdtypefile;
	f->h = h;

	if (fdtdebug>1)
		fprint(2, "%\f h:%p da:%08x\n", fd, f, "open", h, da);

	return fd;
}
