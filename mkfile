MKSHELL=rc

pop=+P9

all:VQ:
	echo 'targets: eq, rm(equals), pop(ulate)'

ls:VQ:
	hg manifest | grep -v '\([.]\(ed\|mk\)\|mkfile\)'

pop:V:	populate
rm:V:	rmequals

P9:VQ:
	test x^$PLAN9 '!=' x
	test -d $PLAN9

populate:VQ:	P9 EQ
	echo '*' copying identical files from p9p
	if (test -f $pop)
		exit 1
	cat EQ | sed '/,/d' | @{cd $PLAN9 && tar cf - `{cat} } | tar xf -
	date > $pop
	cat EQ | sed 's/,/ /g' | while(e=`{read}) {
		o=$PLAN9/$e(1)
		if(~ $#e 2){
			i=$e(2)
			cp $o $i
		}
		if not
			i=$e(1)
		if (test -f $i.ed){
			{
				cat $i.ed
				echo w $i
				echo q
			} | ed $o >[2=1] | egrep -v '^[0-9?]+$' || true
	
			# check sha1sum
			line=`{cat $i.ed | grep '^0s/^sha1,' | tr /, ' '}
			sum=`{sha1sum $i}
			~ $sum(1) $line(3) && s='' || s='		!'
			echo appling $i.ed$s
		}
	}
	echo

rmequals:V:	$pop EQ
	rm -f `{cat EQ | sed 's/.*,//'}
	rm -f $pop


./,,f:
	find include src -type f | grep '/[^/]*\.\([chysS]\|lx\|spp\|utf\|lib\|pdf\|ps\|tr\)$' > ,,f
	find src -type f \
		| grep '/\(README.*\|portdate\|reduce\|utfmap\|cvt\|find\|unansi\|mkfile\|COPYRIGHT\|NOTICE\|FIXES\|Root\|Repository\|Entries\|mkfile\|\.cvsignore\)$' >> ,,f
	find lib | grep '\(lex\|yacc\).*' >> ,,f || true

eq:V:	P9 ./,,f uned
	rm -f ,EQ
	test -f EQ && mv EQ ,EQ
	
	{
		fn testeq{
			o=`{echo $1 | sed s,$2,}
			if (cmp -s $1 $PLAN9/$o)
				echo $o,$1
		}
		for (i in `{cat ,,f}){
			if (cmp -s $i $PLAN9/$i)
				echo $i
			if not {
				testeq $i lib9/linux,lib9 ||
				testeq $i devdraw/x11/,devdraw/x11- ||
				testeq $i devdraw/x11/x11-,devdraw/x11- ||
				testeq $i 9term/linux,9term/Linux ||
				~ 0 0
			}
		}
	} > EQ
	rm ,,f

tkdiff:VQ:	./,,f
	for (i in `{hg manifest}){
		if (! echo $i | grep '\(CVS\|mkfile\)' >/dev/null)
		if (test -f $i)
		if (test -f $PLAN9/$i)
		if (! cmp -s $i $PLAN9/$i)
			echo tkdiff $PLAN9/$i $i
	}
	rm ,,f

tkdiffed:VQ:	./,,f
	for (i in `{hg manifest}){
		e = `{echo $i | sed 's,[.]ed$,,'}
		if(! ~ $e $i)
		if (! echo $i | grep '\(CVS\|mkfile\)' >/dev/null)
		if (test -f $e)
		if (test -f $PLAN9/$e)
		if (! cmp -s $e $PLAN9/$e)
			echo tkdiff $PLAN9/$e $e
	}
	rm ,,f

eqdiff:VQ:	P9 EQ
	cat EQ | sed 's/,/	/g' | while(e=`{read}) {
		o=$PLAN9/$e(1)
		if(~ $#e 2)
			i=$e(2)
		if not
			i=$e(1)
		if (test -f $i)
		if (test -f $o){
			if (test -f $i.ed){
				rm -f ,,eqf
				{
					cat $i.ed
					echo w ,,eqf
					echo q
				} | ed $o >[2=1] | egrep -v '^[0-9?]+$' || true
				if (! cmp -s ,,eqf $i)
					echo tkdiff $o $i	'# '$i.ed
			}
			if not{
				if (! cmp -s $i $o)
					echo tkdiff $o $i
			}
			
		}
	}
	echo

upded:VQ: 	P9 
	echo '*' updating ed files
	cat EQ | sed 's/,/	/g' | while(e=`{read}) {
		o=$PLAN9/$e(1)
		if(~ $#e 2)
			i=$e(2)
		if not
			i=$e(1)
	
		if(test -f $i)
		if(test -f $o)
		if(! cmp -s $i $o){
			echo creating $i.ed
			9 diff -e $o $i > $i.ed || true
			sha1sum $i | awk '{print "0s/^sha1," $1 "/" }' >> $i.ed
		}
	}
	echo

uned:VQ: 	P9 EQ upded
	echo '*' unapply ed files
	for (i in `{cat EQ}){
		if (! test -f $PLAN9/$i){
			tmp=$i
			o=$PLAN9/^`{echo $tmp | sed 's/,.*//'}
			i=`{echo $tmp | sed 's/.*,//'}
		}
		if not o=$PLAN9/$i
		if (test -f $i)
		if (test -f $o)
		if (! cmp -s $i $o) {
			echo reverting $i
			cp $o $i
		}
	}
	echo

showed:VQ:
	hg stat | grep '\.ed$'
