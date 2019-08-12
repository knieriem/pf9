/*
 * Some support for functions that are not part of MingW
 *
 * From Inferno utilities:
 * 	sbrk()
 *
 */

#include <u.h>
#include <mingw32.h>
#include <libc.h>

#include "util.h"
#include "fdtab.h"

#define	utf2wpath winutf2wpath
#define	sl2bsl	winsl2bsl
#define	bsl2sl	winbsl2sl
#define	isdrvspec	winisdrvspec

#define	Chunk	(1*1024*1024)

void*
winsbrk(ulong size)
{
	void *v;
	static int chunk;
	static uchar *brk;

	if(chunk < size) {
		chunk = Chunk;
		if(chunk < size)
			chunk = Chunk + size;
		brk = VirtualAlloc(NULL, chunk, MEM_COMMIT, PAGE_EXECUTE_READWRITE); 	
		if(brk == 0)
			return (void*)-1;
	}
	v = brk;
	chunk -= size;
	brk += size;
	return v;
}


int
fchdir(int fd)
{
	Fd	*f;

	f = fdtget(fd);
	if (f==nil || f->type!=Fdtypedir)
		return -1;
//	fprint(2, "chdir: %s\n", f->name);
	return chdir(f->name);
}

int
fchmod(int fildes, mode_t mode)
{
	werrstr("not implemented");
	return -1;
}

void
winerror(char *pfx)
{
	int e, r;
	char buf[100], *p, *q;

	e = GetLastError();
	
	r = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
		0, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, sizeof(buf), 0);

	if(r == 0)
		snprint(buf, sizeof buf, "windows error %d", e);

	for(p=q=buf; *p; p++) {
		if(*p == '\r')
			continue;
		if(*p == '\n')
			*q++ = ' ';
		else
			*q++ = *p;
	}
	*q = 0;

	if (pfx!=nil)
		werrstr("%s: %s", pfx, buf);
	else
		werrstr("%s", buf);
}

int
winovresult(int ret, HANDLE h, OVERLAPPED *ov, DWORD *np, int evclose)
{
	DWORD e, e0, n;

	n = 0;
	e = e0 = 0;
	if (ret==0) {
		e = GetLastError();
		e0 = e;
		if (e==ERROR_IO_PENDING) {
			e = 0;

			n = np==nil? 0: *np;
			if (!GetOverlappedResult(h, ov, &n, 1 /* wait */))
				e = GetLastError();
			if (np!=nil)
				*np = n;
		}
	}

	if (evclose)
		CloseHandle(ov->hEvent);
	else
		ResetEvent(ov->hEvent);
	return e;
}

void
sl2bsl(WCHAR *r)
{
	for (;*r != '\0'; r++)
		if (*r=='/')
			*r = '\\';
}
void
bsl2sl(char *s)
{
	for (; *s!='\0'; s++)
		if (*s=='\\')
			*s = '/';
}

LPWSTR
utf2wpath(char *s)
{
	WCHAR *w;

	w = winutf2wstr(s);
	if (w==nil)
		return nil;
	winreplacews(w, 1);
	sl2bsl(w);
	return w;
}

static int wschar, wsesc;
void
winreplacews(WCHAR *w, int rev)
{
	char *e, *ep;

	if(wschar<0)
		return;
	if(wschar==0){
		e = getenv("P9FILEWSCHAR");
		if(e==nil){
			wschar = -1;
			return;
		}else{
			wschar = strtol(e, &ep, 16);
			if(*ep != '\0')
				wsesc =  strtol(ep, &ep, 16);
			else
				wsesc = 0xFFFD;
			free(e);
		}
	}
	if(rev){
		for( ;*w; w++)
			if(*w==wschar)
				*w = ' ';
			else if(*w==wsesc)
				*w = wschar;
			else if(*w==0xFFFD)
				*w = wsesc;
	}else{
		for( ; *w; w++)
			if(*w==wsesc)
				*w = 0xFFFD;
			else if(*w==wschar)
				*w = wsesc;
			else if(*w==' ')
				*w = wschar;
	}
}

