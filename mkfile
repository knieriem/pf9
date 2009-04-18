MKSHELL=rc

pop=+P9

all:VQ:
	echo "targets: eq, rm(equals), pop(ulate)"

ls:VQ:
	hg manifest | grep -v '\(\.ed\|mkfile\)'

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
	for (i in `{cat EQ}){
		if (test -f $PLAN9/$i)
			o=$PLAN9/$i
		if not {
			tmp=$i
			o=$PLAN9/^`{echo $tmp | sed 's/,.*//'}
			i=`{echo $tmp | sed 's/.*,//'}
			cp $o $i
		}
		if (test -f $i.ed){
			echo appling $i.ed
			{
				cat $i.ed
				echo w $i
				echo q
			} | ed $o >[2=1] | egrep -v '^[0-9?]+$' || true
		}
	}
	echo

rmequals:V:	$pop EQ
	rm -f `{cat EQ | sed 's/.*,//'}
	rm -f $pop


,,f:
	find include src -type f | grep '/[^/]*\.\([chysS]\|lx\|spp\|utf\|pdf\|ps\|tr\)$' > ,,f
	find src -type f \
		| grep '/\(README.*\|portdate\|mkfile\|COPYRIGHT\|NOTICE\|Root\|Repository\|Entries\|mkfile\|\.cvsignore\)$' >> ,,f
	find lib | grep '\(lex\|yacc\).*' >> ,,f

eq:V:	P9 ,,f uned
	rm -f ,EQ
	test -f EQ && mv EQ ,EQ
	
	{
		for (i in `{cat ,,f}){
			if (cmp -s $i $PLAN9/$i)
				echo $i
			if not {
				o=`{echo $i | sed 's,lib9/linux,lib9,'}
				if (cmp -s $i $PLAN9/$o)
					echo $o,$i
				if not {
					o=`{echo $i | sed 's,devdraw/x11/,devdraw/x11-,'}
					if (cmp -s $i $PLAN9/$o)
						echo $o,$i
					if not {
						o=`{echo $i | sed 's,devdraw/x11/x11-,devdraw/x11-,'}
						if (cmp -s $i $PLAN9/$o)
							echo $o,$i
					}
				}
			}
		}
	} > EQ
	rm ,,f

tkdiff:VQ:	,,f
	for (i in `{hg manifest})
		if (! echo $i | grep '\(CVS\|mkfile\)' >/dev/null)
		if (test -f $i)
		if (test -f $PLAN9/$i)
		if (! cmp -s $i $PLAN9/$i)
			echo tkdiff $PLAN9/$i $i
	rm ,,f

eqdiff:VQ:	P9 EQ
	for (i in `{cat EQ}){
		if (! test -f $PLAN9/$i){
			tmp=$i
			o=$PLAN9/`{echo $tmp | sed 's/,.*//'}
			i=`{echo $tmp | sed 's/.*,//'}
		}
		if not o=$PLAN9/$i
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

upded:VQ: 	P9 EQ
	echo '*' updating ed files
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
			echo creating $i.ed
			9 diff -e $o $i > $i.ed || true
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
