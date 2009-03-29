#include <u.h>
#include <libc.h>
#include <thread.h>

char	output[4096];
void	add(char*, ...);
void	error(char*);
void	notifyf(void*, char*);

void
threadmain(int argc, char *argv[])
{
	int i;
	Channel *cwait;
	Waitmsg *w;
	vlong t0, t1;
	long l;
	char *p;
	char err[ERRMAX];

	if(argc <= 1){
		fprint(2, "usage: time command\n");
		threadexitsall("usage");
	}

	t0 = nsec();
	cwait = threadwaitchan();
	if(threadspawn((int[3]){dup(0, -1), dup(1, -1), dup(2, -1)}, argv[1], &argv[1])==-1)
		error("spawn");

	notify(notifyf);

    loop:
	w = recvp(cwait);
	t1 = nsec();
	if(w == nil){
		rerrstr(err, sizeof err);
		if(strcmp(err, "interrupted") == 0)
			goto loop;
		error("wait");
	}
	l = w->time[0];
	add("%ld.%.2ldu", l/1000, (l%1000)/10);
	l = w->time[1];
	add("%ld.%.2lds", l/1000, (l%1000)/10);
	l = (t1-t0)/1000000;
	add("%ld.%.2ldr", l/1000, (l%1000)/10);
	add("\t");
	for(i=1; i<argc; i++){
		add("%s", argv[i], 0);
		if(i>4){
			add("...");
			break;
		}
	}
	if(w->msg[0]){
		p = utfrune(w->msg, ':');
		if(p && p[1])
			p++;
		else
			p = w->msg;
		add(" # status=%s", p);
	}
	fprint(2, "%s\n", output);
	threadexitsall(w->msg);
}

void
add(char *a, ...)
{
	static int beenhere=0;
	va_list arg;

	if(beenhere)
		strcat(output, " ");
	va_start(arg, a);
	vseprint(output+strlen(output), output+sizeof(output), a, arg);
	va_end(arg);
	beenhere++;
}

void
error(char *s)
{

	fprint(2, "time: %s: %r\n", s);
	threadexitsall(s);
}

void
notifyf(void *a, char *s)
{
	USED(a);
	if(strcmp(s, "interrupt") == 0)
		noted(NCONT);
	noted(NDFLT);
}
