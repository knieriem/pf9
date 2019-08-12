#include <u.h>
#include <mingwutil.h>
#include <libc.h>
#include "rc.h"
#include "exec.h"
#include "io.h"
#include "fns.h"
#include "getflags.h"

extern char **mkargv(word*);
extern char **mkenv(void);
extern int mapfd(int);

char pathsep = ';';

void
execulimit(void)
{
	setstatus("not implemented");
	fprint(mapfd(2), "ulimit: not implemented\n");
	poplist();
	flush(err);
}

void
execumask(void)
{
	int n, argc;
	char **argv, **oargv, *p;
	char *argv0;

	argv0 = nil;
	setstatus("");
	oargv = mkargv(runq->argv->words);
	argv = oargv+1;
	for(argc=0; argv[argc]; argc++)
		;

	ARGBEGIN{
	default:
	usage:
		fprint(mapfd(2), "usage: umask [mode]\n");
		goto out;
	}ARGEND

	if(argc > 1)
		goto usage;

	if(argc == 1){
		n = strtol(argv[0], &p, 8);
		if(*p != 0 || p == argv[0])
			goto usage;
		umask(n);
		goto out;
	}

	n = umask(0);
	umask(n);
	if(n < 0){
		fprint(mapfd(2), "umask: %r\n");
		goto out;
	}

	fprint(mapfd(1), "umask %03o\n", n);

out:
	free(oargv);
	poplist();
	flush(err);
}

/*
 * Don't cope with non-blocking read.
 */
long
readnb(int fd, char *buf, long cnt)
{
	int n;

	n = read(fd, buf, cnt);
	if(ntrap)
		n = -1;
	return n;
}



int
ForkExecute(char *file, char **argv, int sin, int sout, int serr)
{
	char **env=mkenv();
	int pid;
	int fd[3];

	fd[0] = sin==-1? open("/dev/null", OREAD) : dup(sin, -1);
	fd[1] = sout==-1? open("/dev/null", OWRITE) : dup(sout, -1);
	fd[2] = serr==-1? open("/dev/null", OWRITE) : dup(serr, -1);

if (0){int i;
fprint(2, "forkexec %s", file);
for(i = 0; argv[i]; i++)fprint(2, " %s", argv[i]);
fprint(2, " %d %d %d\n", sin, sout, serr);
}
	pid = winspawne(fd, file, argv, env, 1);
	if (pid<0) {
		close(fd[0]);
		close(fd[1]);
		close(fd[2]);
	}
	efree((char*)env);
	return pid;
}

extern char **environ;
void Updenv(void){
}
void Execute(word *args, word *path)
{
	char **argv=mkargv(args);
	char **env=mkenv();
	char file[1024];
	int nc;
	Updenv();
	for(;path;path=path->next){
		nc=strlen(path->word);
		if(nc<1024){
			strcpy(file, path->word);
			if(file[0]){
				strcat(file, "/");
				nc++;
			}
			if(nc+strlen(argv[1])<1024){
				strcat(file, argv[1]);
				winexecve(file, argv+1, env);
			}
			else werrstr("command name too long");
		}
	}
	rerrstr(file, sizeof file);
	pfmt(err, "%s: %s\n", argv[1], file);
	efree((char *)env);
	efree((char *)argv);
}
int Executable(char *file)
{
	Dir *statbuf;
	int ret;
	char path[_MAX_PATH];
	char *shell;

	if (!winexecpath(path, file, &shell))
		return 0;

	if (shell!=nil) {
		free(shell);
		return 1;
	}

	statbuf = dirstat(path);
	if(statbuf == nil) return 0;
	ret = ((statbuf->mode&0111)!=0 && (statbuf->mode&DMDIR)==0);
	free(statbuf);
	return ret;
}

void
pushpipefd(int fd, int mode)
{
	char name[40];

	pushredir(ROPEN, fd, fd);	/* isn't this a noop? */
	winexportfd(name, sizeof name, fd, mode);
	pushword(name);
}
