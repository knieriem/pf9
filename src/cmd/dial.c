#include <u.h>
#include <libc.h>
#include <thread.h>

enum
{
	STACK = 32768,
};

void
usage(void)
{
	fprint(2, "usage: dial [-e] addr\n");
	threadexitsall("usage");
}

int waitforeof;
void
txproc(void *v)
{
	int fd;
	char buf[8192];
	int n;

	fd = (int)v;
	while((n = read(0, buf, sizeof buf)) > 0)
		if(write(fd, buf, n) < 0)
			break;

	if(!waitforeof)
		threadexitsall(nil);
}

void
threadmain(int argc, char **argv)
{
	int fd;
	char buf[8192];
	int n;

	ARGBEGIN{
	case 'e':
		waitforeof = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	if((fd = dial(argv[0], nil, nil, nil)) < 0)
		sysfatal("dial: %r");

	proccreate(txproc, (void*)fd, STACK);

	while((n = read(fd, buf, sizeof buf)) > 0)
		if(write(1, buf, n) < 0)
			break;

	threadexitsall(nil);
}
