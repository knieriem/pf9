#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#include <libc.h>

#undef	accept
#undef	announce
#undef	dial
#undef	setnetmtpt
#undef	hangup
#undef	listen
#undef	netmkaddr
#undef	reject


#undef unix
#define unix xunix

#include "fdtab.h"
#include "util.h"

char pipepfx[] = "//./pipe";
char*
unix2winpipe(char *unix)
{
	char *name, *p;

	name = smprint("%s/plan9_%s", pipepfx, unix);
	if (name!=nil) {
		for (p=name+sizeof(pipepfx)+1; *p; p++)
			if (*p=='/' || *p=='.')
				*p = '_';
	}
	return name;	
}


int
p9dial(char *addr, char *local, char *dummy2, int *dummy3)
{
	char *buf;
	char *net, *unix;
	u32int host;
	int port;
	int proto;
	int sn;
	int n;
	struct sockaddr_in sa, sal;
	SOCKET s;
	HANDLE h;
	char *pname;
	int fd;

	if(dummy2 || dummy3){
		werrstr("cannot handle extra arguments in dial");
		return -1;
	}

	buf = strdup(addr);
	if(buf == nil)
		return -1;

	if(p9dialparse(buf, &net, &unix, &host, &port) < 0){
		free(buf);
		return -1;
	}

	if(strcmp(net, "tcp") == 0)
		proto = SOCK_STREAM;
	else if(strcmp(net, "udp") == 0)
		proto = SOCK_DGRAM;
	else if(strcmp(net, "unix") == 0)
		goto Unix;
	else{
		werrstr("can only handle tcp, udp, and unix: not %s", net);
		free(buf);
		return -1;
	}
	free(buf);

	if((s = socket(AF_INET, proto, 0)) < 0)
		return -1;
		
	if(local){
		buf = strdup(local);
		if(buf == nil){
			closesocket(s);
			return -1;
		}
		if(p9dialparse(buf, &net, &unix, &host, &port) < 0){
		badlocal:
			free(buf);
			closesocket(s);
			return -1;
		}
		if(unix){
			werrstr("bad local address %s for dial %s", local, addr);
			goto badlocal;
		}
		memset(&sal, 0, sizeof sal);
		memmove(&sal.sin_addr, &local, 4);
		sal.sin_family = AF_INET;
		sal.sin_port = htons(port);
		sn = sizeof n;
		if(port && getsockopt(s, SOL_SOCKET, SO_TYPE, (void*)&n, &sn) >= 0
		&& n == SOCK_STREAM){
			n = 1;
			setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof n);
		}
		if(bind(s, (struct sockaddr*)&sal, sizeof sal) < 0)
			goto badlocal;
		free(buf);
	}

	n = 1;
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&n, sizeof n);
	if(host != 0){
		memset(&sa, 0, sizeof sa);
		memmove(&sa.sin_addr, &host, 4);
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		if(connect(s, (struct sockaddr*)&sa, sizeof sa) < 0){
			closesocket(s);
			return -1;
		}
	}
	if(proto == SOCK_STREAM){
		int one = 1;
		setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof one);
	}

	fd = fdtalloc(nil);
	fdtab[fd]->type = Fdtypesock;
	fdtab[fd]->s = s;
	fdtab[fd]->name = strdup(addr);
	return fd;

Unix:
	if(local){
		werrstr("local address not supported on unix network");
		free(buf);
		return -1;
	}
	/* Allow regular files in addition to Unix sockets. */
	if((fd = open(unix, ORDWR)) >= 0)
		return fd;

	pname = unix2winpipe(unix);
	h = wincreatefile(pname,
		GENERIC_READ | GENERIC_WRITE, 
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED);
	if (h==INVALID_HANDLE_VALUE) {
		winerror("CreateFile");
		free(pname);
		return -1;
	}
	fd = fdtalloc(nil);
	fdtab[fd]->type = Fdtypepipecl;
	fdtab[fd]->h = h;
	fdtab[fd]->name = pname;
	return fd;
}
