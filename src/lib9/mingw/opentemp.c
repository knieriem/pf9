#include <u.h>
#include <libc.h>

static int
tmp(char *template, int mode)
{
	char *name;

	name = _mktemp(template);
	if (name == nil)
		return -1;
	return create(name, mode, 0600);
}

/* it seems mingw doesnt define it */
int
mingwmkstemp(char *template)
{
	return tmp(template, ORDWR);
}

int
opentemp(char *template, int mode)
{
	return tmp(template, mode|ORCLOSE);
}
