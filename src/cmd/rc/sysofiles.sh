case $objsys in
mingw)
	echo	haventfork.$O
	;;
linux)
	echo havefork.$O
	;;
esac
echo $objsys.$O

