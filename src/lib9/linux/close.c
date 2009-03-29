#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>


/*
 * mingw/dirread.c will register a cleanup function here
 */
void (*atclose_unreg_dirfd)(int);


int
p9close(int fd)
{
	if (atclose_unreg_dirfd)
		atclose_unreg_dirfd(fd);
	
	return close(fd);
}
