#!/bin/sh

[ -f $PLAN9/config ] && . $PLAN9/config

if [ "x$X11" = "x" ]; then 
	if [ -d /usr/X11R6 ]; then
		X11=/usr/X11R6
	elif [ -d /usr/local/X11R6 ]; then
		X11=/usr/local/X11R6
	elif [ -d /usr/X ]; then
		X11=/usr/X
	elif [ -d /usr/openwin ]; then	# for Sun
		X11=/usr/openwin
	elif [ -d /usr/include/X11 ]; then
		X11=/usr
	elif [ -d /usr/local/include/X11 ]; then
		X11=/usr/local
	else
		X11=noX11dir
	fi
fi

if [ "x$WSYSTYPE" = "x" ]; then
	if [ $objsys = mingw ]; then
		WSYSTYPE=w32
		X11=w32
	elif [ -d "$X11" ]; then
		WSYSTYPE=x11
	else
		WSYSTYPE=nowsys
	fi
fi

if [ "x$WSYSTYPE" = "xx11" -a "x$X11H" = "x" ]; then
	if [ -d "$X11/include" ]; then
		X11H="-I$X11/include"
	else
		X11H=""
	fi
fi
	

echo 'WSYSTYPE='$WSYSTYPE
echo 'X11='$X11

if [ $WSYSTYPE = x11 ]; then
	echo 'CFLAGS=$CFLAGS '$X11H
	echo 'HFILES=$HFILES $XHFILES'
	XO=`ls x11/*.c 2>/dev/null | sed 's,x11/,x11-,;s/\.c$/.$O/'`
	echo 'WSYSOFILES=$WSYSOFILES '$XO
	echo 'WSYS=x11'
fi
if [ $WSYSTYPE = w32 ]; then
	XO=`ls w32/*.c 2>/dev/null | sed 's,w32/,w32-,;s/\.c$/.$O/'`
	echo 'WSYSOFILES=$WSYSOFILES glenda.$O '$XO
	echo 'WSYS=w32'
fi
if [ $WSYSTYPE = nowsys ]; then
	echo 'WSYSOFILES=nowsys.o'
fi
