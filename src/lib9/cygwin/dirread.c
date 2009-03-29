#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>
#include <sys/stat.h>
#include <dirent.h>

extern int _p9dir(struct stat*, struct stat*, char*, Dir*, char**, char*);

enum {
	STRUCT_ALIGN = sizeof(int),
	MAXDENTSIZE = STRUCT_ALIGN + sizeof(struct dirent),
	NDIRFDS = 32
};


/*
 * Since cygwin has no getdents() or getdirentries() function, we will
 * use the (open|read|close)dir() functions. Since these are operating on
 * a DIR structure, we implement a table associating DIR pointers
 * to file descriptors, and the necessary mgmt functions.
 *
 * On the first call of fd2dir() on an open fd, a corresponding DIR
 * is initialized by calling opendir() in register_dirfd().
 * 
 * When this fd gets closed, unregister_dirfd() is called, resulting
 * in closedir() being called.
  */

typedef
struct Dirfdmap {
	int	fd;
	DIR	*dir;
} Dirfdmap;

static struct Dirfdmap dirfdmap[NDIRFDS];

static int ndirfds = 0;

  
  
/*
 * Derive a directory name from a fd, and open the corresponding DIR
 */
static void	unregister_dirfd(int);
extern void (*atclose_unreg_dirfd)(int);
static Dirfdmap*
register_dirfd(int fd)
{
	char	buf[256];
	char	*name;
	int	oldwd;
	Dirfdmap *p;
	int	i;
	
	if (ndirfds >= NDIRFDS)
		return nil;

	p = dirfdmap;
	for (i = 0; i < ndirfds; p++) {
		if (p->fd == -1)
			break;
		i++;
	}

	if ((oldwd = open(".", O_RDONLY)) < 0)
		return nil;
	
	name = nil;
	if (fchdir(fd) == 0) {
		name = p9getwd(buf, sizeof(buf));
		fchdir(oldwd);
	}
	close(oldwd);

	if (name == nil)
		return nil;

	p->dir = opendir(name);
	if (p->dir == nil)
		return nil;

	p->fd = fd;
	atclose_unreg_dirfd = unregister_dirfd;
	ndirfds++;

	return p;
}

static void
unregister_dirfd(int fd)
{
	Dirfdmap *p;
	int i;
	
	p = dirfdmap;
	for (i = 0; i < ndirfds; p++) {
		if (p->fd == -1)
			continue;
		if (p->fd == fd) {
			p->fd = -1;
			closedir(p->dir);
			ndirfds--;
			break;
		}
		i++;
	}
}

static DIR*
fd2dir(int fd)
{
	Dirfdmap *p;
	int i;
	
	p = dirfdmap;
	for (i = 0; i < ndirfds; p++) {
		if (p->fd == -1)
			continue;
		if (p->fd == fd)
			return p->dir;
		i++;
	}
	
	p = register_dirfd(fd);
	if (p != nil)
		return p->dir;
	
	return nil;
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
	int	nn, r;
	ushort	*reclen;

	dir = fd2dir(fd);
	if (dir == nil)
		return 0;
	
	for (nn = 0; nn + MAXDENTSIZE <= n; nn += *reclen) {
		de = readdir(dir);
		if (de == nil)
			break;

		reclen = (ushort*) &buf[nn];
		nn += STRUCT_ALIGN;
		*reclen = offsetof(struct dirent, d_name) + strlen(de->d_name) + 1;
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
	struct stat st, lst;
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
		memset(&lst, 0, sizeof lst);
		if(de->d_name[0] == 0)
			/* nothing */ {}
		else if(lstat(de->d_name, &lst) < 0)
			de->d_name[0] = 0;
		else{
			st = lst;
			if(S_ISLNK(lst.st_mode))
				stat(de->d_name, &st);
			nstr += _p9dir(&lst, &st, de->d_name, nil, nil, nil);
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
		if(de->d_name[0] != 0 && lstat(de->d_name, &lst) >= 0){
			st = lst;
			if((lst.st_mode&S_IFMT) == S_IFLNK)
				stat(de->d_name, &st);
			_p9dir(&lst, &st, de->d_name, &d[m++], &str, estr);
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
	struct stat st;
	int n;

	*dp = 0;

	if(fstat(fd, &st) < 0)
		return -1;

	if(st.st_blksize < 8192)
		st.st_blksize = 8192;

	buf = malloc(st.st_blksize);
	if(buf == nil)
		return -1;

	n = mygetdents(fd, (void*)buf, st.st_blksize);
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
	struct stat st;

	if(fstat(fd, &st) < 0)
		return -1;

	if(st.st_blksize < 8192)
		st.st_blksize = 8192;

	buf = nil;
	ts = 0;
	for(;;){
		nbuf = realloc(buf, ts+st.st_blksize);
		if(nbuf == nil){
			free(buf);
			return -1;
		}
		buf = nbuf;
		n = mygetdents(fd, (void*)(buf+ts), st.st_blksize);
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
