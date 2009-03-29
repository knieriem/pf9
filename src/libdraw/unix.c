#include <u.h>
#include <sys/stat.h>
#include <libc.h>
#include <draw.h>

vlong
_drawflength(int fd)
{
	Dir *d;
	vlong len;

	d = dirfstat(fd);
	if (d==nil)
		return -1;
	len = d->length;
	free(d);
	return len;
}

