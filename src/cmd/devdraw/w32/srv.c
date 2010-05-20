/*
 * Window system protocol server.
 * Use select and a single proc and single stack
 * to avoid aggravating the X11 library, which is
 * subtle and quick to anger.
 */

#include <u.h>
#include <mingwsock.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <memdraw.h>
#include <memlayer.h>
#include <keyboard.h>
#include <mouse.h>
#include <cursor.h>
#include <drawfcall.h>
#include "devdraw.h"
#include "if.h"

extern	int	utoplan9kbd(int);

enum {
	STACK	= 32768,
};

#define MouseMask (\
	ButtonPressMask|\
	ButtonReleaseMask|\
	PointerMotionMask|\
	Button1MotionMask|\
	Button2MotionMask|\
	Button3MotionMask)

#define Mask MouseMask|ExposureMask|StructureNotifyMask|KeyPressMask|EnterWindowMask|LeaveWindowMask

typedef struct Kbdbuf Kbdbuf;
typedef struct Mousebuf Mousebuf;
typedef struct Fdbuf Fdbuf;
typedef struct Tagbuf Tagbuf;

struct Kbdbuf
{
	Rune r[32];
	int ri;
	int wi;
	int stall;
};

struct Mousebuf
{
	Mouse m[32];
	int ri;
	int wi;
	int stall;
};

struct Tagbuf
{
	int t[32];
	int ri;
	int wi;
};

struct Fdbuf
{
	uchar buf[2*MAXWMSG];
	uchar *rp;
	uchar *wp;
	uchar *ep;
};

Kbdbuf kbd;
Mousebuf mouse;
int mresized;
Fdbuf fdin;
Fdbuf fdout;
Tagbuf kbdtags;
Tagbuf mousetags;

void fdslide(Fdbuf*);
void runmsg(Wsysmsg*);
void replymsg(Wsysmsg*);
void matchkbd(void);
void matchmouse(void);
int fdnoblock(int);

int chatty;
int drawsleep;
int fullscreen;

Rectangle windowrect;
Rectangle screenrect;

Channel *cmouse;
Channel *ckbd;

static Channel *cwaitmore;

static int cfd;

void
kbdthread(void *ch)
{
	int	c;

	for(;;) {
		c = recvul(ch);
		if(kbd.stall)
			continue;
		c = utoplan9kbd(c);
		if (c<0)
			continue;
		kbd.r[kbd.wi++] = c;
		if(kbd.wi == nelem(kbd.r))
			kbd.wi = 0;
		if(kbd.ri == kbd.wi)
			kbd.stall = 1;
		matchkbd();
	}
}
void
mousethread(void *ch)
{
	Mouse m;

	for(;;) {
		recv(ch, &m);
		if(mouse.stall)
			continue;
		mouse.m[mouse.wi] = m;
		mouse.wi++;
		if(mouse.wi == nelem(mouse.m))
			mouse.wi = 0;
		if(mouse.wi == mouse.ri){
			mouse.stall = 1;
			mouse.ri = 0;
			mouse.wi = 1;
			mouse.m[0] = m;
			/* fprint(2, "mouse stall\n"); */
		}
		matchmouse();
	}
}

void
writethread(void *cnotew)
{
	int n;
	Ioproc *io;
	
	io = ioproc();
	for(;;) {
		/* write what we can */
		n = 1;
		while(fdout.rp < fdout.wp && (n = iowrite(io, cfd, fdout.rp, fdout.wp-fdout.rp)) > 0)
			fdout.rp += n;
		if(n == 0)
			sysfatal("short write writing wsys");
		if(n < 0)
			sysfatal("writing wsys msg: %r");

		/* slide data to beginning of buf */
		fdslide(&fdout);

		/* tell the reading thread that something has been written */
		nbsendul(cnotew, 1);

		if (fdout.rp >= fdout.wp)
			recvul(cwaitmore);
	}
}

