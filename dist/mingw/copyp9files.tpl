objtype=mingw
tmp=/tmp/,,pf9p9pfiles

odir=`{pwd}
mkdir $tmp
cd $tmp

if (test -d $PLAN9)
	p9pdir=$PLAN9
if not
	p9pdir = /usr/local/plan9

echo copying files from $p9pdir
@{cd $p9pdir && tar cf - $copyfiles} | tar xf -

a=$odir/pf9-$objtype.lib.zip
echo creating $a
9 zip -9 -f $a  $zipfiles

cd $odir
rm -rf $tmp
