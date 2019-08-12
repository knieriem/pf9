#include <u.h>
#include <ws2tcpip.h>
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

static int
isany(struct sockaddr_storage *ss)
{
	switch(ss->ss_family){
	case AF_INET:
		return (((struct sockaddr_in*)ss)->sin_addr.s_addr == INADDR_ANY);
	case AF_INET6:
		return (memcmp(((struct sockaddr_in6*)ss)->sin6_addr.s6_addr,
			in6addr_any.s6_addr, sizeof (struct in6_addr)) == 0);
	}
	return 0;
}

static int
addrlen(struct sockaddr_storage *ss)
{
	switch(ss->ss_family){
	case AF_INET:
		return sizeof(struct sockaddr_in);
	case AF_INET6:
		return sizeof(struct sockaddr_in6);
	}
	return 0;
}

int
p9dial(char *addr, char *local, char *dummy2, int *dummy3)
{
	char *buf;
	char *net, *unix;
	int port;
	int proto;
	int sn;
	int n;
	struct sockaddr_storage ss, ssl;
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

	if(p9dialparse(buf, &net, &unix, &ss, &port) < 0){
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

	if((s = socket(ss.ss_family, proto, 0)) < 0)
		return -1;
		
	if(local){
		buf = strdup(local);
		if(buf == nil){
			closesocket(s);
			return -1;
		}
		if(p9dialparse(buf, &net, &unix, &ss, &port) < 0){
		badlocal:
			free(buf);
			closesocket(s);
			return -1;
		}
		if(unix){
			werrstr("bad local address %s for dial %s", local, addr);
			goto badlocal;
		}
		sn = sizeof n;
		if(port && getsockopt(s, SOL_SOCKET, SO_TYPE, (void*)&n, &sn) >= 0
		&& n == SOCK_STREAM){
			n = 1;
			setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof n);
		}
		if(bind(s, (struct sockaddr*)&ssl, addrlen(&ssl)) < 0)
			goto badlocal;
		free(buf);
	}

	n = 1;
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&n, sizeof n);
	if(!isany(&ss)){
		if(connect(s, (struct sockaddr*)&ss, addrlen(&ss)) < 0){
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
