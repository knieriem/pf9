#include <u.h>
#include <mingw32.h>
#define NOPLAN9DEFINES
#include <libc.h>

#include "fdtab.h"

int
p9close(int fd)
{
	fdtclose(fd, 1);

	return 0;
}
