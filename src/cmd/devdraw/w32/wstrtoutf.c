#include <u.h>
#include <mingw32.h>
#include <libc.h>

int
wstrutflen(WCHAR *s)
{
	int n;
	
	for(n=0; *s; n+=runelen(*s),s++)
		;
	return n;
}

int
wstrtoutf(char *s, WCHAR *t, int n)
{
	int i;
	char *s0;
	Rune r;

	s0 = s;
	if(n <= 0)
		return wstrutflen(t)+1;
	while(*t) {
		if(n < UTFmax+1 && n < runelen(*t)+1) {
			*s = 0;
			return s-s0+wstrutflen(t)+1;
		}
		r = *t;
		i = runetochar(s, &r);
		s += i;
		n -= i;
		t++;
	}
	*s = 0;
	return s-s0;
}
