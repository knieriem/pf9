/*
 * Some support for functions that are not part of MingW
 *
 */

#include <u.h>
#include <libc.h>

extern	int	fork_is_not_implemented_but_you_could_use_threadspawn;

int
fork(void)
{
	fork_is_not_implemented_but_you_could_use_threadspawn = 1;
	werrstr("not implemented");
	return -1;
}
