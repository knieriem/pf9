#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#include <libc.h>

#undef main

#include "fdtab.h"
#include "util.h"

extern void p9main(int, char**);

static void
fatal(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprint(2, fmt, ap);
	va_end(ap);
	exit(1);
}

static
char **
utfargv(int *np)
{
	WCHAR **wargv;
	int	i, argc, len;
	char **argv, *args, *w, *ep;

	wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

	argv = malloc((argc+1) * sizeof(char*));
	if (argv==nil)
		fatal("malloc argv failed");

	len = 0;
	for (i=0; i<argc; i++)
		len += winwstrutflen(wargv[i])+1;
	args = malloc(len*sizeof(char));
	if (args==nil)
		fatal("malloc UTF args failed");

	ep = args+len*sizeof(char);
	w = args;
	for (i=0; i<argc; i++) {
		argv[i] = w;
		w = winwstrtoutfe(w, ep, wargv[i])+1;
	}
	argv[i] = nil;

	LocalFree(wargv);
	*np = argc;

	return argv;
}

int
main(int argc, char *argv[])
{
	OSVERSIONINFOA osinfo;
	WSADATA	wsaData;
	int	errcode;

	if (mingwinitenv(nil)==-1)
		abort();

	fdtabinit();

	argv = utfargv(&argc);

	osinfo.dwOSVersionInfoSize = sizeof(osinfo);
	if(!GetVersionExA(&osinfo))
		fatal("GetVersionEx failed");
	switch(osinfo.dwPlatformId) {
	default:
		fatal("unknown PlatformId");
	case VER_PLATFORM_WIN32s:
		fatal("Win32s not supported");
	case VER_PLATFORM_WIN32_WINDOWS:
		fatal("Sorry, Windows Me, 95, and 98, are not supported");
		break;
	case VER_PLATFORM_WIN32_NT:
		break;
	}

	errcode = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (errcode != 0)
		fatal("WSAStartup failed with error: %d", errcode);

	p9main(argc, argv);
	free(argv);

	exits("main");
	return 99;
}
