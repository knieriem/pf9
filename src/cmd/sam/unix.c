#include <u.h>
#include <mingw32.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mingwutil.h>

#include "sam.h"

Rune    samname[] = { '~', '~', 's', 'a', 'm', '~', '~', 0 };
 
static Rune l1[] = { '{', '[', '(', '<', 0253, 0};
static Rune l2[] = { '\n', 0};
static Rune l3[] = { '\'', '"', '`', 0};
Rune *left[]= { l1, l2, l3, 0};

static Rune r1[] = {'}', ']', ')', '>', 0273, 0};
static Rune r2[] = {'\n', 0};
static Rune r3[] = {'\'', '"', '`', 0};
Rune *right[]= { r1, r2, r3, 0};

#ifndef SAMTERMNAME
#define SAMTERMNAME "samterm"
#endif
#ifndef TMPDIRNAME
#define TMPDIRNAME "/tmp"
#endif
#ifndef SHNAME
#define SHNAME "sh"
#endif
#ifndef SHPATHNAME
#define SHPATHNAME "/bin/sh"
#endif
#ifndef RXNAME
#define RXNAME "ssh"
#endif
#ifndef RXPATHNAME
#define RXPATHNAME "ssh"
#endif

char	RSAM[] = "sam";
char	SAMTERM[] = SAMTERMNAME;
char	HOME[] = "HOME";
char	TMPDIR[] = TMPDIRNAME;
char	SH[] = SHNAME;
char	SHPATH[] = SHPATHNAME;
char	RX[] = RXNAME;
char	RXPATH[] = RXPATHNAME;


void
dprint(char *z, ...)
{
	char buf[BLOCKSIZE];
	va_list arg;

	va_start(arg, z);
	vseprint(buf, &buf[BLOCKSIZE], z, arg);
	va_end(arg);
	termwrite(buf);
}

void
print_ss(char *s, String *a, String *b)
{
	dprint("?warning: %s: `%.*S' and `%.*S'\n", s, a->n, a->s, b->n, b->s);
}

void
print_s(char *s, String *a)
{
	dprint("?warning: %s `%.*S'\n", s, a->n, a->s);
}

int     
statfile(char *name, ulong *dev, uvlong *id, long *time, long *length, long *appendonly) 
{
        Dir *d;

        d = dirstat(name);
	if (d==nil)
                return -1;
        if (dev)
                *dev = d->dev;
        if (id) 
                *id = d->qid.path;
        if (time)
                *time = d->mtime;
        if (length)
                *length = d->length;
        if(appendonly)
                *appendonly = 0;
	free(d);
        return 1;
}

int
statfd(int fd, ulong *dev, uvlong *id, long *time, long *length, long *appendonly)
{
        Dir *d;

        d = dirfstat(fd);
	if (d==nil)
                return -1;
        if (dev)
                *dev = d->dev;
        if (id) 
                *id = d->qid.path;
        if (time)
                *time = d->mtime;
        if (length)
                *length = d->length;
        if(appendonly)
                *appendonly = 0;
	free(d);
        return 1;
}

void
hup(int sig)
{
        panicking = 1; /* ??? */
        rescue();
        exit(1);
}

int
mynotify(void(*f)(void *, char *))
{
//        signal(SIGINT, SIG_IGN);
//        signal(SIGPIPE, SIG_IGN);  /* XXX - bpipeok? */
//        signal(SIGHUP, hup);
        return 1;
}

void
notifyf(void *a, char *b)       /* never called; hup is instead */
{
}

static int
temp_file(char *buf, int bufsize)
{
        char *tmp;
        int n, fd;

        tmp = getenv("TMPDIR");
        if (!tmp)
                tmp = TMPDIR;

        n = snprint(buf, bufsize, "%s/sam.%d.XXXXXXX", tmp, 1000 /*getuid()*/);		/* FIXME */
        if (bufsize <= n)
                return -1;
        if ((fd = opentemp(buf)) < 0)
                return -1;
//	if (fcntl(fd, F_SETFD, fcntl(fd,F_GETFD,0) | FD_CLOEXEC) < 0)
//                return -1;
        return fd;
}

int
tempdisk(void)
{
        char buf[4096];
        int fd = temp_file(buf, sizeof buf);
        if (fd >= 0)
                remove(buf);
        return fd; 
}

#undef waitfor
int     
samwaitfor(int pid)
{
	int r;
	Waitmsg *w;

	w = p9waitfor(pid);
	if(w == nil)
		return -1;
	r = atoi(w->msg);
	free(w);
	return r;
}

void
samerr(char *buf)
{
	sprint(buf, "%s/sam.%s.err", TMPDIR, getuser());
}

void*
emalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p == 0)
		panic("malloc fails");
	memset(p, 0, n);
	return p;
}

void*
erealloc(void *p, ulong n)
{
	p = realloc(p, n);
	if(p == 0)
		panic("realloc fails");
	return p;
}


int
ForkExecute(char *file, char **argv, int sin, int sout, int serr)
{
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
	pid = winspawn(fd, file, argv, 1);
	if (pid<0) {
		close(fd[0]);
		close(fd[1]);
		close(fd[2]);
	}
	return pid;
}