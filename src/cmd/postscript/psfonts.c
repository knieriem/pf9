#include <u.h>
#include <libc.h>

char *dnlargv[] = {
	"psdownload",
	"-f",
	"-mfontmap",
	"-r",
	"#9/postscript/font/lw+",
	nil
};


void
main(int argc, char *argv[])
{
	char **p, **dnlarg, **newargv;

	print("%%!PS-Adobe-2.0\n");
	
	newargv = malloc((nelem(dnlargv) + argc -1) * sizeof(char*));
	if (newargv == nil)
		sysfatal("could not allocate memory for the argument list");
	
	dnlarg = &dnlargv[0];
	for (p = &newargv[0]; *dnlarg != nil; p++, dnlarg++)
		*p = unsharp(*dnlarg);
	
	do {
		argc--;
		argv++;
		*p++ = argv[0];
	} while (argc > 0);
	*p = nil;

	exec("psdownload", newargv);
	
	exits(nil);
}
