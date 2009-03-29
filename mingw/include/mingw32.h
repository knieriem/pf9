#ifdef _LIBC_H_
#error Please include libc.h after mingw32.h
#endif

#if defined(__cplusplus)
extern "C" {
#endif


#include <windows.h>

/* missing stuff */
WINBASEAPI DWORD WINAPI GetLongPathNameW(LPCWSTR,LPWSTR,DWORD);

/*
 * check whether MingwCritSect has the right size
 */
typedef
char _wewontusethis[sizeof(MingwCritSect)<sizeof(CRITICAL_SECTION)? 1/0: 1];

/* undef macros that might conflict with own definitions */
#undef	max
//#define	max		pf9_max
#undef	min
//#define	min		pf9_min
#undef	small
#define	small	pf9_small

#undef ERROR
#undef FSHIFT
#undef	Arc
#undef	Rectangle
#define	Rectangle	pf9_Rectangle
#define	shutdown pf9_shutdown
#define	strlwr	pf9_strlwr
#define	CY		pf9_CY
#define	CRGB	pf9_CRGB
#undef	OUT
#undef	IN
#undef	TEXT
#undef	DELETE
#undef	rad2

extern	int	winovresult(int ret, HANDLE, OVERLAPPED*, DWORD *np, int evclose);



#if defined(__cplusplus)
}
#endif
