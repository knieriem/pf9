/*
 * Some support for functions that might be missing on lunix
 *
 */

#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>

long
p9write(int fd, void *buf, long nbytes)
{
	return write(fd, buf, nbytes);
}

long
p9read(int fd, void *buf, long nbytes)
{
	return read(fd, buf, nbytes);
}

int
p9close(int fd)
{
	return close(fd);
}
