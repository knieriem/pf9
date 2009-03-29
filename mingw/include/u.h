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

/* mingw doesn't know SIGHUP and SIGQUIT */
#define	SIGHUP	-1
#define	SIGQUIT	-1

 
#define _NEEDUCHAR 1
#define _NEEDUSHORT 1
#define _NEEDUINT 1
#define _NEEDULONG 1

/* FIXME: mingw has no sigjmp_buf */
#define sigjmp_buf jmp_buf
typedef long p9jmp_buf[sizeof(sigjmp_buf)/sizeof(long)];

/* mingw has no sigsetjmp */
#define sigsetjmp(b,s) _setjmp((void*)b)

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

extern	void* winsbrk(unsigned long);
#define	sbrk	winsbrk

extern	int	fchdir(int);
extern	int	fchmod(int, mode_t);

#ifndef O_DIRECT
#define O_DIRECT 0
#endif

typedef const char	cchar;

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

#undef _NEEDUCHAR
#undef _NEEDUSHORT
#undef _NEEDUINT
#undef _NEEDULONG

/* FCR */
#define	FPINEX	(1<<5)
#define	FPUNFL	((1<<4)|(1<<1))
#define	FPOVFL	(1<<3)
#define	FPZDIV	(1<<2)
#define	FPINVAL	(1<<0)
#define	FPRNR	(0<<10)
#define	FPRZ	(3<<10)
#define	FPRPINF	(2<<10)
#define	FPRNINF	(1<<10)
#define	FPRMASK	(3<<10)
#define	FPPEXT	(3<<8)
#define	FPPSGL	(0<<8)
#define	FPPDBL	(2<<8)
#define	FPPMASK	(3<<8)
/* FSR */
#define	FPAINEX	FPINEX
#define	FPAOVFL	FPOVFL
#define	FPAUNFL	FPUNFL
#define	FPAZDIV	FPZDIV
#define	FPAINVAL	FPINVAL
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
#	if __GNUC__ >= 4 || (__GNUC__==3 && !defined(__MINGW32__))
#		undef AUTOLIB
#		define AUTOLIB(x) int __p9l_autolib_ ## x __attribute__ ((weak));
#		undef AUTOFRAMEWORK
#		define AUTOFRAMEWORK(x) int __p9l_autoframework_ ## x __attribute__ ((weak));
#	else
#		undef AUTOLIB
#		define AUTOLIB(x) static int __p9l_autolib_ ## x __attribute__ ((unused));
#		undef AUTOFRAMEWORK
#		define AUTOFRAMEWORK(x) static int __p9l_autoframework_ ## x __attribute__ ((unused));
#	endif
#endif

#if defined(__cplusplus)
}
#endif
#endif