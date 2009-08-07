#include <u.h>
#include <mingwutil.h>
#include <mingw32.h>
#include <libc.h>
#include <thread.h>

#include "util.h"

#if 0
static
Rune**
runeargv(char *argv[])
{
	Rune **v;
	char **p, *dp;
	long	ds, s;
	int i, n;

	n = 0;
	for (p=argv; *p!=nil; p++)
		++n;

	v = malloc(sizeof(Rune*)*(n+1));
	if (v==nil)
		return nil;

	ds = 0;
	dp = nil;
	for (i=0; i<n; i++) {
		s = strlen(argv[i])+1;
		if (2*s>ds) {
			ds = 2*s + 10;
			dp = realloc(dp, ds);
		}
		winargdblquote(dp, argv[i]);
		v[i] = runesmprint("%s", dp);
	}
	v[i] = nil;
	free(dp);
	return v;
}
_CRTIMP int	__cdecl	_wexecve(Rune*, Rune*[], Rune*[]);
#endif

int
winexecve(char *prog, char *argv[], char *env[])
{
	Waitmsg *w;
	int pid;
	int fd[3];

	fd[0] = dup(0, -1);
	fd[1] = dup(1, -1);
	fd[2] = dup(2, -1);

	pid = winspawne(fd, prog, argv, env, 1);
	if (pid<0) {
		close(fd[0]);
		close(fd[1]);
		close(fd[2]);
		return -1;
	} else {
		w = waitfor(pid);
//		fprint(2, "SCHNONK %s\n", prog);
		if (w==nil)
			threadexitsall("waitfor");
		threadexitsall(w->msg);
	}

	return 0;
}
int
exec(char *prog, char *argv[])
{
	extern char **environ;

	return winexecve(prog, argv, environ);
}
