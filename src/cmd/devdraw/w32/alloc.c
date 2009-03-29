#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>

Memimage*
allocmemimage(Rectangle r, u32int chan)
{
	return _allocmemimage(r, chan);
}

void
freememimage(Memimage *i)
{
	_freememimage(i);
}

void
memfillcolor(Memimage *i, u32int val)
{
	_memfillcolor(i, val);
}

