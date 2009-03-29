#include <u.h>
#include <libc.h>

extern	char	**environ;
void
main(void)
{
	char **envp;
	int i;

	envp = environ;
	for(i=0; envp[i]; i++)
		print("%s\n", envp[i]);
}
