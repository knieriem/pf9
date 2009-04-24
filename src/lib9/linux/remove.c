#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>
#include <stdio.h>

int
p9remove(char *file)
{
	return remove(file);
}
