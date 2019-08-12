#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#include <libc.h>

#include "util.h"

int
post9pservice(int fd, char *name, char *mtpt)
{
	char *ns, *s;
	Waitmsg *w;
	int	fds[3];

	if(strchr(name, '!'))	/* assume is already network address */
		s = strdup(name);
	else{
		if((ns = getns()) == nil)
			return -1;
		s = smprint("unix!%s/%s", ns, name);
		free(ns);
	}
	if(s == nil)
		return -1;
	fds[0] = dup(fd, -1);
	fds[1] = fds[0];
	fds[2] = dup(2, -1);
	winspawnl(fds, "9pserve", "9pserve", "-u", s, (char*)0);
	return 0;
	w = wait();
	if(w == nil)
		return -1;
	free(s);
	if(w->msg && w->msg[0]){
		free(w);
		werrstr("9pserve failed");
		return -1;
	}
	free(w);
	return 0;
}
