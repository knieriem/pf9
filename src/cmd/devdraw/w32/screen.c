#define UNICODE
#include <u.h>
#include <mingw32.h>
#include <mingwutil.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <cursor.h>
#include <memdraw.h>
#include "screen.h"
#include "fns.h"
#include "keyboard.h"
#include "if.h"

enum {
	STACK	= 32768,
};

extern	LPWSTR	winutf2wstr(char*);
extern int parsewinsize(char*, Rectangle*, int*);

Memimage	*gscreen;
Screeninfo	screen;

static int depth;

static	HINSTANCE	inst;
static	HWND		window;
static	HPALETTE	palette;
static	LOGPALETTE	*logpal;
static  Lock		gdilock;
static 	BITMAPINFO	*bmi;
static	HCURSOR		hcursor;

static void	winproc(void *);
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static void	paletteinit(void);
static void	bmiinit(void);

Point	ZP;

extern	int	mresized;
extern	Channel *cmouse;
extern	Channel *ckbd;
static void
kbdputc(ulong c)
{
	static int wasctl;

	switch (c) {
	case Kctl:
		wasctl = 1;
		return;
	case Kalt:
		if (wasctl)
			break;	/* ignore AltGr key */
	default:
//		fprint(2, "taste: %ld 0x%04lux\n", c, c);
		sendul(ckbd, c);
	}
	wasctl = 0;
}

static void
oserror(void)
{
}

static uint
msec(void)
{
	return nsec()/1000000;
}

static int fmt;
static void
mkgscreen(void)
{
	RECT r;
	int dx, dy;

	if(!GetClientRect(window, &r)){
		oserror();
		sysfatal("GetClientRect: %r");
	}
	dx = r.right - r.left;
	dy = r.bottom - r.top;
	gscreen = allocmemimage(Rect(0,0,dx,dy), fmt);
}

static void
resizescreenimage(void)
{
	mkgscreen();
	_drawreplacescreenimage(gscreen);
}

typedef
struct Winarg
{
	int	x, y, dx, dy;
	Channel *csync;
	LPWSTR label;
} Winarg;
Memimage*
_xattach(char *label, char *winsize)
{
	Winarg *a;
	Rectangle r;
	int havemin;

	memimageinit();
	if(depth == 0)
		depth = GetDeviceCaps(GetDC(NULL), BITSPIXEL);
	switch(depth){
	case 32:
		screen.dibtype = DIB_RGB_COLORS;
		screen.depth = 32;
		fmt = XRGB32;
		break;
	case 24:
		screen.dibtype = DIB_RGB_COLORS;
		screen.depth = 24;
		fmt = RGB24;
		break;
	case 16:
		screen.dibtype = DIB_RGB_COLORS;
		screen.depth = 16;
		fmt = RGB15;	/* [sic] */
		break;
	case 8:
	default:
		screen.dibtype = DIB_PAL_COLORS;
		screen.depth = 8;
		depth = 8;
		fmt = CMAP8;
		break;
	}
	if(label == nil)
		label = "pjw-face-here";
	a = malloc(sizeof(Winarg));
	if (a==nil)
		sysfatal("out of memory");
	a->label = winutf2wstr(label);

	a->dx = CW_USEDEFAULT;
	a->dy = CW_USEDEFAULT;
	havemin = 0;
	if(winsize && winsize[0]){
		if(parsewinsize(winsize, &r, &havemin) < 0)
			sysfatal("%r");
		a->dx = Dx(r);
		a->dy = Dy(r);
	}
	if (havemin) {
		a->x = r.min.x;
		a->y = r.min.y;
	} else {
		a->x = CW_USEDEFAULT;
		a->y = CW_USEDEFAULT;
	}

//	fprint(2, "%d %d %d %d\n", a->x, a->y, a->dx, a->dy);
	a->csync = chancreate(sizeof(ulong), 0);
	proccreate(winproc, a, STACK);
	recvul(a->csync);

	mkgscreen();

	sendul(a->csync, 1);
	recvul(a->csync);
	return gscreen;
}

uchar*
attachscreen(Rectangle *r, ulong *chan, int *depth, int *width, int *softscreen, void **X)
{
	*r = gscreen->r;
	*chan = gscreen->chan;
	*depth = gscreen->depth;
	*width = gscreen->width;
	*softscreen = 1;

	return gscreen->data->bdata;
}

void
_flushmemscreen(Rectangle r)
{
	screenload(r, gscreen->depth, byteaddr(gscreen, ZP), ZP,
		gscreen->width*sizeof(ulong));
//	Sleep(100);
}

