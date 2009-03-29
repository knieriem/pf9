#include <u.h>
#include <libc.h>
#include <thread.h>

Channel *c;
void
sqrtthread(void *v)
{
	double d;

	for(;;){
		recv(c, &d);
		d = sqrt(d);
		send(c, &d);
	}
}
void
threadmain(int argc, char **argv)
{
	double d;
	float f;
	int	i;
	ulong *l;

	c = chancreate(sizeof(double), 0);
	if (argc>1)
		proccreate(sqrtthread, nil, 32768);
	else
		threadcreate(sqrtthread, nil, 32768);

	d = atof(argv[1]);
		f = d;
	print("d=%.8f (%.8f) 0x%08lux 0x%08lux\n", d, (double)f, *((ulong*)&d), *((ulong*)&f));

	d = 2.;
	for (i=0;i<4;i++) {
		send(c, &d);
		recv(c, &d);
		d *= .5;
		f = d;
		print("d=%.8f (%.8f) 0x%08lux\n", d, (double)f, *((ulong*)&f));
	}

	threadexitsall(nil);
}
