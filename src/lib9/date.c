#define NOPLAN9DEFINES
#include <u.h>
#include <libc.h>
#include <time.h>

typedef
struct Tzspec
{
	char	name[4];
	long	off;
} Tzspec;

static Tzspec tzgmt = { "GMT", 0 };
static Tzspec *tzst, *tzdl;

static long
dotz(time_t t)
{
	struct tm *gtm;
	struct tm tm;

	tm = *localtime(&t);	/* set local time zone field */
	gtm = gmtime(&t);
	tm.tm_sec = gtm->tm_sec;
	tm.tm_min = gtm->tm_min;
	tm.tm_hour = gtm->tm_hour;
	tm.tm_mday = gtm->tm_mday;
	tm.tm_mon = gtm->tm_mon;
	tm.tm_year = gtm->tm_year;
	tm.tm_wday = gtm->tm_wday;
	return t - mktime(&tm);
	/* tzname[] is set now */
}
static void
copyname(Tzspec *tz, int i)
{
	strncpy(tz->name, tzname[i], sizeof tz->name);
	tz->name[sizeof tz->name-1] = 0;
}
static void
readtimezone(void)
{
	static Tzspec tz0, tz1;

	/*
	 * Get offset values for normal time
	 * and perhaps daylight savings time;
	 * this will also set tzname[]. Then find
	 * out which one is the normal time (might
	 * be different on southern hemisphere).
	 */
	tz0.off = dotz(1230678000);		/* December 31, 2008 */
	tz1.off = dotz(1212271200);		/* June 1, 2008 */

	tzst = &tz0;
	if (tz1.off>tz0.off)
		tzdl = &tz1;
	else if (tz1.off<tz0.off) {
		tzdl = &tz0;
		tzst = &tz1;
	} else
		tzdl = tzst;	/* no dst */

	copyname(tzst, 0);
	if (tzdl!=tzst)
		copyname(tzdl, 1);
}
static void
tm2Tm(struct tm *tm, Tm *bigtm, Tzspec *tz)
{
	memset(bigtm, 0, sizeof *bigtm);
	bigtm->sec = tm->tm_sec;
	bigtm->min = tm->tm_min;
	bigtm->hour = tm->tm_hour;
	bigtm->mday = tm->tm_mday;
	bigtm->mon = tm->tm_mon;
	bigtm->year = tm->tm_year;
	bigtm->wday = tm->tm_wday;
	bigtm->tzoff = tz->off;
	strncpy(bigtm->zone, tz->name, 3);
	bigtm->zone[3] = 0;
}

static void
Tm2tm(Tm *bigtm, struct tm *tm)
{
	tm->tm_sec = bigtm->sec;
	tm->tm_min = bigtm->min;
	tm->tm_hour = bigtm->hour;
	tm->tm_mday = bigtm->mday;
	tm->tm_mon = bigtm->mon;
	tm->tm_year = bigtm->year;
	tm->tm_wday = bigtm->wday;
}

Tm*
p9gmtime(long x)
{
	time_t t;
	struct tm tm;
	static Tm bigtm;
	
	t = (time_t)x;
	tm = *gmtime(&t);
	tm2Tm(&tm, &bigtm, &tzgmt);
	return &bigtm;
}

Tm*
p9localtime(long x)
{
	time_t t;
	struct tm tm;
	static Tm bigtm;

	t = (time_t)x;
	tm = *localtime(&t);
	if (tzst==nil)
		readtimezone();
	tm2Tm(&tm, &bigtm, tm.tm_isdst>0? tzdl: tzst);
	return &bigtm;
}

long
p9tm2sec(Tm *bigtm)
{
	time_t t;
	struct tm tm;

	Tm2tm(bigtm, &tm);
	tm.tm_isdst = -1;		/* let libc decide */
	t = mktime(&tm);
	if(strcmp(bigtm->zone, "GMT") == 0 || strcmp(bigtm->zone, "UCT") == 0) {
		if (tzst==nil)
			readtimezone();
		/*
		 * shift t by the offset actually applied by mktime
		 */
		t += (localtime(&t)->tm_isdst>0? tzdl: tzst)->off;	
	}
	return t;
}

