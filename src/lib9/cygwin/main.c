#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>

extern void p9main(int, char**);

extern int	opendevfd(char *);		/* misc.c */
extern int	(*devfdhandler)(char *);	/* open.c */

int
main(int argc, char **argv)
{
	/* install /dev/fd handler */
	devfdhandler = opendevfd;

	p9main(argc, argv);
	exits("main");
	return 99;
}
