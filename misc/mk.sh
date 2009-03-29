#! /bin/sh

mkbin=/usr/local/plan9/bin/mk

# check whether we are inside of a pf9 tree,
# and try to guess a value for PF9

if pwd | grep sys/src > /dev/null; then

	base=`pwd | sed 's,/sys/src.*,,'`

	if test -f "$base/pf9.defaults"; then
		user_objtype=$objtype
		user_cputype=$cputype
		user_sysname=$sysname

		. $base/pf9.defaults

		test -z "$user_objtype" || objtype=$user_objtype
		test -z "$user_cputype" || cputype=$user_cputype
		test -z "$user_sysname" || sysname=$user_sysname
		PF9=$base
		
		export \
			PF9 \
			cputype \
			objtype \
			sysname \

		if test -f $PF9/$cputype/bin/mk; then
			mkbin=$PF9/$cputype/bin/mk
		fi
	fi
fi


# try to make rc runable
if test -z "$PLAN9"; then
	PLAN9=$PF9/$cputype
	export PLAN9
elif test -d "$PLAN9"; then :
else
	PLAN9=$PF9/$cputype
	export PLAN9
fi

exec $mkbin "$@"
