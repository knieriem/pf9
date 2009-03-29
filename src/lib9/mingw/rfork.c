#include <u.h>
#include <libc.h>
#undef rfork

int
p9rfork(int flags)
{
	if(flags&RFPROC){
		werrstr("cannot use rfork for shared memory -- use libthread");
		return -1;
	}
	if(flags&RFNAMEG){
		/* XXX set $NAMESPACE to a new directory */
		flags &= ~RFNAMEG;
	}
	if(flags&RFNOTEG){
//		setpgid(0, getpid());
		flags &= ~RFNOTEG;
	}
	if(flags&RFNOWAIT){
		werrstr("cannot use RFNOWAIT without RFPROC");
		return -1;
	}
	if(flags){
		werrstr("unknown flags %08ux in rfork", flags);
		return -1;
	}
	return 0;
}