int
isdrvspec(char *s)
{
	if (s[0]>='a'&&s[0]<='z' || s[0]>='A'&&s[0]<='Z')
	if (s[1]==':' || s[1]=='-')
		return 1;
	return 0;
}
int
iswindrvspec(char *s)
{
	if (s[0]>='a'&&s[0]<='z' || s[0]>='A'&&s[0]<='Z')
	if (s[1]==':')
		return 1;
	return 0;
}
char*
winpathdup(char *s)
{
	DWORD	ts;
	char tbuf[256], *tmp;
	char *d;
	long	l, ds;
	int	skip;
	int	reserve;

	reserve = 2;			/* reserve (for e.g. chdir, and below) */

	tmp = nil;
	if (!strncmp(s, "/tmp", 4)) {
		ts = GetTempPath(sizeof tbuf, tbuf);
		if (ts>0 && ts<sizeof tbuf) {
			switch (tbuf[ts-1]){
			case '\\':
			case '/':
				--ts;
				tbuf[ts] = '\0';
			}
			reserve += ts-4;

			tmp = &tbuf[0];
			/*
			 * Fix paths like Z:/cygdrive/c/temp
			 * returned by GetTempPath
			 * actuallymeaning c:\temp
			 */
			if (iswindrvspec(tbuf))
			if (!strncmp(tbuf+3, "cygdrive", 8)) {
				tmp = &tbuf[11];
				tmp[0] = tmp[1];	/* drive letter */
				tmp[1] = ':';
			}
		}
	} else if (*s=='/' && isdrvspec(s+1))	/* remove / before drive letter */
		++s;

	ds = strlen(s)+1+reserve;
	d = malloc(ds);
	if (d==nil)
		return nil;

	skip = 1;
	if (tmp!=nil)
		snprint(d, ds, "%s%s", tmp, s+4);
	else if (isdrvspec(s)) {
		d[0]=s[0];
		d[1]=':';
		if (s[2]!='/'&&s[2]!='\\') {	/* insert a slash, don't allow names like e:file */
			d[2] = '/';
			strcpy(d+3, s+2);
		} else
			strcpy(d+2, s+2);
		skip = 3;
	} else
		strcpy(d, s);
	bsl2sl(d);
	l = strlen(d);
	while (l>skip && d[l-1]=='/') {
		--l;
		d[l] = '\0';
	}
	return d;
}

static
int
pureasc(char *s)
{
	uchar *p;

	p = (uchar*)s;
	for(; *p!='\0'; p++)
		if (*p >= Runeself)
			return 0;
	return 1;
}

HANDLE
wincreatefile(char *name, int desiacc, int share, int creatdisp, int flagsattr)
{
	WCHAR *wname;
	HANDLE h;

	if (pureasc(name))
		h = CreateFileA(name, desiacc, share, nil, creatdisp, flagsattr, nil);
	else {
		wname = utf2wpath(name);
		h = CreateFileW(wname, desiacc, share, nil, creatdisp, flagsattr, nil);
		free(wname);
	}
	return h;
}

BOOL
wincreatedir(char *name)
{
	WCHAR *wname;
	int	r;

	if (pureasc(name))
		r = CreateDirectoryA(name, nil);
	else {
		wname = utf2wpath(name);
		r = CreateDirectoryW(wname, nil);
		free(wname);
	}
	return r;
}

static
HANDLE
createnamedpipe(char *name, int omode, int pmode, int maxinst, int outsz, int insz, int timeout)
{
	HANDLE h;
	WCHAR *wname;

	if (pureasc(name))
		h = CreateNamedPipeA(name, omode, pmode, maxinst, outsz, insz, timeout, nil);
	else {
		wname = utf2wpath(name);
		h = CreateNamedPipeW(wname, omode, pmode, maxinst, outsz, insz, timeout, nil);
		free(wname);
	}
	return h;
}

int
wincreatenamedpipe(HANDLE *h, char *name, int mode, int nclients)
{
	int acc;

	if(nclients==0)
		nclients = PIPE_UNLIMITED_INSTANCES;

	switch(mode){
	case OREAD:	acc = PIPE_ACCESS_INBOUND; break;
	case OWRITE:	acc = PIPE_ACCESS_OUTBOUND; break;
	case ORDWR:
	default:	acc = PIPE_ACCESS_DUPLEX;
	}

	assert(h != nil);
	*h = createnamedpipe(name, 
		acc | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		nclients,
		4096,	// output buffer size 
		4096,	// input buffer size 
		0);		// client time-out
	
	if(*h==INVALID_HANDLE_VALUE){
		winerror("CreateNamedPipe");
		return -1;
	}
	return 0;
}

int
winconnectpipe(HANDLE h, int needclient)
{
	OVERLAPPED ov;

	memset(&ov, 0, sizeof ov);
	if(wincreatevent(&ov.hEvent, "connectpipe", 1 /* manual reset */, 0 /* initial */)==-1)
		return -1;

	switch(winovresult(ConnectNamedPipe(h, &ov), h, &ov, nil, 1)){
	case 0:
		if(needclient){
			werrstr("pipe: connect is expected to fail");
			break;
		}
		/* fall through */
	case ERROR_PIPE_CONNECTED:
		return 0;
	default:
		winerror("ConnectNamedPipe");
	}

	CloseHandle(h);
	return -1;
}

int
wincreatevent(HANDLE *hp, char *who, int manrst, int ini)
{
	static int nevents;
	HANDLE h;
	char buf[64];

	snprint(buf, sizeof buf, "lib9-%x-%d-%s", GetCurrentProcessId(), nevents++, who);
	h = CreateEventA(nil, manrst, ini, buf);
	if(h==INVALID_HANDLE_VALUE){
		winerror("");
		return -1;
	}
	*hp = h;
	return 0;
}
