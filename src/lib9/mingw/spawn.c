#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#include <libc.h>

#include "fdtab.h"
#include "util.h"

/*
 * derived from inferno's rcsh/Nt.c
 */

/*
 * windows quoting rules - I think
 * Words are seperated by space or tab
 * Words containing a space or tab can be quoted using "
 * 2N backslashes + " ==> N backslashes and end quote
 * 2N+1 backslashes + " ==> N backslashes + literal "
 * N backslashes not followed by " ==> N backslashes
 */
static char *
dblquote(char *cmd, char *s)
{
	int nb;
	char *p;

	for(p=s; *p; p++)
		if(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '"')
			break;

	if(*p == 0){				/* easy case */
		strcpy(cmd, s);
		return cmd+(p-s);
	}

	*cmd++ = '"';
	for(;;) {
		for(nb=0; *s=='\\'; nb++)
			*cmd++ = *s++;

		if(*s == 0) {			/* trailing backslashes -> 2N */
			while(nb-- > 0)
				*cmd++ = '\\';
			break;
		}

		if(*s == '"') {			/* literal quote -> 2N+1 backslashes */
			while(nb-- > 0)
				*cmd++ = '\\';
			*cmd++ = '\\';		/* escape the quote */
		}
		*cmd++ = *s++;
	}

	*cmd++ = '"';
	*cmd = 0;

	return cmd;
}

static char *
proccmd(char **argv)
{
	int i, n;
	char *cmd, *p;

		/* conservatively calculate length of command;
		 * backslash expansion can cause growth in dblquote().
		 */
	for(i=0,n=0; argv[i]; i++) {
		n += 2*strlen(argv[i]);
	}
	n++;
	
	cmd = malloc(n);
	for(i=0,p=cmd; argv[i]; i++) {
		p = dblquote(p, argv[i]);
		*p++ = ' ';
	}
	if(p != cmd)
		p--;
	*p = 0;

	return cmd;
}

static WCHAR *
exportenv(char **e)
{
	int i, j, n;
	WCHAR *buf;

	if(e == 0 || *e == 0)
		return 0;

	winsortenv(e);

	buf = 0;
	n = 0;
	for(i = 0; *e; e++, i++) {
		j = utflen(*e)+1;
		buf = realloc(buf, sizeof(WCHAR)*(n+j));
		winutftowstr(buf+n, *e, j);
//		fprint(2, "export: %*s\n", j, *e);
		n += j;
	}
	/* final null */
	buf = realloc(buf, sizeof(Rune)*(n+1));
	buf[n] = 0;

	return buf;
}


static char**
getshargs(char *shell, char *script, char *shargv[])
{
	char **v;
	char *sh, *p;
	long	shsz;
	int	i, n;

	p = strrchr(shell, '/');
	if (p==nil)
		sh = shell;
	else
		sh = p+1;

	for (n=0; shargv[n]!=nil;)
		n++;
	shsz = strlen(sh)+1;

	v = malloc((n+1+1) * sizeof(char*)  + shsz + strlen(script)+1);
	if (v==nil)
		return nil;

	for (i=1; i<=n; i++)
		v[i+1] = shargv[i];
	
	p = (char*)&v[i+1];
	v[0] = p;
	strcpy(p, sh);

	p += shsz;
	v[1] = p;
	strcpy(p, script);

	return v;
}

int
lookupexe(char path[], char *file, char ***argvp, int search)
{
	int found, full;
	char *shell;
	char *p, *q, buf[MAX_PATH];
	char **argv;

	full = file[0] == '\\' || file[0] == '/' || file[0] == '.' || winisdrvspec(file);
	found = winexecpath(path, file, &shell);

	if(!found && !full && search) {
		p = getenv("PATH");
		for(; p && *p; p = q){
			q = strchr(p, ';');
			if(q)
				*q = 0;
			snprint(buf, sizeof(buf), "%s/%s", p, file);
			if(q)
				*q++ = ';';
			found = winexecpath(path, buf, &shell);
			if(found)
				break;
		}
	}

	if(!found) {
		werrstr("file not found");
		return -1;
	}

	argv = *argvp;
	if (shell!=nil) {
//		fprint(2, "look shell %s %s\n", shell, path);
		argv = getshargs(shell, path, argv);
		if (argv==nil) {
			free(shell);
			werrstr("could not assemble shell command line");
			return -1;
		}
		found = winexecpath(path, shell, nil);
		if(!found) {
			werrstr("interpreter not found: %s", shell);
			free(shell);
			return -1;
		}
		free(shell);
	}

	if (argvp!=nil)
		*argvp = argv;
	return 0;
}

