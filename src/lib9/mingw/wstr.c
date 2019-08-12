/*
 * Some support for functions that are not part of MingW
 *
 * From Inferno utilities:
 * 	sbrk()
 *
 */

#include <u.h>
#include <mingw32.h>
#include <libc.h>

#include "util.h"
#include "fdtab.h"

int
winwstrlen(WCHAR *w)
{
	int	n;

	for(n=0; *w != 0; n++)
		;
	return n;
}
int
winwstrutflen(WCHAR *w)
{
	int n;
	
	for(n=0; *w; n+=runelen(*w), w++)
		;
	return n;
}
int
winwstrtoutfn(char *s, int n, WCHAR *w)
{
	int i;
	char *s0;
	Rune r;

	s0 = s;
	if(n <= 0)
		return 0;
	while(*w) {
		if(n < UTFmax+1 && n < runelen(*w)+1) {
			*s = 0;
			return s-s0+winwstrutflen(w)+1;
		}
		r = *w;
		i = runetochar(s, &r);
		s += i;
		n -= i;
		w++;
	}
	*s = 0;
	return s-s0;
}
char*
winwstrtoutfm(WCHAR *w)
{
	char *s;
	int n;

	n = winwstrutflen(w);
	s = malloc(n+1);
	if(s==nil)
		return nil;
	winwstrtoutfn(s, n+1, w);
	return s;
}
char*
winwstrtoutfe(char *wp, char *ep, WCHAR *w)
{
	return wp+winwstrtoutfn(wp, ep-wp, w);
}

int
winutftowstr(WCHAR *w, char *s, int n)
{
	int len;
	WCHAR *e;
	Rune r;

	len = utflen(s);
	if(len >= n)
		return len;
	e = w+n-1;
	while(w<e && *s){
		s += chartorune(&r, s);
		*w = r&0xFFFF;
		w++;
	}
	*w = '\0';
	return len;
}

LPWSTR
winutf2wstr(char *s)
{
	WCHAR *w;
	int len;

	if(s == nil)
		return nil;

	len = utflen(s)+1;
	w = malloc(sizeof(WCHAR)*len);
	winutftowstr(w, s, len);
	return w;
}
