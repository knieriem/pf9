#include <u.h>
#include <mingw32.h>
#include <libc.h>

#include "util.h"


enum {
	NBUF	= _MAX_PATH,
	BUFSZ	= sizeof(Rune)*NBUF,
};

static
Rune*
beautyseg(Rune *w, Rune *ep, Rune **ppl, Rune **pps)
{
	int	copyshort;
	Rune *p, *w0;

	if (w>=ep)
		return w;
		
	w0 = w;
	copyshort = 0;

	/* check long path segment */
	for (p = *ppl; *p!='\0'; p++) {
		if (*p==' ')
			copyshort=1;
		*w++ = *p;
		if (w==ep)
			goto oflow;
		if (*p=='\\') {
			++p;
			break;
		}
	}
	*ppl = p;

	/* eat short path segment, copy if required */
	if (copyshort)
		w = w0;
	for (p = *pps; *p!='\0'; p++) {
		if (copyshort) {
			*w++ = *p;
			if (w==ep)
				goto oflow;
		}
		if (*p=='\\') {
			++p;
			break;
		}
	}
	*pps = p;

	if (w==ep) {
oflow:
		if (w>w0)
			*--w = '\0';
	} else
		*w = '\0';
	return w;
}
WINBASEAPI DWORD WINAPI GetLongPathNameW(LPCWSTR,LPWSTR,DWORD);
static
void
beautypath(Rune *wbuf)
{
	Rune *wl, *ws, *pl, *ps, *p, *ep;

	ws = malloc(BUFSZ);
	if (ws==nil)
		return;
	wl = malloc(BUFSZ);
	if (wl==nil)
		goto fws;

	if (GetLongPathNameW(wbuf, wl, NBUF))
	if (GetShortPathNameW(wbuf, ws, NBUF)) {
		p = wbuf;
		ep = p+NBUF;
		ps = ws;
		pl = wl;
		while (*pl != '\0' && *ps != '\0')
			p = beautyseg(p, ep, &pl, &ps);
	}

	free(wl);
fws:
	free(ws);
}


#undef getwd

char*
p9getwd(char *s, int ns)
{
	Rune wbuf[NBUF];
	char *ep;
	long	sz;

	if (_wgetcwd(wbuf, nelem(wbuf)) == NULL)
		return nil;

	beautypath(wbuf);

	ep = s+ns;
	seprint(s, ep, "/%S", wbuf);
	if (winisdrvspec(s+1))
		s[2] = '-';
	else
		seprint(s, ep, "%S", wbuf);
	winbsl2sl(s);
	sz = strlen(s);
	if (s[sz-1]=='/')
		s[sz-1] = '\0';
  	return s;
}