void
usage(void)
{
	fprint(2, "usage: devdraw (don't run directly)\n");
	threadexitsall("usage");
}

void
bell(void *v, char *msg)
{
	if(strcmp(msg, "alarm") == 0)
		drawsleep = drawsleep ? 0 : 1000;
	noted(NCONT);
}

void
threadmain(int argc, char **argv)
{
	int fd[2];
	int n, top;
	fd_set rd, wr, xx;
	Wsysmsg m;
	Channel *cnotew;
	Ioproc *io;

	pipe(fd);
	if(sendfd(1, fd[1]) == -1)
		sysfatal("sendfd: %r");	
	cfd = fd[0];

	/*
	 * Move the protocol off stdin/stdout so that
	 * any inadvertent prints don't screw things up.
	 */
	close(0);
	close(1);
	open("/dev/null", OREAD);
	open("/dev/null", OWRITE);

	fmtinstall('H', encodefmt);
	fmtinstall('W', drawfcallfmt);

	ARGBEGIN{
	case 'D':
		chatty++;
		break;
	default:
		usage();
	}ARGEND

	/*
	 * Ignore arguments.  They're only for good ps -a listings.
	 */
	
	notify(bell);

	fdin.rp = fdin.wp = fdin.buf;
	fdin.ep = fdin.buf+sizeof fdin.buf;
	
	fdout.rp = fdout.wp = fdout.buf;
	fdout.ep = fdout.buf+sizeof fdout.buf;

	ckbd = chancreate(sizeof(ulong), 32);
	cmouse = chancreate(sizeof(Mouse), 32);

	threadcreate(kbdthread, ckbd, STACK);
	threadcreate(mousethread, cmouse, STACK);

	cwaitmore = chancreate(sizeof(ulong), 0);
	cnotew = chancreate(sizeof(ulong), 0);
	threadcreate(writethread, cnotew, STACK);

	io = ioproc();
	for(;;){
		/*
		 * Don't read unless there's room *and* we haven't
		 * already filled the output buffer too much.
		 */
		while(fdout.wp > fdout.buf+MAXWMSG) 
			recvul(cnotew);

		/* read what we can */
		n = 1;
		if (fdin.wp < fdin.ep) {
			n = ioread(io, cfd, fdin.wp, fdin.ep-fdin.wp);
			if (n>0)
				fdin.wp += n;
		}
		if(n == 0){
			if(chatty)
				fprint(2, "eof\n");
			closeioproc(io);
			threadexitsall(0);
		}
		if(n < 0)
			sysfatal("reading wsys msg: %r");

		/* pick off messages one by one */
		while((n = convM2W(fdin.rp, fdin.wp-fdin.rp, &m)) > 0){
//			fprint(2, "<- %W\n", &m);
			runmsg(&m);
			fdin.rp += n;
		}
			
		/* slide data to beginning of buf */
		fdslide(&fdin);
	}
}

void
fdslide(Fdbuf *fb)
{
	int n;

	n = fb->wp - fb->rp;
	if(n > 0)
		memmove(fb->buf, fb->rp, n);
	fb->rp = fb->buf;
	fb->wp = fb->rp+n;
}

void
replyerror(Wsysmsg *m)
{
	char err[256];
	
	rerrstr(err, sizeof err);
	m->type = Rerror;
	m->error = err;
	replymsg(m);
}



/* 
 * Handle a single wsysmsg. 
 * Might queue for later (kbd, mouse read)
 */
