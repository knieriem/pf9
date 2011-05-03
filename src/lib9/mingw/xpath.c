#include <u.h>
#include <mingw32.h>
#include <libc.h>

#include "util.h"

static int
setpath(char *path, char *file)
{
	char *p, *last, tmpbuf[MAX_PATH+1], *tmp;
	int n;

	tmp = tmpbuf;

	if(strlen(file) >= MAX_PATH){
		werrstr("file name too long");
		return -1;
	}
	strcpy(tmp, file);

	for(p=tmp; *p; p++) {
		if(*p == '/')
			*p = '\\';
	}

	if(tmp[0] == '\\' && winisdrvspec(&tmp[1])){
		++tmp;
		tmp[1] = ':';	/* make sure its not a '-' */
		goto drvspec;
	}
	if(tmp[0] != 0 && tmp[1] == ':') {
	drvspec:
		if(tmp[2] == 0) {
			tmp[2] = '\\';
			tmp[3] = 0;
		} else if(tmp[2] != '\\') {
			/* don't allow c:foo - only c:\foo */
			werrstr("illegal file name");
			return -1;
		}
	}

	path[0] = 0;
	n = GetFullPathName(tmp, MAX_PATH, path, &last);
	if(n >= MAX_PATH) {
		werrstr("file name too long");
		return -1;
	}
	if(n == 0 && tmp[0] == '\\' && tmp[1] == '\\' && tmp[2] != 0) {
		strcpy(path, tmp);
		return -1;
	}

	if(n == 0) {
		werrstr("bad file name");
		return -1;
	}

	for(p=path; *p; p++) {
		if(*(uchar*)p < 32 || *p == '*' || *p == '?') {
			werrstr("file not found");
			return -1;
		}
	}

	/* get rid of trailling \ */
	if(path[n-1] == '\\') {
		if(n <= 2) {
			werrstr("illegal file name");
			return -1;
		}
		path[n-1] = 0;
		n--;
	}

	if(path[1] == ':' && path[2] == 0) {
		path[2] = '\\';
		path[3] = '.';
		path[4] = 0;
		return -1;
	}

	if(path[0] != '\\' || path[1] != '\\')
		return 0;

	for(p=path+2,n=0; *p; p++)
		if(*p == '\\')
			n++;
	if(n == 0)
		return -1;
	if(n == 1)
		return -1;
	return 0;
}

static
int
fexists(char *name)
{
	DWORD attr;
	WCHAR *wname;

	wname = winutf2wpath(name);
	attr = GetFileAttributesW(wname);
	free(wname);
	return attr != INVALID_FILE_ATTRIBUTES;
}

int
winexecpath(char path[], char *file, char **shell)
{
	char buf[80];
	int fd;
	int n;

	if (shell)
		*shell = nil;
	if(setpath(path, file) < 0)
		return 0;

	n = strlen(path)-4;
	if(path[n] == '.')
	if(fexists(path))
		return 1;
	strncat(path, ".exe", MAX_PATH);
	path[MAX_PATH-1] = 0;
//	fprint(2, "look exe %s\n", path);
	if(fexists(path))
		return 1;
	path[n+4] = '\0';

	strncat(path, ".com", MAX_PATH);
	path[MAX_PATH-1] = 0;
//	fprint(2, "look com %s\n", path);
	if(fexists(path))
		return 1;
	path[n+4] = '\0';

	if (shell==nil)
		return 0;

	fd = open(file, OREAD);
	if (fd == -1)
		return 0;

	n = read(fd, buf, sizeof(buf));
	close(fd);
	if(n<1)
		return 0;
	buf[n-1] = '\0';
	if (n > 4 && buf[0] == '#' && buf[1] == '!') {
		char *p, *ep, *sl;
				
		p = buf+2;
		while(*p==' ')
			++p;
		for (ep=p; *ep!='\0'; ep++)
			switch(*ep){
			case ' ':
			case '\r':
			case '\n':
				*ep = '\0';
				goto out;
			}
	out:
		sl = strrchr(p, '/');
		if (sl != nil
			&& strncmp(sl, "/rc", 3)==0
			&& strcmp(p, "/usr/local/plan9/bin/rc")==0)
			*shell = unsharp("#9/mingw/bin/rc");
		else
			*shell = strdup(p);
		return 1;
	}
	return 0;
}
