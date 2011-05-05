#include <u.h>
#include <mingw32.h>
#include <libc.h>
#include <thread.h>

#include "term.h"

#define dprint	if(0)fprint

static int fdslave[2];
static int fdmaster[2];

static
void
copyslaveproc(void *v)
{
	uchar buf[4096];
	int	n;

	for(;;){
		n = read(fdslave[1], buf, sizeof buf);
		if(n<=0)
			break;
		write(fdmaster[1], buf, n);
	}
	threadexitsall(nil);
}
extern int rcpid;
extern int noecho;
static int copied;
static
void
copymasterproc(void *v)
{
	uchar buf[4096], *ep, *p0, *p;
	int	n;

	for(;;){
		n = read(fdmaster[1], buf, sizeof buf);
		if(n<=0)
			break;
		p0 = buf;
		ep = buf+n;
		for(p=buf; p<ep; p++){
			if(*p==0x7F){
				if(p!=p0)
					write(fdslave[1], p0, p-p0);
				GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, rcpid);
				p0 = p+1;
			}
		}
		if(p!=p0)
			write(fdslave[1], p0, p-p0);
		copied = 1;
	}
}

int
getintr(int fd)
{
	return 0x7F;
}
extern int _winspawnpg;
int
getpts(int fd[], char *slave)
{
	pipe(fdslave);
	pipe(fdmaster);
	fd[0] = fdslave[0];
	fd[1] = fdmaster[0];
	proccreate(copyslaveproc, nil, 232768);
	proccreate(copymasterproc, nil, 232768);
	strcpy(slave, "faketty");
	_winspawnpg = 1;
	return 0;
}

void
updatewinsize(int row, int col, int dx, int dy)
{
	
}

int
isecho(int fd)
{
	return 1;
}
