#include "rc.h"
#include "getflags.h"
#include "exec.h"
#include "io.h"
#include "fns.h"

extern	void	pushpipefd(int, int);

int havefork = 0;

static char **
rcargv(char *s)
{
	int argc;
	char **argv;
	word *p;

	p = vlook("*")->val;
	argv = malloc((count(p)+6)*sizeof(char*));
	argc = 0;
	argv[argc++] = argv0;
	if(flag['e'])
		argv[argc++] = "-Se";
	else
		argv[argc++] = "-S";
	argv[argc++] = "-c";
	argv[argc++] = s;
	for(p = vlook("*")->val; p; p = p->next)
		argv[argc++] = p->word;
	argv[argc] = 0;
	return argv;
}

void
Xasync(void)
{
	uint pid;
	char buf[20], **argv;

	Updenv();

//	fprint(2, "Xasync\n");
	argv = rcargv(runq->code[runq->pc].s);
	pid = ForkExecute(argv0, argv, -1, 1, 2);
	free(argv);

	if(pid == 0) {
		Xerror("proc failed");
		return;
	}

	addwaitpid(pid);
	runq->pc++;
	sprint(buf, "%d", pid);
	setvar("apid", newword(buf, (word *)0));
}

void
Xbackq(void)
{
	struct thread *p = runq;
	int pc = p->pc;
	char wd[8193], **argv;
	int c;
	char *s, *ewd=&wd[8192], *stop;
	struct io *f;
	var *ifs = vlook("ifs");
	word *v, *nextv;
	int pfd[2];
	int pid;

//	fprint(2, "Xbackq\n");
	stop = ifs->val?ifs->val->word:"";
	if(pipe(pfd)<0){
		Xerror("can't make pipe");
		return;
	}

	Updenv();

	argv = rcargv(runq->code[runq->pc].s);
	pid = ForkExecute(argv0, argv, 0, pfd[1], 2);
	free(argv);

	close(pfd[1]);

	if(pid == 0) {
		Xerror("proc failed");
		close(pfd[0]);
		return;
	}

	addwaitpid(pid);
	f = openfd(pfd[0]);
	s = wd;
	v = 0;
	while((c=rchr(f))!=EOF){
		if(strchr(stop, c) || s==ewd){
			if(s!=wd){
				*s='\0';
				v=newword(wd, v);
				s=wd;
			}
		}
		else *s++=c;
	}
	if(s!=wd){
		*s='\0';
		v=newword(wd, v);
	}
	closeio(f);
	Waitfor(pid, 1);
	/* v points to reversed arglist -- reverse it onto argv */
	while(v){
		nextv=v->next;
		v->next=p->argv->words;
		p->argv->words=v;
		v=nextv;
	}
	p->pc = pc+1;
}

void
Xpipe(void)
{
	thread *p=runq;
	int pc=p->pc, pid;
	int rfd=p->code[pc+1].i;
	int pfd[2];
	char **argv;

//	fprint(2, "Xpipe\n");
	if(pipe(pfd)<0){
		Xerror1("can't get pipe");
		return;
	}

	Updenv();

	argv = rcargv(runq->code[pc+2].s);
	pid = ForkExecute(argv0, argv, mapfd(0), pfd[1], mapfd(2));
	free(argv);
	close(pfd[1]);

	if(pid == 0) {
		Xerror("proc failed");
		close(pfd[0]);
		return;
	}

	addwaitpid(pid);
	start(p->code, pc+4, runq->local);
	pushredir(ROPEN, pfd[0], rfd);
	p->pc=p->code[pc+3].i;
	p->pid=pid;
}


typedef
struct Pipefds {
	int	side;
	int	main;
	int	redir;
} Pipefds;

static
int
setuppipe(Pipefds *p, int redirfd)
{
	int pfd[2];

	if(pipe(pfd)<0){
		Xerror("can't get pipe");
		return -1;
	}
	p->side = pfd[redirfd==1? PWR: PRD];
	p->main = pfd[redirfd==1? PRD: PWR];
	p->redir = redirfd;
	return 0;
}

void
Xpipefd(void)
{
	struct thread *p = runq;
	int pc = p->pc, pid;
	Pipefds pfds[2], *r, *w;
	char **argv;

	r = &pfds[0];
	w = &pfds[1];
	switch(p->code[pc].i){
	case READ:
		w = nil;
		break;
	case WRITE:
		r = nil;
	}
	if(r && setuppipe(r, 1)<0)
		return;
	if(w && setuppipe(w, 0)<0)
		return;

	argv = rcargv(p->code[p->pc+1].s);
	pid = ForkExecute(argv0, argv, w? w->side: 0, r? r->side: 1, 2);
	free(argv);

	if(pid == 0) {
		Xerror("proc failed");
		if(r)
			close(r->main);
		if(w)
			close(w->main);
		return;
	}

	addwaitpid(pid);
	if(w){
		close(w->side);
		pushpipefd(w->main, OWRITE);
	}
	if(r){
		close(r->side);
		pushpipefd(r->main, OREAD);
	}
	p->pc += 2;
}

void
Xsubshell(void)
{
	char **argv;
	int pid;

//	fprint(2, "Xsubshell\n");
	Updenv();

	argv = rcargv(runq->code[runq->pc].s);
	pid = ForkExecute(argv0, argv, 0, 1, 2);
	free(argv);

	if(pid < 0) {
		Xerror("proc failed");
		return;
	}

	addwaitpid(pid);
	Waitfor(pid, 1);
	runq->pc++;
}

/*
 *  start a process running the cmd on the stack and return its pid.
 */
int
execforkexec(void)
{
	char **argv;
	char file[1024];
	int nc;
	word *path;
	int pid;

	if(runq->argv->words==0)
		return -1;
	argv = mkargv(runq->argv->words);

	for(path = searchpath(runq->argv->words->word);path;path = path->next){
		nc = strlen(path->word);
		if(nc<sizeof(file)){
			strcpy(file, path->word);
			if(file[0]){
				strcat(file, "/");
				nc++;
			}
			if(nc+strlen(argv[1])<sizeof(file)){
				strcat(file, argv[1]);
				pid = ForkExecute(file, argv+1, mapfd(0), mapfd(1), mapfd(2));
				if(pid >= 0){
					free(argv);
					addwaitpid(pid);
					return pid;
				}
			}
		}
	}
	free(argv);
	return -1;
}
