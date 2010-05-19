#!/bin/sh

case $objsys in
linux)
	echo pthread.$O
	;;
mingw)
	echo $objsys.$O
	echo $objsys-asm.$O
	;;
esac
