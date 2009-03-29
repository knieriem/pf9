#include <u.h>
#include <libc.h>

long
pread(int fd, void *buf, long nbytes, vlong offset)
{
	seek(fd, offset, 0);
	return read(fd, buf, nbytes);
}
long
pwrite(int fd, void *buf, long nbytes, vlong offset)
{
	seek(fd, offset, 0);
	return write(fd, buf, nbytes);
}
