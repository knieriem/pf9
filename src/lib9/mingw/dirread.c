#include <u.h>
#include <mingw32.h>
#include <libc.h>
#include <sys/stat.h>
#include <dirent.h>

#include "util.h"
#include "fdtab.h"
#define	readdir	_wreaddir
#define	dirent	_wdirent
#define	DIR		_WDIR
#define	stat		_stat

extern int _p9dir(struct stat*, WCHAR*, Dir*, char**, char*);

enum {
	STRUCT_ALIGN = sizeof(int),
	MAXDENTSIZE = STRUCT_ALIGN + sizeof(struct dirent),
	NDIRFDS = 32,
	BLKSIZE = 8192,
};


static DIR*
fd2dir(int fd)
{
	Fd	*f;

	f = fdtget(fd);
	if (f==nil || f->type!=Fdtypedir)
		return nil;

	return f->dir;	
}

static int
d_reclen(char *p, int *reclen)
{
	*reclen = *(ushort *)p;
	return STRUCT_ALIGN;
}

static int
mygetdents(int fd, uchar *buf, int n)
{
	DIR	*dir;
	struct dirent	*de;
	int	nn, r, len;
	ushort	*reclen;

	dir = fd2dir(fd);
	if (dir == nil)
		return 0;
	
	for (nn = 0; nn + MAXDENTSIZE <= n; nn += *reclen) {
		de = _wreaddir(dir);
		if (de == nil)
			break;

		reclen = (ushort*) &buf[nn];
		nn += STRUCT_ALIGN;
		for (len=0; de->d_name[len]!='\0'; len++);
		*reclen = offsetof(struct dirent, d_name) + 2*(len+1);
		r = *reclen % STRUCT_ALIGN;
		if (r)
			*reclen += STRUCT_ALIGN - r;

		memcpy(&buf[nn], de, *reclen);
	}
	return nn;
}


static int
countde(char *p, int n)
{
	char *e;
	int m;
	struct dirent *de;
	int reclen;

	e = p+n;
	m = 0;
	while(p + sizeof(ushort) <= e){
		p += d_reclen(p, &reclen);
		de = (struct dirent*)p;
		if(p+reclen > e)
			break;
		if(de->d_name[0]=='.' && de->d_name[1]==0)
			de->d_name[0] = 0;
		else if(de->d_name[0]=='.' && de->d_name[1]=='.' && de->d_name[2]==0)
			de->d_name[0] = 0;
		m++;
		p += reclen;
	}
	return m;
}

static int
dirpackage(int fd, char *buf, int n, Dir **dp)
{
	int oldwd;
	char *p, *str, *estr;
	int i, nstr, m;
	struct dirent *de;
	int reclen;
	struct stat st;
	Dir *d;

	n = countde(buf, n);
	if(n <= 0)
		return n;

	if((oldwd = open(".", O_RDONLY)) < 0)
		return -1;
	if(fchdir(fd) < 0)
		return -1;
		
	p = buf;
	nstr = 0;

	for(i=0; i<n; i++){
		p += d_reclen(p, &reclen);
		de = (struct dirent*)p;
		memset(&st, 0, sizeof st);
		if(de->d_name[0] == 0)
			/* nothing */ {}
		else if(_wstat(de->d_name, &st) < 0)
			de->d_name[0] = 0;
		else{
			nstr += _p9dir(&st, de->d_name, nil, nil, nil);
		}
		p += reclen;
	}

	d = malloc(sizeof(Dir)*n+nstr);
	if(d == nil){
		fchdir(oldwd);
		close(oldwd);
		return -1;
	}
	str = (char*)&d[n];
	estr = str+nstr;

	p = buf;
	m = 0;
	for(i=0; i<n; i++){
		p += d_reclen(p, &reclen);
		de = (struct dirent*)p;
		if(de->d_name[0] != 0 && _wstat(de->d_name, &st) >= 0){
			winreplacews(de->d_name, 0);
			_p9dir(&st, de->d_name, &d[m++], &str, estr);
		}
		p += reclen;
	}

	fchdir(oldwd);
	close(oldwd);
	*dp = d;
	return m;
}

long
dirread(int fd, Dir **dp)
{
	char *buf;
	int n;

	*dp = 0;

	buf = malloc(BLKSIZE);
	if(buf == nil)
		return -1;

	n = mygetdents(fd, (void*)buf, BLKSIZE);
	if(n < 0){
		free(buf);
		return -1;
	}
	n = dirpackage(fd, buf, n, dp);
	free(buf);
	return n;
}


long
dirreadall(int fd, Dir **d)
{
	uchar *buf, *nbuf;
	long n, ts;

	buf = nil;
	ts = 0;
	for(;;){
		nbuf = realloc(buf, ts+BLKSIZE);
		if(nbuf == nil){
			free(buf);
			return -1;
		}
		buf = nbuf;
		n = mygetdents(fd, (void*)(buf+ts), BLKSIZE);
		if(n <= 0)
			break;
		ts += n;
	}
	if(ts >= 0)
		ts = dirpackage(fd, (char*)buf, ts, d);
	free(buf);
	if(ts == 0 && n < 0)
		return -1;
	return ts;
}
