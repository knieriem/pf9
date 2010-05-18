#include <u.h>
#include <mingw32.h>
#include <libc.h>
#include <sys/stat.h>
#include "util.h"

int
mingwremove(const char *f)
{
	struct _stat st;
	WCHAR *wf;
	char *file;

	file = winpathdup((char*)f);
	if(file==nil)
		return -1;
	wf = winutf2wpath(file);
	if(_wstat(wf, &st)<0){
	err:
		free(wf);
		free(file);
		return -1;
	}
	if(st.st_mode & _S_IFDIR){
		if(_wrmdir(wf)<0)
			goto err;
	}else if(_wunlink(wf)<0)
			goto err;
	
	free(wf);
	free(file);
	return 0;
}
