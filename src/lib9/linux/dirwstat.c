#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>
#include <sys/time.h>
#include <utime.h>
#include <sys/stat.h>
#include <stdio.h>

int
dirwstat(char *file, Dir *dir)
{
	int ret;
	struct utimbuf ub;

	/* BUG handle more */
	ret = 0;
	if(~dir->mode != 0){
		if(chmod(file, dir->mode) < 0)
			ret = -1;
	}
	if(~dir->mtime != 0){
		ub.actime = dir->mtime;
		ub.modtime = dir->mtime;
		if(utime(file, &ub) < 0)
			ret = -1;
	}
	if(~dir->length != 0){
		if(truncate(file, dir->length) < 0)
			ret = -1;
	}
	if(dir->name != nil)
	if(dir->name[0] != '\0'){
		if(rename(file, dir->name) < 0)
			ret = -1;
	}
	return ret;
}
