#include <u.h>
#include <libc.h>
#include <keyboard.h>

extern int _latin1(Rune*, int);
static Rune*
utoplan9latin1(int r)
{
	static Rune k[10];
	static int alting, nk;
	int n;

	if(r < 0)
		return nil;
	if(alting){
		/*
		 * Kludge for Mac's X11 3-button emulation.
		 * It treats Command+Button as button 3, but also
		 * ends up sending XK_Meta_L twice.
		 */
		if(r == Kalt){
			alting = 0;
			return nil;
		}
		k[nk++] = r;
		n = _latin1(k, nk);
		if(n > 0){
			alting = 0;
			k[0] = n;
			k[1] = 0;
			return k;
		}
		if(n == -1){
			alting = 0;
			k[nk] = 0;
			return k;
		}
		/* n < -1, need more input */
		return nil;
	}else if(r == Kalt){
		alting = 1;
		nk = 0;
		return nil;
	}else{
		k[0] = r;
		k[1] = 0;
		return k;
	}
}
int
utoplan9kbd(int c)
{
	static Rune *r;

	r = utoplan9latin1(c);
	if(r && *r)
		return *r++;
	return -1;
}