void
screenload(Rectangle r, int depth, uchar *p, Point pt, int step)
{
	int dx, dy, delx;
	HDC hdc;
	RECT winr;

	if(depth != gscreen->depth)
		sysfatal("screenload: bad ldepth");

	/*
	 * Sometimes we do get rectangles that are off the
	 * screen to the negative axes, for example, when
	 * dragging around a window border in a Move operation.
	 */
	if(rectclip(&r, gscreen->r) == 0)
		return;

	if((step&3) != 0 || ((pt.x*depth)%32) != 0 || ((ulong)p&3) != 0)
		sysfatal("screenload: bad params %d %d %ux", step, pt.x, p);
	dx = r.max.x - r.min.x;
	dy = r.max.y - r.min.y;

	if(dx <= 0 || dy <= 0)
		return;

	if(depth == 24)
		delx = r.min.x % 4;
	else
		delx = r.min.x & (31/depth);

	p += (r.min.y-pt.y)*step;
	p += ((r.min.x-delx-pt.x)*depth)>>3;

	if(GetWindowRect(window, &winr)==0)
		return;
	if(rectclip(&r, Rect(0, 0, winr.right-winr.left, winr.bottom-winr.top))==0)
		return;
	
	lock(&gdilock);

	hdc = GetDC(window);
	SelectPalette(hdc, palette, 0);
	RealizePalette(hdc);

//FillRect(hdc,(void*)&r, GetStockObject(BLACK_BRUSH));
//GdiFlush();
//Sleep(100);

	bmi->bmiHeader.biWidth = (step*8)/depth;
	bmi->bmiHeader.biHeight = -dy;	/* - => origin upper left */

	StretchDIBits(hdc, r.min.x, r.min.y, dx, dy,
		delx, 0, dx, dy, p, bmi, screen.dibtype, SRCCOPY);

	ReleaseDC(window, hdc);

	GdiFlush();
 
	unlock(&gdilock);
}

