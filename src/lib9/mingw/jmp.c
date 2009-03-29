#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>

/*
 * NOTE: no sig* versions of set/longjmp available
 */

void
p9longjmp(p9jmp_buf buf, int val)
{
	longjmp((void*)buf, val);
}

void
p9notejmp(void *x, p9jmp_buf buf, int val)
{
	USED(x);
	longjmp((void*)buf, val);
}

