/* from 9pm */

#include <u.h>
#include <mingw32.h>
#include <libc.h>
#include <libsec.h>

typedef struct State{
	int		seeded;
	uvlong		seed;
	DES3state	des3;
} State;

static State x917state;

static __inline uvlong
rdtsc (void)
{
	unsigned long l, h;

	/* RDTSC - get beginning timestamp to edx:eax */
	__asm__ __volatile__ ("rdtsc":"=a" (l), "=d" (h));
	return ((uvlong) h << 32) | l;
}

/*
 * attempt at generating a pretty good random seed
 * the following assumes we are on a pentium - should do
 * something if we do not have the rdtsc instruction
 */
static void
randomseed(uchar *p)
{
	SHAstate *s;
	uvlong tsc;
	LARGE_INTEGER ti;
	int i, j;
	FILETIME ft;

	GetSystemTimeAsFileTime(&ft);
	s = sha1((uchar*)&ft, sizeof(ft), 0, 0);
	for(i=0; i<50; i++) {
		for(j=0; j<10; j++) {
			tsc = rdtsc();
			s = sha1((uchar*)&tsc, sizeof(tsc), 0, s);
			QueryPerformanceCounter(&ti);
			s = sha1((uchar*)&ti, sizeof(ti), 0, s);
			tsc = GetTickCount();
			s = sha1((uchar*)&tsc, sizeof(tsc), 0, s);
		}
		Sleep(10);
	}
	sha1(0, 0, p, s);
}


static void
X917(uchar *rand, int nrand)
{
	int i, m, n8;
	uvlong I, x;

	/* 1. Compute intermediate value I = Ek(time). */
	I = nsec();
	triple_block_cipher(x917state.des3.expanded, (uchar*)&I, 0); /* two-key EDE */

	/* 2. x[i] = Ek(I^seed);  seed = Ek(x[i]^I); */
	m = (nrand+7)/8;
	for(i=0; i<m; i++){
		x = I ^ x917state.seed;
		triple_block_cipher(x917state.des3.expanded, (uchar*)&x, 0);
		n8 = (nrand>8) ? 8 : nrand;
		memcpy(rand, (uchar*)&x, n8);
		rand += 8;
		nrand -= 8;
		x ^= I;
		triple_block_cipher(x917state.des3.expanded, (uchar*)&x, 0);
		x917state.seed = x;
	}
}

static void
X917init(void)
{
	uchar mix[128];
	uchar key3[3][8];
	uchar seed[SHA1dlen];

	randomseed(seed);
	memmove(key3, seed, SHA1dlen);
	setupDES3state(&x917state.des3, key3, nil);
	X917(mix, sizeof mix);
	x917state.seeded = 1;
}

ulong
truerand(void)
{
	ulong x;

	static QLock lk;

	qlock(&lk);
	if(x917state.seeded == 0)
		X917init();
	X917((uchar*) &x, sizeof x);
	qunlock(&lk);
	return x;
}
