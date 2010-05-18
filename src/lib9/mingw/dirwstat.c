#include <u.h>
#include <mingw32.h>
#define NOPLAN9DEFINES
#include <libc.h>
#include <sys/time.h>
#include <utime.h>
#include <sys/stat.h>

#include "util.h"

int
dirwstat(char *file, Dir *dir)
{
	WCHAR *wfile, *wto;
	int	ret;
	struct _utimbuf ub;

	file = winpathdup(file);
	if(file==nil)
		return -1;
	wfile = winutf2wpath(file);

	/* BUG handle more */
	ret = 0;
	if(~dir->mode != 0){
		if(_wchmod(wfile, dir->mode) < 0)
			ret = -1;
	}
	if(~dir->mtime != 0){
		ub.actime = dir->mtime;
		ub.modtime = dir->mtime;
		if(_wutime(wfile, &ub) < 0)
			ret = -1;
	}
	if(~dir->length != 0){
//		if(truncate(file, dir->length) < 0)
//			ret = -1;
	}
	if(dir->name != nil)
	if(dir->name[0] != '\0'){
		wto = winutf2wpath(dir->name);
		if(_wrename(wfile, wto) < 0)
			ret = -1;
		free(wto);
	}
	free(wfile);
	free(file);
	return ret;
}
