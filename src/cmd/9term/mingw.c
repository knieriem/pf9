#include <u.h>
#include <libc.h>
#include "term.h"

int
getpts(int fd[], char *slave)
{
	pipe(fd);
	strcpy(slave, "faketty");
	return 0;
}

void
updatewinsize(int row, int col, int dx, int dy)
{
	
}

int
isecho(int fd)
{
	return 0;
}
int
setecho(int fd, int newe)
{
}
