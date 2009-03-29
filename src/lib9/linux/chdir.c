#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>

int
p9chdir(char *d)
{
	return chdir(d);
}