void
runmsg(Wsysmsg *m)
{
	uchar buf[65536];
	int n;
	Memimage *i;

	switch(m->type){
	case Tinit:
		memimageinit();
		i = _xattach(m->label, m->winsize);
		_initdisplaymemimage(i);
		replymsg(m);
		break;

	case Trdmouse:
		mousetags.t[mousetags.wi++] = m->tag;
		if(mousetags.wi == nelem(mousetags.t))
			mousetags.wi = 0;
		if(mousetags.wi == mousetags.ri)
			sysfatal("too many queued mouse reads");
		/* fprint(2, "mouse unstall\n"); */
		mouse.stall = 0;
		matchmouse();
		break;

	case Trdkbd:
		kbdtags.t[kbdtags.wi++] = m->tag;
		if(kbdtags.wi == nelem(kbdtags.t))
			kbdtags.wi = 0;
		if(kbdtags.wi == kbdtags.ri)
			sysfatal("too many queued keyboard reads");
		kbd.stall = 0;
		matchkbd();
		break;

	case Tmoveto:
		_xmoveto(m->mouse.xy);
		replymsg(m);
		break;

	case Tcursor:
		if(m->arrowcursor)
			_xsetcursor(nil);
		else
			_xsetcursor(&m->cursor);
		replymsg(m);
		break;
			
	case Tbouncemouse:
		_xbouncemouse(&m->mouse);
		replymsg(m);
		break;

	case Tlabel:
		_xsetlabel(m->label);
		replymsg(m);
		break;

	case Trdsnarf:
		m->snarf = _xgetsnarf();
		replymsg(m);
		free(m->snarf);
		break;

	case Twrsnarf:
		_xputsnarf(m->snarf);
		replymsg(m);
		break;

	case Trddraw:
		n = m->count;
		if(n > sizeof buf)
			n = sizeof buf;
		n = _drawmsgread(buf, n);
		if(n < 0)
			replyerror(m);
		else{
			m->count = n;
			m->data = buf;
			replymsg(m);
		}
		break;

	case Twrdraw:
		if(_drawmsgwrite(m->data, m->count) < 0)
			replyerror(m);
		else
			replymsg(m);
		break;
	
	case Ttop:
		_xtopwindow();
		replymsg(m);
		break;
	
	case Tresize:
		_xresizewindow(m->rect);
		replymsg(m);
		break;
	}
}

/*
 * Reply to m.
 */
void
replymsg(Wsysmsg *m)
{
	int n;

	/* T -> R msg */
	if(m->type%2 == 0)
		m->type++;

//	fprint(2, "-> %W\n", m);
	/* copy to output buffer */
	n = sizeW2M(m);
	if(fdout.wp+n > fdout.ep)
		sysfatal("out of space for reply message");
	convW2M(m, fdout.wp, n);
	fdout.wp += n;
	nbsendul(cwaitmore, 1);
}

/*
 * Match queued kbd reads with queued kbd characters.
 */
void
matchkbd(void)
{
	Wsysmsg m;
	
	if(kbd.stall)
		return;
	while(kbd.ri != kbd.wi && kbdtags.ri != kbdtags.wi){
		m.type = Rrdkbd;
		m.tag = kbdtags.t[kbdtags.ri++];
		if(kbdtags.ri == nelem(kbdtags.t))
			kbdtags.ri = 0;
		m.rune = kbd.r[kbd.ri++];
		if(kbd.ri == nelem(kbd.r))
			kbd.ri = 0;
		replymsg(&m);
	}
}

/*
 * Match queued mouse reads with queued mouse events.
 */
void
matchmouse(void)
{
	Wsysmsg m;
	
	while(mouse.ri != mouse.wi && mousetags.ri != mousetags.wi){
		m.type = Rrdmouse;
		m.tag = mousetags.t[mousetags.ri++];
		if(mousetags.ri == nelem(mousetags.t))
			mousetags.ri = 0;
		m.mouse = mouse.m[mouse.ri];
		m.resized = mresized;
		/*
		if(m.resized)
			fprint(2, "sending resize\n");
		*/
		mresized = 0;
		mouse.ri++;
		if(mouse.ri == nelem(mouse.m))
			mouse.ri = 0;
		replymsg(&m);
	}
}

int
_xsetlabel(char *label)
{
	return 0;
}
void
_xbouncemouse(Mouse *m)
{
}

