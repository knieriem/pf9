warn=false
opti=:
opts=
outfile=
has_src=false
srcs=
avoid_outfile=false
while test $# -ne 0; do
	case $1 in
	-o)
		outfile=$2
		shift
		;;
	-E)
		avoid_outfile=:
		opts="$opts $1"
		;;
	-[Nw]*)
		if echo "x$1" | grep N > /dev/null; then
			opti=false
		fi
		if echo "x$1" | grep w > /dev/null; then
			warn=:
		fi
		;;
	-*)
		opts="$opts $1"
		;;
	*)
		if $has_src && test "$outfile"; then
			echo "$frontend: if option -o is specified, only one source file is allowed" >&2
			exit 1
		fi
		srcs="$srcs $1"
		has_src=:
		;;
	esac
	shift
done
