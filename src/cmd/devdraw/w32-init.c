/*
 * Some of the stuff in this file is not X-dependent and should be elsewhere.
 */
#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include <keyboard.h>
#include <mouse.h>
#include <cursor.h>

static int parsewinsize(char*, Rectangle*, int*);

static void	plan9cmap(void);

Memimage*
_xattach(char *label, char *winsize)
{
	char *argv[2], *disp;
	int i, havemin, height, mask, n, width, x, xrootid, y;
	Rectangle r;

	return nil;
}

int
_xsetlabel(char *label)
{
	return 0;
}
/*
 * Initialize map with the Plan 9 rgbv color map.
 */
static void
plan9cmap(void)
{
	int r, g, b, cr, cg, cb, v, num, den, idx, v7, idx7;
	static int once;

	if(once)
		return;
	once = 1;

	for(r=0; r!=4; r++)
	for(g = 0; g != 4; g++)
	for(b = 0; b!=4; b++)
	for(v = 0; v!=4; v++){
		den=r;
		if(g > den)
			den=g;
		if(b > den)
			den=b;
		/* divide check -- pick grey shades */
		if(den==0)
			cr=cg=cb=v*17;
		else {
			num=17*(4*den+v);
			cr=r*num/den;
			cg=g*num/den;
			cb=b*num/den;
		}
		idx = r*64 + v*16 + ((g*4 + b + v - r) & 15);
#if 0
		_x.map[idx].red = cr*0x0101;
		_x.map[idx].green = cg*0x0101;
		_x.map[idx].blue = cb*0x0101;
		_x.map[idx].pixel = idx;
		_x.map[idx].flags = DoRed|DoGreen|DoBlue;

		v7 = v >> 1;
		idx7 = r*32 + v7*16 + g*4 + b;
		if((v & 1) == v7){
			_x.map7to8[idx7][0] = idx;
			if(den == 0) { 		/* divide check -- pick grey shades */
				cr = ((255.0/7.0)*v7)+0.5;
				cg = cr;
				cb = cr;
			}
			else {
				num=17*15*(4*den+v7*2)/14;
				cr=r*num/den;
				cg=g*num/den;
				cb=b*num/den;
			}
			_x.map7[idx7].red = cr*0x0101;
			_x.map7[idx7].green = cg*0x0101;
			_x.map7[idx7].blue = cb*0x0101;
			_x.map7[idx7].pixel = idx7;
			_x.map7[idx7].flags = DoRed|DoGreen|DoBlue;
		}
		else
			_x.map7to8[idx7][1] = idx;
#endif
	}
}


void
_flushmemscreen(Rectangle r)
{
}


static int
parsewinsize(char *s, Rectangle *r, int *havemin)
{
	char c, *os;
	int i, j, k, l;

	os = s;
	*havemin = 0;
	*r = Rect(0,0,0,0);
	if(!isdigit((uchar)*s))
		goto oops;
	i = strtol(s, &s, 0);
	if(*s == 'x'){
		s++;
		if(!isdigit((uchar)*s))
			goto oops;
		j = strtol(s, &s, 0);
		r->max.x = i;
		r->max.y = j;
		if(*s == 0)
			return 0;
		if(*s != '@')
			goto oops;

		s++;
		if(!isdigit((uchar)*s))
			goto oops;
		i = strtol(s, &s, 0);
		if(*s != ',' && *s != ' ')
			goto oops;
		s++;
		if(!isdigit((uchar)*s))
			goto oops;
		j = strtol(s, &s, 0);
		if(*s != 0)
			goto oops;
		*r = rectaddpt(*r, Pt(i,j));
		*havemin = 1;
		return 0;
	}

	c = *s;
	if(c != ' ' && c != ',')
		goto oops;
	s++;
	if(!isdigit((uchar)*s))
		goto oops;
	j = strtol(s, &s, 0);
	if(*s != c)
		goto oops;
	s++;
	if(!isdigit((uchar)*s))
		goto oops;
	k = strtol(s, &s, 0);
	if(*s != c)
		goto oops;
	s++;
	if(!isdigit((uchar)*s))
		goto oops;
	l = strtol(s, &s, 0);
	if(*s != 0)
		goto oops;
	*r = Rect(i,j,k,l);
	*havemin = 1;
	return 0;

oops:
	werrstr("bad syntax in window size '%s'", os);
	return -1;
}
