#include <u.h>
#include <mingw32.h>
#include <time.h>
#define NOPLAN9DEFINES
#include <libc.h>



/*
 * Definition of gettimeofday by Wu Yongwei, taken from
 * see http://mywebpage.netscape.com/yongweiwu/timeval.h.txt
 * In the public domain.
 */

#define EPOCHFILETIME (116444736000000000LL)


static int
gettimeofday(struct timeval *tv)
{
	FILETIME	ft;
	LARGE_INTEGER li;
	__int64	t;
	
	if (tv) {
		GetSystemTimeAsFileTime(&ft);
		li.LowPart  = ft.dwLowDateTime;
		li.HighPart = ft.dwHighDateTime;
		t = li.QuadPart;		/* In 100-nanosecond intervals */
		t -= EPOCHFILETIME;	/* Offset to the Epoch time */
		t /= 10;				/* In microseconds */
		tv->tv_sec = (long)(t / 1000000);
		tv->tv_usec = (long)(t % 1000000);
	}
	
	return 0;
}


long
p9times(long *t)
{
	/* BUG */
	return -1;
}

double
p9cputime(void)
{
	long t[4];
	double d;

	if(p9times(t) < 0)
		return -1.0;

	d = (double)t[0]+(double)t[1]+(double)t[2]+(double)t[3];
	return d/1000.0;
}

vlong
p9nsec(void)
{
	struct timeval tv;

	if(gettimeofday(&tv) < 0)
		return -1;

	return (vlong)tv.tv_sec*1000*1000*1000 + tv.tv_usec*1000;
}

long
p9time(long *tt)
{
	long t;
	t = time(0);
	if(tt)
		*tt = t;
	return t;
}