static HANDLE
fdexport(int fd, int i, int tx, int *fused)
{
	Fd *f;
	HANDLE l, r, hr, hw;
	SECURITY_ATTRIBUTES	seca;

	seca = (SECURITY_ATTRIBUTES) {
		.nLength=	sizeof seca,
		.lpSecurityDescriptor=	NULL,
		.bInheritHandle= TRUE,
	};
	f = fdtget(fd);
	if (f!=nil)
		switch (f->type) {
		case Fdtypefile:
		case Fdtypestd:
		case Fdtypecons:
			if (DuplicateHandle(GetCurrentProcess(), f->h,
				GetCurrentProcess(), &r, DUPLICATE_SAME_ACCESS,
				1, DUPLICATE_SAME_ACCESS)) {
//				fprint(2, "duphan %p->%p\n", f->h, r);
				return r;
			}
			break;
		case	Fdtypedevnull:
			fd = -1;
		case Fdtypepipecl:
		case Fdtypepipesrv:
		case Fdtypesock:
			if (!CreatePipe(&hr, &hw, &seca, 0))
				break;
			if (tx) {
				l = hw;
				r = hr;
			} else {
				l = hr;
				r = hw;
			}
   			SetHandleInformation(l, HANDLE_FLAG_INHERIT, 0);
			if (fd!=-1)
				*fused |= 1<<i;
			wincreaterxtxproc(fd, tx? OREAD: OWRITE, l);
			return r;
		}
	if(fd != -1){
		winerror(nil);
		fprint(2, "fdexport: %r\n");
	}
	return INVALID_HANDLE_VALUE;
}

int
winspawn(int fd[3], char *file, char *argv[], int search)
{
	extern char **environ;

	return winspawne(fd, file, argv, environ, search);
}
int _winspawnpg;
int
winspawne(int fd[3], char *file, char *argv[], char *env[], int search)
{
	WCHAR	*wpath, *wcmd, *eb;
	char path[MAX_PATH], *cmd, **margv;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	DWORD cflags;
	int r;
	int used;

	used = 0;
	margv = argv;
	if (lookupexe(path, file, &margv, search)==-1)
		return -1;

	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = fdexport(fd[0], 0, 1, &used);
	si.hStdOutput = fdexport(fd[1], 1, 0, &used);
	si.hStdError = fdexport(fd[2], 2, 0, &used);

	eb = exportenv(env);

	cmd = proccmd(margv);
	if (margv!=argv)
		free(margv);

	wpath = winutf2wpath(path);
	wcmd = winutf2wstr(cmd);
	cflags = CREATE_UNICODE_ENVIRONMENT;
	if (!winhascons())
		cflags |= DETACHED_PROCESS;
	else if(_winspawnpg){
		cflags |= CREATE_NEW_PROCESS_GROUP;
		_winspawnpg = 0;
	}
	r = CreateProcessW(wpath, wcmd, nil, nil, 1/*inherit*/, cflags, eb, nil, &si, &pi);
	free(wpath);
	free(wcmd);

	/* allow child to run */
	Sleep(0);

	free(cmd);
	free(eb);

	CloseHandle(si.hStdInput);
	CloseHandle(si.hStdOutput);
	CloseHandle(si.hStdError);

	if(!r) {
		winerror("CreateProcess");
		return -1;
	}

	CloseHandle(pi.hThread);

	if(winaddchild(pi.dwProcessId, pi.hProcess) == -1)
		return -1;

	if (!(used&1<<0))
		close(fd[0]);

	if(fd[1] != fd[0])
	if (!(used&1<<1))
		close(fd[1]);

	if(fd[2] != fd[1] && fd[2] != fd[0])
	if (!(used&1<<2))
		close(fd[2]);

	return pi.dwProcessId;
}

int
winspawnl(int fd[3], char *cmd, ...)
{
	char **argv, *s;
	int n, pid;
	va_list arg;

	va_start(arg, cmd);
	for(n=0; va_arg(arg, char*) != nil; n++)
		;
	n++;
	va_end(arg);

	argv = malloc(n*sizeof(argv[0]));
	if(argv == nil)
		return -1;

	va_start(arg, cmd);
	for(n=0; (s=va_arg(arg, char*)) != nil; n++)
		argv[n] = s;
	argv[n] = 0;
	va_end(arg);

	pid = winspawn(fd, cmd, argv, 1);
	free(argv);
	return pid;
}
