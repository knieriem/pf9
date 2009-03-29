#include <u.h>
#include <mingw32.h>
#include <libc.h>

char*
getuser(void)
{
	static char user[64];

	DWORD	ulen;

	ulen = sizeof user;
	if (GetUserName(user, &ulen))
		return user;

	return "none";
}
