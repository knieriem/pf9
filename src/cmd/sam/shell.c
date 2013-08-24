#include "sam.h"
#include "parse.h"

extern	jmp_buf	mainloop;

char	errfile[64];
String	plan9cmd;	/* null terminated */
Buffer	plan9buf;
void	checkerrs(void);

void
setname(File *f)
{
	char buf[1024];
	if(f)
		snprint(buf, sizeof buf, "%.*S", f->name.n, f->name.s);
	else
		buf[0] = 0;
	putenv("samfile", buf);
	putenv("%", buf); // like acme
}

static
void
procpipe(void *v)
{
	int	retcode;
	long l;
	int m;

	io = (int)v;
#ifdef __MINGW32__
	retcode = 0;				/* would crash when calling p9longjmp later */
	{
#else
	/*
	 * It's ok if we get SIGPIPE here
	 */
	if(retcode=!setjmp(mainloop)){	/* assignment = */
#endif
		char *c;
		for(l = 0; l<plan9buf.nc; l+=m){
			m = plan9buf.nc-l;
			if(m>BLOCKSIZE-1)
				m = BLOCKSIZE-1;
			bufread(&plan9buf, l, genbuf, m);
			genbuf[m] = 0;
			c = Strtoc(tmprstr(genbuf, m+1));
			Write(io, c, strlen(c));
			free(c);
		}
	}
	close(io);
	threadexits(retcode? "error" : 0);
}
int
plan9(File *f, int type, String *s, int nest)
{
	int volatile pid;
	int fds[3];
	int retcode;
	int pipe1[2], pipe2[2];

	if(s->s[0]==0 && plan9cmd.s[0]==0)
		error(Enocmd);
	else if(s->s[0])
		Strduplstr(&plan9cmd, s);
	if(downloaded){
		samerr(errfile);
		remove(errfile);
	}
	if(type!='!' && pipe(pipe1)==-1)
		error(Epipe);
	if(type=='|')
		snarf(f, addr.r.p1, addr.r.p2, &plan9buf, 1);

	fds[0] = fds[1] = fds[2] = -1;
	setname(f);
	if(downloaded){	/* also put nasty fd's into errfile */
		fds[2] = create(errfile, 1, 0666L);
		if(fds[2] < 0)
			fds[2] = create("/dev/null", 1, 0666L);

		/* fds[2] now points at err file */
		if(type=='>' || type=='!')
			fds[1] = fds[2];
	}
	if(type != '!') {
		if(type=='<' || type=='|')
			fds[1] = pipe1[1];
		else if(type == '>')
			fds[0] = pipe1[0];
	}
	if(type == '|'){
		if(pipe(pipe2) != -1){
			proccreate(&procpipe, (void*)pipe2[1], 32768);
			fds[0] = pipe2[0];
		}
	}
	if(fds[0]==-1)
		fds[0] = open("/dev/null", 0);	/* so it won't read from terminal */
	if(fds[1]==-1)
		fds[1] = dup(1, -1);
	if(fds[2]==-1)
		fds[2] = dup(2, -1);
	threadwaitchan();
	pid = threadspawnl(fds, SHPATH, SH, "-c", Strtoc(&plan9cmd), nil);
	if(pid == -1)
		error(Efork);
	if(type=='<' || type=='|'){
		int nulls;
		if(downloaded && addr.r.p1 != addr.r.p2)
			outTl(Hsnarflen, addr.r.p2-addr.r.p1);
		snarf(f, addr.r.p1, addr.r.p2, &snarfbuf, 0);
		logdelete(f, addr.r.p1, addr.r.p2);
		io = pipe1[0];
		f->tdot.p1 = -1;
		f->ndot.r.p2 = addr.r.p2+readio(f, &nulls, 0, FALSE);
		f->ndot.r.p1 = addr.r.p2;
		closeio((Posn)-1);
	}else if(type=='>'){
		io = pipe1[1];
		bpipeok = 1;
		writeio(f);
		bpipeok = 0;
		closeio((Posn)-1);
	}
	retcode = waitfor(pid);
	if(type=='|' || type=='<')
		if(retcode!=0)
			warn(Wbadstatus);
	if(downloaded)
		checkerrs();
	if(!nest)
		dprint("!\n");
	return retcode;
}

void
checkerrs(void)
{
	char buf[BLOCKSIZE-10];
	int f, n, nl;
	char *p;
	long l;

	if(statfile(errfile, 0, 0, 0, &l, 0) > 0 && l != 0){
		if((f=open(errfile, 0)) != -1){
			if((n=read(f, buf, sizeof buf-1)) > 0){
				for(nl=0,p=buf; nl<25 && p<&buf[n]; p++)
					if(*p=='\n')
						nl++;
				*p = 0;
				dprint("%s", buf);
				if(p-buf < l-1)
					dprint("(sam: more in %s)\n", errfile);
			}
			close(f);
		}
	}else
		remove(errfile);
}
