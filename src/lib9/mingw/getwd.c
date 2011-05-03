#include <u.h>
#include <mingw32.h>
#include <libc.h>

#include "util.h"


enum {
	NBUF	= _MAX_PATH,
	BUFSZ	= sizeof(WCHAR)*NBUF,
};

#undef getwd

char*
p9getwd(char *s, int ns)
{
	WCHAR wbuf[NBUF];
	char *ep;
	long	sz;

	if (_wgetcwd(wbuf, nelem(wbuf)) == NULL)
		return nil;

	winreplacews(wbuf, 0);

	ep = s+ns;
	if(s<ep){
		*s = '/';
		winwstrtoutfe(s+1, ep, wbuf);
	}
	if (winisdrvspec(s+1))
		s[2] = '-';
	else
		winwstrtoutfe(s, ep, wbuf);
	winbsl2sl(s);
	sz = strlen(s);
	if (s[sz-1]=='/')
		s[sz-1] = '\0';
  	return s;
}