static void
winproc(void *v)
{
	WNDCLASS wc;
	MSG msg;
	LPWSTR class;
	Winarg *a;

	a = v;
	inst = GetModuleHandle(NULL);

	paletteinit();
	bmiinit();
//	terminit();

	class = winutf2wstr("9pmgraphics");
	wc.style = 0;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = inst;
	wc.hIcon = LoadIconA(inst, "IDI_ICON1");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = class;
	RegisterClass(&wc);

	window = CreateWindowEx(
		0,			/* extended style */
		class,		/* class */
		a->label,		/* caption */
		WS_OVERLAPPEDWINDOW,    /* style */
		a->x,		/* init. x pos */
		a->y,		/* init. y pos */
		a->dx,		/* init. x size */
		a->dy,		/* init. y size */
		NULL,			/* parent window (actually owner window for overlapped)*/
		NULL,			/* menu handle */
		inst,			/* program handle */
		NULL			/* create parms */
		);

	if(window == nil)
		sysfatal("can't make window\n");

	sendul(a->csync, 1);		/* window is ready */
	recvul(a->csync);		/* wait for gscreen */

	ShowWindow(window, SW_SHOWDEFAULT);

	sendul(a->csync, 1);		/* window is ready */
	chanfree(a->csync);	
//	free(a);

	screen.reshaped = 0;

	for(;;){
		switch(GetMessage(&msg, NULL, 0, 0)){
		case -1:
			winerror("GetMessage");
			sysfatal("%r\n");
		case 0:
			goto quit;
		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
quit:
	
//	MessageBox(0, "winproc", "exits", MB_OK);
	threadexitsall(nil);
}

int
col(int v, int n)
{
	int i, c;

	c = 0;
	for(i = 0; i < 8; i += n)
		c |= v << (16-(n+i));
	return c >> 8;
}


void
paletteinit(void)
{
	PALETTEENTRY *pal;
	int r, g, b, cr, cg, cb, v;
	int num, den;
	int i, j;

	logpal = mallocz(sizeof(LOGPALETTE) + 256*sizeof(PALETTEENTRY), 1);
	if(logpal == nil)
		sysfatal("out of memory");
	logpal->palVersion = 0x300;
	logpal->palNumEntries = 256;
	pal = logpal->palPalEntry;

	for(r=0,i=0; r<4; r++) {
		for(v=0; v<4; v++,i+=16){
			for(g=0,j=v-r; g<4; g++) {
				for(b=0; b<4; b++,j++){
					den=r;
					if(g>den)
						den=g;
					if(b>den)
						den=b;
					/* divide check -- pick grey shades */
					if(den==0)
						cr=cg=cb=v*17;
					else{
						num=17*(4*den+v);
						cr=r*num/den;
						cg=g*num/den;
						cb=b*num/den;
					}
					pal[i+(j&15)].peRed = cr;
					pal[i+(j&15)].peGreen = cg;
					pal[i+(j&15)].peBlue = cb;
					pal[i+(j&15)].peFlags = 0;
				}
			}
		}
	}
	palette = CreatePalette(logpal);
}


void
getcolor(ulong i, ulong *r, ulong *g, ulong *b)
{
	PALETTEENTRY *pal;

	pal = logpal->palPalEntry;
	*r = pal[i].peRed;
	*g = pal[i].peGreen;
	*b = pal[i].peBlue;
}

void
bmiinit(void)
{
	ushort *p;
	int i;

	bmi = mallocz(sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD), 1);
	if(bmi == 0)
		sysfatal("out of memory");
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = 0;
	bmi->bmiHeader.biHeight = 0;	/* - => origin upper left */
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = depth;
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biSizeImage = 0;
	bmi->bmiHeader.biXPelsPerMeter = 0;
	bmi->bmiHeader.biYPelsPerMeter = 0;
	bmi->bmiHeader.biClrUsed = 0;
	bmi->bmiHeader.biClrImportant = 0;	/* number of important colors: 0 means all */

	p = (ushort*)bmi->bmiColors;
	for(i = 0; i < 256; i++)
		p[i] = i;
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	PAINTSTRUCT paint;
	HDC hdc;
	LONG x, y, b;
	int wid, ht;
	Rectangle r;
	static Mouse m;

	b = 0;
	switch(msg) {
	case WM_CREATE:
		break;
	case WM_SETCURSOR:
		/* User set */
		if(hcursor != NULL) {
			SetCursor(hcursor);
			return 1;
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	case WM_MOUSEWHEEL:
		if ((int)(wparam & 0xFFFF0000)>0)
			b|=8;
		else
			b|=16;
	case WM_MOUSEMOVE:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		x = LOWORD(lparam);
		y = HIWORD(lparam);
		if(wparam & MK_LBUTTON)
			b = 1;
		if(wparam & MK_MBUTTON)
			b |= 2;
		if(wparam & MK_RBUTTON) {
			if(wparam & MK_SHIFT)
				b |= 2;
			else
				b |= 4;
		}
		m.xy.x = x;
		m.xy.y = y;
		m.buttons = b;
		m.msec = msec();
		send(cmouse, &m);
		break;

	case WM_CHAR:
		/* repeat count is lparam & 0xf */
		switch(wparam){
		case '\n':
			wparam = '\r';
			break;
		case '\r':
			wparam = '\n';
			break;
		}
		kbdputc(wparam);
		break;

	case WM_SYSKEYUP:
		break;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		switch(wparam) {
		case VK_CONTROL:
			kbdputc(Kctl);
			break;
		case VK_MENU:
			kbdputc(Kalt);
			break;
		case VK_INSERT:
			kbdputc(Kins);
			break;
		case VK_DELETE:
//			kbdputc(Kdel);
			kbdputc(0x7f);	// should have Kdel in keyboard.h
			break;
		case VK_HOME:
			kbdputc(Khome);
			break;
		case VK_PRIOR:
			kbdputc(Kpgup);
			break;
		case VK_NEXT:
			kbdputc(Kpgdown);
			break;
		case VK_END:
			kbdputc(Kend);
			break;
		case VK_UP:
			kbdputc(Kup);
			break;
		case VK_DOWN:
			kbdputc(Kdown);
			break;
		case VK_LEFT:
			kbdputc(Kleft);
			break;
		case VK_RIGHT:
			kbdputc(Kright);
			break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_PALETTECHANGED:
		if((HWND)wparam == hwnd)
			break;
	/* fall through */
	case WM_QUERYNEWPALETTE:
		hdc = GetDC(hwnd);
		SelectPalette(hdc, palette, 0);
		if(RealizePalette(hdc) != 0)
			InvalidateRect(hwnd, nil, 0);
		ReleaseDC(hwnd, hdc);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &paint);
		r.min.x = paint.rcPaint.left;
		r.min.y = paint.rcPaint.top;
		r.max.x = paint.rcPaint.right;
		r.max.y = paint.rcPaint.bottom;
		_flushmemscreen(r);
		EndPaint(hwnd, &paint);
		break;
	case WM_SIZE:
		if(wparam != SIZE_RESTORED && wparam != SIZE_MAXIMIZED)
			break;
		wid = lparam&0xFFFF;
		ht = (lparam>>16)&0xFFFF;
		if(wid != Dx(gscreen->r) || ht != Dy(gscreen->r)) {
			mresized = 1;
			resizescreenimage();
			send(cmouse, &m);
		}
//		fprint(2, "SIZE: %d %d\n", wid, ht);
		break;
	case WM_COMMAND:
	case WM_SETFOCUS:
	case WM_DEVMODECHANGE:
	case WM_WININICHANGE:
	case WM_INITMENU:
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}

void
_xmoveto(Point xy)
{
	POINT pt;

	pt.x = xy.x;
	pt.y = xy.y;
	MapWindowPoints(window, 0, &pt, 1);
	SetCursorPos(pt.x, pt.y);
}

void
cursorarrow(void)
{
	if(hcursor != 0) {
		DestroyCursor(hcursor);
		hcursor = 0;
	}
	SetCursor(LoadCursor(0, IDC_ARROW));
	PostMessage(window, WM_SETCURSOR, (int)window, 0);
}


void
_xsetcursor(Cursor *c)
{
	HCURSOR nh;
	int x, y, h, w;
	uchar *sp, *cp;
	uchar *and, *xor;

	if (c==nil) {
		cursorarrow();
		return;
	}
	h = GetSystemMetrics(SM_CYCURSOR);
	w = (GetSystemMetrics(SM_CXCURSOR)+7)/8;

	and = mallocz(h*w, 1);
	memset(and, 0xff, h*w);
	xor = mallocz(h*w, 1);

	for(y=0,sp=c->set,cp=c->clr; y<16; y++) {
		for(x=0; x<2; x++) {
			and[y*w+x] = ~(*sp|*cp);
			xor[y*w+x] = ~*sp & *cp;
			cp++;
			sp++;
		}
	}
	nh = CreateCursor(inst, -c->offset.x, -c->offset.y,
			GetSystemMetrics(SM_CXCURSOR), h,
			and, xor);
	if(nh != NULL) {
		SetCursor(nh);
		if(hcursor != NULL)
			DestroyCursor(hcursor);
		hcursor = nh;
	}

	free(and);
	free(xor);

	PostMessage(window, WM_SETCURSOR, (int)window, 0);
}

void
setcolor(ulong index, ulong red, ulong green, ulong blue)
{
}


char*
clipreadunicode(HANDLE h)
{
	WCHAR *p;
	int n;
	char *q;
	
	p = GlobalLock(h);
	n = wstrutflen(p)+1;
	q = malloc(n);
	wstrtoutf(q, p, n);
	GlobalUnlock(h);

	return q;
}

char *
clipreadutf(HANDLE h)
{
	char *p;

	p = GlobalLock(h);
	p = strdup(p);
	GlobalUnlock(h);
	
	return p;
}

char*
_xgetsnarf(void)
{
	HANDLE h;
	char *p;

	if(!OpenClipboard(window)) {
		oserror();
		return strdup("");
	}

	if((h = GetClipboardData(CF_UNICODETEXT)))
		p = clipreadunicode(h);
	else if((h = GetClipboardData(CF_TEXT)))
		p = clipreadutf(h);
	else {
		oserror();
		p = strdup("");
	}
	
	CloseClipboard();
	return p;
}

void
_xputsnarf(char *buf)
{
	HANDLE h;
	char *p, *e;
	WCHAR *wp;
	Rune r;
	int n = strlen(buf);

	if(!OpenClipboard(window)) {
		oserror();
		return;
	}

	if(!EmptyClipboard()) {
		oserror();
		CloseClipboard();
		return;
	}

	h = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, (n+1)*sizeof(WCHAR));
	if(h == NULL)
		sysfatal("out of memory");
	wp = GlobalLock(h);
	p = buf;
	e = p+n;
	while(p<e){
		p += chartorune(&r, p);
		*wp++ = r;
	}
	*wp = 0;
	GlobalUnlock(h);

	SetClipboardData(CF_UNICODETEXT, h);

	h = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, n+1);
	if(h == NULL)
		sysfatal("out of memory");
	p = GlobalLock(h);
	memcpy(p, buf, n);
	p[n] = 0;
	GlobalUnlock(h);
	
	SetClipboardData(CF_TEXT, h);

	CloseClipboard();
}

int
atlocalconsole(void)
{
	return 1;
}
