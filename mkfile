MKSHELL=rc

all:VQ:
	echo "targets: eq, rm(equals), pop(ulate)"

pop:V:	populate
rm:V:	rmequals

populate:V:	EQ
	@{cd $PLAN9 && tar cf - `{cat} } < EQ | tar xf -

rmequals:V:	EQ
	rm -f `{cat EQ}


,,f:
	find include src -type f -name '*.[chysS]' > ,,f
	find src -type f \
		| grep '/\(README.*\|portdate\|mkfile\|COPYRIGHT\|NOTICE\|Root\|Repository\|Entries\|mkfile\|\.cvsignore\)$' >> ,,f


eq:V:	,,f
	rm -f ,EQ
	test -f EQ && mv EQ ,EQ
	
	{
		for (i in `{cat ,,f}){
			if (cmp -s $i $PLAN9/$i)
				echo $i
		}
	} > EQ
	rm ,,f

tkdiff:V:	,,f
	for (i in `{cat ,,f})
		if (! echo $i | grep '\(CVS\|mkfile\)' >/dev/null)
		if (test -f $i)
		if (test -f $PLAN9/$i)
		if (! diff $i $PLAN9/$i >/dev/null >[2=1])
			echo tkdiff $PLAN9/$i $i
	rm ,,f
