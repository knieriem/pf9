#include <u.h>
#include <libc.h>
#include <thread.h>

int	p[2];
#include <stdio.h>
void
threadmain(int argc, char **argv)
{
	Waitmsg	*w;
	Channel	*cwait;
	char buf[200], *name;
	int	n;
	int	fd, fds[3];
	int	pid;

	ARGBEGIN {
	} ARGEND

	name = *argv == nil? "öä" : *argv;

	cwait = threadwaitchan();
	fd = create(name, OWRITE, 0666);
	print("create wauäm: %d\n",fd );
	fprint(fd, "hoppla\n");
	dup(fd, -1);
	dup(fd, 128);
	close(fd);

	print("ns: %s\n", getns());

	pipe(p);
	fds[0] = dup(0, -1);
	fds[1] = p[1];
	fds[2] = dup(2, -1);
	fdtprint();
	pid = threadspawn(fds, name, argv);
	if (pid == -1)
		fprint(2, "spawnl: %r\n");
	else
		fprint(2, "pid: %d\n", pid);
	fdtprint();
	for(;pid!=-1;) {
		n = read(p[0], buf, sizeof buf-1);
		if (n<=0)
			break;
		buf[n] = 0;
		fprint(2, "\n");
		fdtprint();
		print("readp: \"%s\" rand:%08ux\n", buf, truerand());
		write(fd, buf, n);
	}
	w = nil;
	if (pid!=-1)
		w = recvp(cwait);
	if (w!=nil) {
		if (w->msg[0]=='\0')
			fprint(2, "exit ok\n");
		else
			fprint(2, "exit err: %s\n", w->msg);
		fprint(2, "wmsg u:%d s:%d u+s:%d\n", w->time[0], w->time[1], w->time[2]);
		free(w);
	}
	close (p[0]);
	fprint(2, "termin\n");
	threadexitsall(nil);
}
