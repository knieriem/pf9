#ifndef _U_H_
#define _U_H_ 1
#if defined(__cplusplus)
extern "C" {
#endif

#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <inttypes.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <assert.h>
#include <setjmp.h>
#include <stddef.h>
#include <math.h>
#include <ctype.h>	/* for tolower */
#include <wchar.h>

/*
 * OS-specific crap
 */
 
/* undef macros that might conflict with own definitions */
#define	sopen	pf9_sopen
#define	cwait	pf9_cwait
#undef	eof
#define	eof		pf9_eof
#undef	stdin
#undef	stderr
#undef	stdout
#undef	strtod

/* mingw doesn't know SIGHUP and SIGQUIT */
#define	SIGHUP	-1
#define	SIGQUIT	-1

 
#define _NEEDUCHAR 1
#define _NEEDUSHORT 1
#define _NEEDUINT 1
#define _NEEDULONG 1

typedef int sigset_t;

/* FIXME: mingw has no sigjmp_buf */
#define sigjmp_buf jmp_buf
typedef long p9jmp_buf[sizeof(sigjmp_buf)/sizeof(long)];

/* mingw has no sigsetjmp */
#ifdef _WIN64
#define sigsetjmp(b, s) _setjmp((b), mingw_getsp())
#else
#define sigsetjmp(b,s) _setjmp3((void*)(b), NULL)
#endif

/*
 * This is a placeholder for Windows's CRITICAL_SECTION
 */
typedef struct MingwCritSect
{
	int	dummy[32/sizeof(int)];
} MingwCritSect;

#undef	environ
#define	environ	mingwenviron

#undef	mkstemp
#define	mkstemp	mingwmkstemp
extern	int	mingwmkstemp(char*);

#undef remove
#define remove mingwremove
extern	int	mingwremove(const char*);

extern	void* winsbrk(unsigned long);
#define	sbrk	winsbrk

extern	int	fchdir(int);
extern	int	fchmod(int, mode_t);

#ifndef O_DIRECT
#define O_DIRECT 0
#endif

typedef signed char schar;

#ifdef _NEEDUCHAR
	typedef unsigned char uchar;
#endif
#ifdef _NEEDUSHORT
	typedef unsigned short ushort;
#endif
#ifdef _NEEDUINT
	typedef unsigned int uint;
#endif
#ifdef _NEEDULONG
	typedef unsigned long ulong;
#endif
typedef unsigned long long uvlong;
typedef long long vlong;

typedef uint64_t u64int;
typedef int64_t s64int;
typedef uint8_t u8int;
typedef int8_t s8int;
typedef uint16_t u16int;
typedef int16_t s16int;
typedef uintptr_t uintptr;
typedef intptr_t intptr;
typedef uint32_t u32int;
typedef int32_t s32int;
typedef union FPdbleword FPdbleword;

typedef u32int uint32;
typedef s32int int32;
typedef u16int uint16;
typedef s16int int16;
typedef u64int uint64;
typedef s64int int64;


#undef _NEEDUCHAR
#undef _NEEDUSHORT
#undef _NEEDUINT
#undef _NEEDULONG

union FPdbleword
{
	double	x;
	struct {	/* little endian */
		ulong lo;
		ulong hi;
	};
};


/*
 * Funny-named symbols to tip off 9l to autolink.
 */
#define AUTOLIB(x)	static int __p9l_autolib_ ## x = 1;
#define AUTOFRAMEWORK(x) static int __p9l_autoframework_ ## x = 1;

/*
 * Gcc is too smart for its own good.
 */
#if defined(__GNUC__)
#	undef strcmp	/* causes way too many warnings */
#	if __GNUC__ >= 4 && !defined(__MINGW32__)
#		undef AUTOLIB
#		define AUTOLIB(x) int __p9l_autolib_ ## x __attribute__ ((weak));
#		undef AUTOFRAMEWORK
#		define AUTOFRAMEWORK(x) int __p9l_autoframework_ ## x __attribute__ ((weak));
#	else
#		undef AUTOLIB
#		define AUTOLIB(x) int __p9l_autolib_ ## x;
#		undef AUTOFRAMEWORK
#		define AUTOFRAMEWORK(x) int __p9l_autoframework_ ## x;
#	endif
#endif

#if defined(__cplusplus)
}
#endif
#endif
