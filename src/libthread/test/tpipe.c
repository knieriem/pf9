#include "u.h"
#include "libc.h"
#include "thread.h"

enum
{
	STACK = 8192
};

int	p[2];

void
writethread(void *v)
{
	char buf[200];
	int	n;

	for(;;){
	n = read(0, buf, sizeof buf-1);
	if (n>0) {
		buf[n] = 0;
		print("read0: \"%s\"\n", buf);
		if (buf[0]=='q') {
			close(p[1]);
			threadexits(nil);
		}
	}
	write (p[1], buf, n);
//	n = read(p[1], buf, sizeof buf-1);
//	if (n>0) {
//		buf[n] = 0;
//		print("readb: \"%s\"\n", buf);
//	}

	}
}


void
threadmain(int argc, char **argv)
{
	char buf[200];
	int	n;
	int	fd;

	ARGBEGIN {
	} ARGEND

	fd = create(*argv, OWRITE, 0666);
	print("create wauäm: %d\n",fd );
	fprint(fd, "hoppla\n");
	dup(fd, -1);
	dup(fd, 128);
	close(fd);

	pipe(p);
	proccreate(writethread, nil, STACK);

	print("ns: %s\n", getns());

	for(;;) {
		n = read(p[0], buf, sizeof buf-1);
		if (n<=0)
			break;
		buf[n] = 0;
		print("readp: \"%s\" rand:%08ux\n", buf, truerand());
		write(fd, buf, n);
		write(p[0], "huhu", 5);
	}
	close (p[0]);
	fprint(2, "termin\n");
	threadexitsall(nil);
}
