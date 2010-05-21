#include <u.h>
#include <mingw32.h>
#include <libc.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fdtab.h"
#include "util.h"

#define ISTYPE(s, t)	(((s)->st_mode&S_IFMT) == t)

static char nullstring[] = "";
static char Enovmem[] = "out of memory";

static Dir*
statconv(struct _stat *s, char *name)
{
	Dir *dir;
	char	*namep;

#ifdef NO
	extern char* GetNameFromID(int);

	uid = GetNameFromID(s->st_uid), NAMELEN);
	gid = GetNameFromID(s->st_gid), NAMELEN);
#endif
	dir = malloc(sizeof(Dir)+strlen(name)+1);
	if(dir == nil){
		werrstr(Enovmem);
		return nil;
	}
	namep = (char*)dir + sizeof(Dir);
	strcpy(namep, name);
	dir->name = namep;
	dir->uid = dir->gid = dir->muid = nullstring;
	dir->qid.type = ISTYPE(s, _S_IFDIR)? QTDIR: QTFILE;
	dir->qid.path = s->st_ino;
	dir->qid.vers = s->st_mtime;
	dir->mode = (dir->qid.type<<24)|(s->st_mode&0777);
	dir->atime = s->st_atime;
	dir->mtime = s->st_mtime;
	dir->length = s->st_size;
	dir->dev = s->st_dev;
	dir->type = ISTYPE(s, _S_IFIFO)? '|': 'M';
	return dir;
}

Dir*
dirfstat(int fd)
{
	Fd	*f;

	f = fdtget(fd);
	if (f==nil)
		return nil;
	switch (f->type) {
	case Fdtypedir:
	case Fdtypefile:
		return dirstat(f->name);
	}
	return nil;
}

Dir*
dirstat(char *f)
{
	Dir *d;
	WCHAR *wf;
	struct _stat sbuf;
	char *p;

	
	f = winpathdup(f);
	if (f==nil)
		return nil;
	wf = winutf2wpath(f);
	if(_wstat(wf, &sbuf) < 0) {
		free(wf);
		free(f);
		return nil;
	}
	free(wf);
	p = strrchr(f, '/');
	if(p && p[1]!='\0')
		p++;
	else
		p = f;
	
	d = statconv(&sbuf, p);
	free(f);
	return d;
}
