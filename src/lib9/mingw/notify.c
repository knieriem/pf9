/*
 * Signal handling for Plan 9 programs. 
 * We stubbornly use the strings from Plan 9 instead 
 * of the enumerated Unix constants.  
 * There are some weird translations.  In particular,
 * a "kill" note is the same as SIGTERM in Unix.
 * There is no equivalent note to Unix's SIGKILL, since
 * it's not a deliverable signal anyway.
 *
 * We do not handle SIGABRT or SIGSEGV, mainly because
 * the thread library queues its notes for later, and we want
 * to dump core with the state at time of delivery.
 *
 * We have to add some extra entry points to provide the
 * ability to tweak which signals are deliverable and which
 * are acted upon.  Notifydisable and notifyenable play with
 * the process signal mask.  Notifyignore enables the signal
 * but will not call notifyf when it comes in.  This is occasionally
 * useful.
 */

#include <u.h>
#include <signal.h>
#include <mingw32.h>
#define NOPLAN9DEFINES
#include <libc.h>

#include "util.h"

static struct {
	int sig;
	char *str;
} tab[] = {
	SIGHUP,		"hangup",
	SIGINT,		"interrupt",
	SIGILL,		"sys: illegal instruction",
	SIGABRT,		"sys: abort",
	SIGFPE,		"sys: fp: trap",
	SIGSEGV,		"sys: segmentation violation",
	SIGTERM,		"kill",
};
	
static char*
_p9sigstr(int sig, char *tmp)
{
	int i;

	for(i=0; i<nelem(tab); i++)
		if(tab[i].sig == sig)
			return tab[i].str;
	if(tmp == nil)
		return nil;
	sprint(tmp, "sys: signal %d", sig);
	return tmp;
}

static int
_p9strsig(char *s)
{
	int i;

	for(i=0; i<nelem(tab); i++)
		if(strcmp(s, tab[i].str) == 0)
			return tab[i].sig;
	return 0;
}

typedef struct Sig Sig;
struct Sig
{
	int sig;			/* signal number */
	int flags;
};

static Sig sigs[] = {
	SIGINT,		0,
	SIGILL,		0,
/*	SIGABRT, 		0, 	*/
	SIGFPE,		0,
/*	SIGSEGV, 		0, 	*/
	SIGTERM,		0,
};

static Sig*
findsig(int s)
{
	int i;

	for(i=0; i<nelem(sigs); i++)
		if(sigs[i].sig == s)
			return &sigs[i];
	return nil;
}

/*
 * _notejmpbuf is just a dummy here,
 * as signal handlers are not called inside
 * one of libthread's procs, but perhaps in
 * a separate one. Onejmp is always used.
 * 
 */
typedef struct Jmp Jmp;
struct Jmp
{
	p9jmp_buf b;
};

static Jmp onejmp;

Jmp *(*_notejmpbuf)(void);
static void noteinit(void);

/*
 * Actual signal handler. 
 */

static void (*notifyf)(void*, char*);	/* Plan 9 handler */

static void
signotify(int sig)
{
	char tmp[64];
	Jmp *j;

	signal(sig, signotify);
	j = &onejmp;
	switch(p9setjmp(j->b)){
	case 0:
//    Beep( 200, 200 ); 
		if(notifyf)
			(*notifyf)(nil, _p9sigstr(sig, tmp));
		/* fall through */
	case 1:	/* noted(NDFLT) */
 //   Beep( 7000, 200 ); 
 		if(0)print("DEFAULT %d\n", sig);
		signal(sig, SIG_DFL);
		raise(sig);
		_exit(1);
	case 2:	/* noted(NCONT) */
		if(0)print("HANDLED %d\n", sig);
		return;
	}
}

static void
signonotify(int sig)
{
	USED(sig);
}

int
noted(int v)
{
	p9longjmp(onejmp.b, v==NCONT ? 2 : 1);
	abort();
	return 0;
}

int
notify(void (*f)(void*, char*))
{
	static int init;

	notifyf = f;
	if(!init){
		init = 1;
		noteinit();
	}
	return 0;
}

/*
 * Nonsense about enabling and disabling signals.
 */
typedef void Sighandler(int);
static Sighandler*
handler(int s)
{
	Sighandler *h;

	h = signal(s, SIG_DFL);
	signal(s, h);

	return h;
}

static int
notesetenable(int sig, int enabled)
{
	if(sig == 0)
		return -1;

	/* not implemented yet */
	return -1;	
}

int
noteenable(char *msg)
{
	return notesetenable(_p9strsig(msg), 1);
}

int
notedisable(char *msg)
{
	return notesetenable(_p9strsig(msg), 0);
}

static int
notifyseton(int s, int on)
{
	Sig *sig;
	Sighandler *h, *oh;

	sig = findsig(s);
	if(sig == nil)
		return -1;
	h = on ? signotify : signonotify;

	/*
	 * Install handler.
	 */
	oh = signal(sig->sig, h);
	return oh == signotify;
}

int
notifyon(char *msg)
{
	return notifyseton(_p9strsig(msg), 1);
}

int
notifyoff(char *msg)
{
	return notifyseton(_p9strsig(msg), 0);
}

static
BOOL WINAPI
ctrlhandler(DWORD type)
{
	switch(type){
	case CTRL_BREAK_EVENT:
		raise(SIGINT);
		return TRUE;
	}
	return FALSE;	
}


/*
 * Initialization follows sigs table.
 */
static void
noteinit(void)
{
	int i;
	Sig *sig;

	for(i=0; i<nelem(sigs); i++){
		sig = &sigs[i];
		/*
		 * If someone has already installed a handler,
		 * It's probably some ld preload nonsense,
		 * like pct (a SIGVTALRM-based profiler).
		 * Or maybe someone has already called notifyon/notifyoff.
		 * Leave it alone.
		 */
		if(handler(sig->sig) != SIG_DFL)
			continue;
		notifyseton(sig->sig, 1);
	}
	if(winhascons())
		SetConsoleCtrlHandler(ctrlhandler, 1);
}
