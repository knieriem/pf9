#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>

void
memimagedraw(Memimage *dst, Rectangle r, Memimage *src, Point sp, Memimage *mask, Point mp, int op)
{
	Memdrawparam *par;

	par = _memimagedrawsetup(dst, r, src, sp, mask, mp, op);
	if (par==nil)
		return;
	_memimagedraw(par);
}

u32int
pixelbits(Memimage *m, Point p)
{
	return _pixelbits(m, p);
}

