#!/bin/sh

case $objsys in
linux)
	echo unix.$O
	;;
mingw)
	echo mingw.$O
	;;
esac
