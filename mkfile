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
	@{cd $PLAN9 && tar cf - `{cat} } < EQ | tar xf -
	date > $pop
	for (i in `{cat EQ})
		if (test -f $i.ed){
			echo appling $i.ed
			{
				cat $i.ed
				echo w $i
				echo q
			} | ed $PLAN9/$i >[2=1] | egrep -v '^[0-9?]+$' || true
		}
	echo

rmequals:V:	$pop EQ
	rm -f `{cat EQ}
	rm -f $pop


,,f:
	find include src -type f | grep '/[^/]*\.\([chysS]\|lx\|spp\)$' > ,,f
	find src -type f \
		| grep '/\(README.*\|portdate\|mkfile\|COPYRIGHT\|NOTICE\|Root\|Repository\|Entries\|mkfile\|\.cvsignore\)$' >> ,,f


eq:V:	P9 ,,f uned
	rm -f ,EQ
	test -f EQ && mv EQ ,EQ
	
	{
		for (i in `{cat ,,f}){
			if (cmp -s $i $PLAN9/$i)
				echo $i
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
	for (i in `{cat EQ})
		if (test -f $i)
		if (test -f $PLAN9/$i)
		if (! cmp -s $i $PLAN9/$i)
			echo tkdiff $PLAN9/$i $i
	echo

upded:VQ: 	P9 EQ
	echo '*' updating ed files
	for (i in `{cat EQ})
		if (test -f $i)
		if (test -f $PLAN9/$i)
		if (! cmp -s $i $PLAN9/$i) {
			echo creating $i.ed
			9 diff -e $PLAN9/$i $i > $i.ed || true
		}
	echo

uned:VQ: 	P9 EQ upded
	echo '*' unapply ed files
	for (i in `{cat EQ})
		if (test -f $i)
		if (test -f $PLAN9/$i)
		if (! cmp -s $i $PLAN9/$i) {
			echo reverting $i
			cp $PLAN9/$i $i
		}
	echo
