/*
 * Cygwin-specific implementations of misc functions
 *
 */

#define NOMINGWDEFINES
#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>



/*
 * Provide sigsetjmp and siglongjump as functions, since cygwin
 * defines them as macros.  The macros could not be used directly,
 * because at some places the jmp_buf is casted to (void*), which
 * would result in an error when the macro gets expanded.  Using the
 * macros inside corresponding function definitions, the program
 * crashed when finally calling siglongjmp.  With the macro
 * definitions expanded manually as below, it works.  How come?
 */

#ifndef siglongjmp
#error Siglongjmp isn't a macro anymore. Please update this file and <u.h>
#endif

void
mingw_siglongjmp(sigjmp_buf buf, int val)
{
	if (buf[_SAVEMASK])
		sigprocmask (SIG_SETMASK, (sigset_t *) (buf + _SIGMASK), 0);
	longjmp (buf, val);
}


#ifndef sigsetjmp
#error Sigsetjmp isn't a macro anymore. Please update this file and <u.h>
#endif

int
mingw_sigsetjmp(sigjmp_buf buf, int val)
{

	buf[_SAVEMASK] = val;
	sigprocmask (SIG_SETMASK, 0, (sigset_t *) (buf + _SIGMASK));	
        return setjmp (buf);
}


static char fddir[] = "/dev/fd/";
enum {
	NFDDIR = sizeof(fddir) - 1,
};
int
opendevfd(char *name)
{
	int fd;

	fd = -1;
	if (strncmp(name, fddir, NFDDIR) == 0) {
		int fdnum;
		
		fdnum = atoi(name + NFDDIR);
		switch(fdnum) {
		case 0:
		case 1:
		case 2:
			fd = dup(fdnum);
			break;
		}
	}

	return fd;
}
