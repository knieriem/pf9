#include <u.h>
#include <mingw32.h>
#include <libc.h>
#include <sys/stat.h>
#include "util.h"

int
p9remove(char *file)
{
	struct _stat st;
	Rune *wf;

	file = winpathdup(file);
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
