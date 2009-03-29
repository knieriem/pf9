#include <u.h>
#include <mingw32.h>
#define NOPLAN9DEFINES
#include <libc.h>

int
p9sleep(long milli)
{
	Sleep(milli);
	return 0;
}

