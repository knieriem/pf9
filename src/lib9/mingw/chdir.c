#include <u.h>
#include <mingwutil.h>
#include <mingw32.h>
#include <libc.h>

#include "util.h"

int
p9chdir(char *d)
{
	WCHAR	*wname;
	long	s;
	int	r;

	d = winpathdup(d);
	if (d==nil) {
		werrstr("out of memory");
		return -1;
	}
	s = strlen(d);
	if (d[s-1] != '/') {
		d[s] = '/';
		d[s+1] = '\0';
	}
	wname = winutf2wpath(d);
	r = SetCurrentDirectoryW(wname);
	free(wname);
	free(d);
	if (!r) {
		winerror(nil);
		return -1;
	}
	return 0;
}
