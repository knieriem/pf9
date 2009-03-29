#include <u.h>
#include <libc.h>
#include <thread.h>

QLock q;
Rendez r;

void
testproc(void *v)
{
	print("bin da\n");
	rwakeup(&r);
}

void
threadmain(int argc, char **argv)
{
	r.l = &q;
	qlock(&q);
	print("create\n");
	proccreate(testproc, nil, 32768);
	print("warte\n");
	rsleep(&r);

	print("hello, world\n");
}

