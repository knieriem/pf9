#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <disk.h>

enum {
	Qdir = 0,
	Qctl,
	Qdata,

	CMpart = 1,
	CMdelpart,

	NPART = 32,
};

typedef
struct Parttab {
	long	path;
	char	*name;
	vlong 	base, length;
	long	mtime;
} Parttab;

Parttab ptab[NPART] = {
	Qdata, "data", 0, 0,
	0,
};

typedef struct Dirtab Dirtab;
struct Dirtab {
	char	*name;
	Qid		qid;
	int		mode;
};
Dirtab dirtab[] = {
	".",	{Qdir,0,QTDIR},		DMDIR|0555,
	"ctl",	{Qctl},				0640,
	"data",	{Qdata},			0640,
};

Cmdtab cmdtab[] = {
	CMpart,	"part",	4,
	CMdelpart,	"delpart",	2,
};

int	nparts = 1;
long	newpath = Qdata;
Disk *disk;

long starttime;
char *owner;

Parttab *
lookuppart(char *name, long path, int ipart)
{
	Parttab	*p, *pfree;
	int	i;

	if (0)	fprint(2, "lookup: n/p/i %s/%ld/%d\n", name?name:"-", path, ipart);

	pfree = &ptab[nparts];
	for (i = 0, p = &ptab[0]; i < nparts; p++) {
		if (p->name == nil) {
			pfree = p;
			continue;
		}
		if (name != nil) {
			if (strcmp(name, p->name) == 0)
				return p;
		}
		else if (ipart != -1) {
			if (ipart == i)
				return p;
		}
		else if (path == p->path)
			return p;
		i++;
	}

	if (nparts == NPART)
		return nil;

	if (name == nil)
		return nil;

	return pfree;
}

void
releasepart(Parttab *p)
{
	if (p->path != Qdata) {
		free(p->name);
		p->name = nil;
	}
	p->path = -1;
	nparts--;
}

int
updatefwin(Parttab *p, Fcall *ic)
{
	long count;

	if (ic->offset >= p->base + p->length)
		return 0;

	if (ic->offset + ic->count > p->length)
		count = p->length - ic->offset;
	else
		count = ic->count;
	
	return count;
}



void
rattach(Req *r)
{
	r->ofcall.qid = dirtab[Qdir].qid;
	r->fid->qid = r->ofcall.qid;
	respond(r, nil);
}

char*
rwalk1(Fid *fid, char *name, Qid *qid)
{
	Parttab	*p;
	int i;

	for (i = 0; i < nelem(dirtab); i++) {
		if (!strcmp(name, dirtab[i].name)) {
			*qid = dirtab[i].qid;
			fid->qid = *qid;
			return 0;
		}
	}

	p = lookuppart(name, -1, -1);
	if (p != nil && p->name != nil) {
			qid->path = p->path;
			qid->vers = 0;
			qid->type = 0;
			fid->qid = *qid;
			return 0;
	}

	return "schnack file does not exist";
}

void
dostat(int n, Dir *dir, int ispath)
{
	if (n >= Qdata) {
		Parttab	*p;
		
		if (ispath)
			p = lookuppart(nil, n, -1);
		else
			p = lookuppart(nil, -1, n - Qdata);

		if (p == nil)
			return;

		dir->length = p->length;
		dir->qid.path = p->path;
		dir->qid.vers = 0;
		dir->qid.type = 0;
		dir->mode = disk->rdonly ? 0440 : 0640;
		dir->name = estrdup9p(p->name);
		dir->mtime = p->mtime;
	} else {
		Dirtab *d;

		d = &dirtab[n];
		dir->qid = d->qid;
		dir->mode = d->mode;
		dir->name = estrdup9p(d->name);
		dir->mtime = starttime;
	}
	dir->atime = starttime;
	dir->uid = estrdup9p(owner);
	dir->gid = estrdup9p(owner);
}

int
dirgen(int n, Dir *dir, void*p)
{
	++n;

	if (n >= nelem(dirtab) + nparts - 1)
		return -1;
	dostat(n, dir, 0);
	return 0;
}

void
rstat(Req *r)
{
	dostat((long)r->fid->qid.path, &r->d, 1);
	respond(r, nil);
}

void
ropen(Req *r)
{
	switch ((long)r->fid->qid.path ){
		break;
	}
	respond(r, nil);
}

