#include <u.h>
#include <mingw32.h>
#include <libc.h>

#include "util.h"

enum {
	Nmore = 10,
};
char **mingwenviron;
static int	envsz;
int
mingwinitenv(WCHAR *wenv[])
{
	int	n;
	WCHAR **rp;
	char **p;

	if (wenv==nil) {
		WCHAR dummy[] = {'D', '\0'};

		_wgetenv(dummy);			/* initialize */
		wenv = _wenviron;
	}

	n = 0;
	for (rp=wenv; *rp!=nil; rp++)
		++n;

	envsz = n+Nmore;
	environ = malloc(sizeof(char*) * (envsz+1));
	if (environ==nil)
		return -1;

	p = environ;
	for (rp=wenv; *rp!=nil; rp++) {
		*p = winwstrtoutfm(*rp);
		if (*p!=nil) {
			/* make sure PATH is uppercase */
			if ((*p)[0]=='P' && !strncasecmp("Path=", *p, 5))
				memcpy(*p, "PATH", 4);				
		}
		++p;
	}
	*p = nil;

	return 0;
}

static char**
get(char *s, char **vp)
{
	char **p;
	int	l;

	for (p=environ; *p!=nil; p++) {
		l = strlen(s);
		if (strncasecmp(*p, s, l)==0)
		if ((*p)[l]=='=') {
			if (vp != nil)
				*vp = &(*p)[l+1];
			return p;
		}
	}
	return nil;
}

char*
p9getenv(char *s)
{
	char *t;

	if (get(s, &t) != nil)
		return strdup(t);
	return nil;
}

int
p9putenv(char *s, char *v)
{
	char **p, *np, **nep;
	long	ss, vs, l;
	int	n;

//	fprint(2, "putenv: %s=%s\n", s, v);
	p = get(s, nil);
	ss = strlen(s);
	vs = strlen(v);
	if (p!=nil) {
		if (s==0)
			/* remove */
			do {
				*p = *(p+1);
			} while (*p++ != nil);
		else {
			/* update */
			l = strlen(&(*p)[ss+1]);
			if (l<vs) {
				np = realloc(*p, ss+vs+2);
				if (np==nil)
					return -1;
				*p = np;
			}
			strcpy(&(*p)[ss+1], v);
		}
		return 0;
	}

	for (p=environ; *p!=nil; )
		p++;

	n = p-environ;
	if (n==envsz) {
		n += Nmore;
		nep = realloc(environ, sizeof(char*) * (n+1));
		if (nep==nil)
			return -1;
		envsz = n;
		p = nep + (p-environ);
		environ = nep;
	}

	np = smprint("%s=%s", s, v);
	if (np==nil)
		return -1;
	*p = np;
	*(p+1) = nil;
	return 0;
}


WCHAR **
winruneenv(void)
{
	WCHAR **rp, **wenv, *r;
	char **p;
	int n;

	winsortenv(environ);

	n = 0;
	for (p=environ; *p!=nil; p++)
		n += utflen(*p)+1;

	wenv = malloc(sizeof(WCHAR*)*(envsz+1) + sizeof(WCHAR)*n);
	if (wenv==nil)
		return nil;

	rp = wenv;
	r = (WCHAR*) (rp + envsz+1);
	for (p=environ; *p!=nil; p++) {
		*rp++ = r;
		r += winutftowstr(r, *p, strlen(*p));
	}
	*rp = '\0';
	return wenv;
}


static int
cmpenv(const void *a, const void *b)
{
	return strcasecmp(*(char**)a, *(char**)b);
}
void
winsortenv(char **e)
{
	char **ep;
	int n;

	n = 0;
	for(ep=e; *ep; ep++)
		n++;
	qsort((char *)e, n, sizeof e[0], cmpenv);
}
