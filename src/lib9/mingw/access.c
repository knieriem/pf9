#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#define NOPLAN9DEFINES
#include <libc.h>

int
p9access(char *name, int mode)
{
	char path[MAX_PATH], *shell;
	int omode;
	int fd;

	omode = OREAD;

	if (mode!=AEXIST) {
		if (mode&AWRITE) {
			if (mode&AREAD)
				omode = ORDWR;
			else
				omode = OWRITE;
		}
	}
	if (mode&AEXEC) {
		if (!winexecpath(path, name, &shell))
			return -1;

		if (shell!=nil)
			free(shell);
		name = path;
	}
	fd = p9open(name, omode);
	if (fd<0)
		return -1;
	close(fd);
	return 0;
}

