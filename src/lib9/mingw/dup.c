#include <u.h>
#include <mingw32.h>
#include <libc.h>

#include "fdtab.h"

#undef dup

int
p9dup(int old, int new)
{
	return fdtdup(old, new);
}
