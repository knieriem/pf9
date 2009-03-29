#include <u.h>
#include <mingwsock.h>
#include <libc.h>

static char*
nsfromwsock(void)
{
	char host[32];
	char *p;

	if (gethostname(host, sizeof host)!=0)
		return nil;
	p = smprint("/tmp/ns.%s", host);
	if(p == nil){
		werrstr("out of memory");
		return p;
	}
	return p;
}

char*
getns(void)
{
	char *ns;

	ns = getenv("NAMESPACE");
	if(ns == nil)
		ns = nsfromwsock();
	if(ns == nil){
		werrstr("$NAMESPACE not set, %r");
		return nil;
	}
	return ns;
}