void
rread(Req *r)
{
	char buf[256];
	int i, n;
	long	path;
	Parttab *pp;

	path = (long)r->fid->qid.path;
	switch (path) {
	case Qdir:
		dirread9p(r, dirgen, 0);
		break;
	case Qctl:
		n = 0;
		n += snprint(buf+n, sizeof buf-n, "geometry %lld %lld", disk->secs, disk->secsize);
		if (disk->c > 0)
			n += snprint(buf+n, sizeof(buf)-n, " %d %d %d", disk->c, disk->h, disk->s);

		n += snprint(buf+n, sizeof(buf)-n, "\n");
 
		for (i = 0, pp = &ptab[0]; i < nparts; pp++) {
			if (pp->name == nil)
				continue;
			n += snprint(buf+n, sizeof(buf)-n,
					"part %s %lld %lld\n",
					pp->name, pp->base/disk->secsize, (pp->base + pp->length)/disk->secsize);
			i++;
		}
		readbuf(r, buf, n);
		break;
	case Qdata:
	default:
		pp = lookuppart(nil, path, -1);
		if (pp == nil) {
			respond(r, "read error");
			return;
		}
		n = updatefwin(pp, &r->ifcall);

		if (n > 0)
			n = pread(disk->fd, r->ofcall.data, n, pp->base + r->ifcall.offset);
		fprint(2, "o:%lld c:%d n:%d\n", r->ifcall.offset, r->ifcall.count, n);
		r->ofcall.count = n;
	}
	respond(r, nil);
}

void
rwrite(Req *r)
{
	int n;
	Parttab *part;
	long path;
	long now;
	Cmdbuf *cb;
	Cmdtab *ct;

	n = r->ifcall.count;
	r->ofcall.count = 0;
	now = time(nil);
	path = (long)r->fid->qid.path;
	switch (path) {
		char *estr;

	case Qctl:
		estr = "%r";
		cb = parsecmd(r->ifcall.data, n);
		ct = lookupcmd(cb, cmdtab, nelem(cmdtab));
		if (ct == 0) {
		r_c_e:
			respondcmderror(r, cb, estr);
			return;
		}
		switch (ct->index) {
		case CMpart:
			part = lookuppart(cb->f[1], -1, -1);
			if (part == nil) {
				estr = "number of partitions exceeded";
				goto r_c_e;
			}
			part->base = strtoll(cb->f[2], nil, 10);
			part->length = strtoll(cb->f[3], nil, 10) - part->base;
			part->base *=  disk->secsize;
			part->length *=  disk->secsize;

			if (part->base < 0
				|| part->base >= ptab[0].length
				|| part->base + part->length > ptab[0].length) {
				estr = "partition exceeds device boundaries";
				goto r_c_e;
			}

			if (part->name == nil) {
				part->name = strdup(cb->f[1]);
				nparts++;
			}
			part->path = ++newpath;
			part->mtime = now;

			fprint(2, "new: path:%ld n:%d\n", newpath, nparts);
			break;

		case CMdelpart:
			part = lookuppart(cb->f[1], -1, -1);
			if (part && part->name && part->path != Qdata)
				releasepart(part);
			break;
		}
		break;

	case Qdata:
	default:
		if (disk->rdonly) {
			respond(r, "write prohibited");
			return;
		}
			
		part = lookuppart(nil, path, -1);
		if (part == nil) {
			respond(r, "partition resource lost");
			return;
		}
		n = updatefwin(part, &r->ifcall);
		if (n > 0) {
			n = pwrite(disk->wfd, r->ifcall.data, n, part->base + r->ifcall.offset);
			part->mtime = now;
		}
		if (0) fprint(2, "Wo:%lld c:%d n:%d\n", r->ifcall.offset, r->ifcall.count, n);
		r->ofcall.count = n;
		break;
	}
	r->ofcall.count = n;
	respond(r, nil);
}

void
rend(Srv *s)
{
	Parttab *p;
	int	i;

	USED(s);

	for (i = 0, p = &ptab[0]; i < nparts; p++) {
		if (p->name == nil)
			continue;
		releasepart(p);
		i++;
	}
	close(disk->fd);
	if (!disk->rdonly)
		close(disk->wfd);
	free(disk);
}

Srv partsrv = {
	.attach = rattach,
	.walk1 = rwalk1,
	.open = ropen,
	.read = rread,
	.write = rwrite,
	.stat = rstat,
	.end = rend,
};

void
initptab0(char *file, int rdonly)
{
	Parttab *p0;

	p0 = &ptab[0];

	disk = opendisk(file, rdonly, 0);

	if (disk == nil)
		sysfatal("%r");

	p0->length = disk->size;
	p0->mtime = starttime;
	if (disk->rdonly) {
		dirtab[Qdata].mode &= 0444;
	}
}

void
usage(void)
{
	fprint(2, "Usage: %s [-rsD] [-m mountpoint] diskdata\n", argv0);
	threadexitsall("Usage");
}

void
threadmain(int argc, char **argv)
{
	char *srvname = nil; //"pfs";
	char *mntname = "/n/pfs";
	int stdio = 0;
	int rdonly;

	rdonly = 0;
	ARGBEGIN{
	case 'r':
		++rdonly;
		break;
	case 's':
		++stdio;
		break;
	case 'm':
		mntname = ARGF();
		break;
	case 'D':
		++chatty9p;
		break;
	default:
		usage();
	}ARGEND

	if (argc < 1)
		usage();

	starttime = time(0);
	owner = getuser();

	initptab0(*argv, rdonly);

	if (stdio)
		srv(&partsrv);
	else
		threadpostmountsrv(&partsrv, srvname, mntname, 0);
	threadexits(nil);
}

