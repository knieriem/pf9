#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#include <libc.h>
#include <stdio.h>

#include "fdtab.h"
#include "util.h"

BOOL
winwritecons(HANDLE h, void *v, int n)
{
static char utf[UTFmax], nutf;
	DWORD nw;
	Rune *wbuf, *r;
	uchar *buf, *s;
	int	res, l;

	if (n==0)
		return 1;

	n += nutf;
	wbuf = malloc(n*sizeof(Rune));
	if (wbuf==nil)
		return 0;

	if (nutf>0) {
		buf = malloc(n*sizeof(char));
		if (buf==nil) {
			free(wbuf);
			return 0;
		}
		memcpy(buf, utf, nutf);
		memcpy(buf+nutf, v, n-nutf);
		nutf = 0;
		v = nil;
	} else
		buf = v;
	s = buf;
	for (r=wbuf; n>0; ) {
		if (*s<Runeself) {
			*r = *s;
			--n;
			++s;
			++r;
		} else {
			if (n<UTFmax)
				if (!fullrune(s, n)) {
					nutf = n;
					memcpy(utf, s, nutf);
//					fprintf(stderr, "CONS: not a full rune!");
					break;
				}
			l = chartorune(r, s);
			if (*r!=Runeerror)
				++r;
//			else
//				fprintf(stderr, "CONS: runerror: %02x\n", *s);
			n -= l;
			s += l;
		}
	}
	n = r-wbuf;
	res = WriteConsoleW(h, wbuf, n, &nw, nil);
	if (nw!=n)
		res = 0;
	free(wbuf);
	if (v==nil)
		free(buf);
	return res;
}

static int eof;
int
winreadcons(HANDLE h, void *buf, int nwant)
{
	static Rune wbuf[8], *wp;
	static DWORD	wn;
	char	utf[UTFmax], *s;
	int	n, l, wascr;
	int	ncalls;

	if (eof)
		return 0;

	if (nwant==0)
		return 0;

	ncalls = 0;
	s = buf;
	for (n=0; n<nwant;) {
		if (wn==0) {
			if (ncalls>0 && n>0)
				return n;			
			wp = wbuf;
			if (!ReadConsoleW(h, wbuf, nelem(wbuf), &wn, nil))
				return -1;
//		fprintf(stderr, "RC-%ld %08x\n", wn, m);
			if (wn==0)
				return 0;
			++ncalls;
		}
		switch (*wp) {
		case 'Z'-'@':
			eof = 1;
			return n;
		case '\r':
			*wp = '\n';
			wascr = 1;
			break;
		case '\n':
			if (wascr) {
				wascr = 0;
				goto Next;
			}
		default:
			wascr = 0;
		}
//		fprintf(stderr, "R%04x\n", *wp);
		l = runetochar(utf, wp);
		if (l > nwant-n)
			return n;
		memcpy(s+n, utf, l);
		n += l;
	Next:
		++wp;
		--wn;
	}
	return n;
}

void
winconsctl(Consctl cmd)
{
	Fd	*f;
	DWORD	m;

	f = fdtget(0);
	if (f==nil || f->type != Fdtypecons)
		return;

	if (!GetConsoleMode(f->h, &m))
		return;
	switch (cmd) {
	case RawOn:
		m &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
		break;
	case RawOff:
		m |= ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT;
		break;
	}
	SetConsoleMode(f->h, m);
}

int
winconscolumns(int *ncol)
{
	Fd	*f;
	CONSOLE_SCREEN_BUFFER_INFO i;

	f = fdtget(1);
	if (f==nil || f->type != Fdtypecons)
		return -1;

	if (!GetConsoleScreenBufferInfo(f->h, &i))
		return -1;

	*ncol = i.srWindow.Right-i.srWindow.Left+1;
//	fprint(2, "COL:%d\n", *ncol);
	return 0;	
}

int
winhascons(void)
{
	int	fd;

	fd = open("CONOUT$", OWRITE);
	if (fd<0)
		return 0;
	close(fd);
	return 1;
}