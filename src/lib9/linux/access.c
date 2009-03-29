#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>

int
p9access(char *name, int mode)
{
	return access(name, mode);
}
