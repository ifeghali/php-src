#! /bin/sh

# $Id$
# replacement for genif.pl

infile="$1"
shift
srcdir="$1"
shift

if test "$infile" = "" -o "$srcdir" = ""; then
	echo "please supply infile and srcdir"
	exit 1
fi

module_ptrs=""
includes=""

for ext in ${1+"$@"} ; do
	module_ptrs="	phpext_${ext}_ptr,\\\n$module_ptrs"
	for pre in php3 php php4 zend; do
		hdrfile="ext/$ext/${pre}_${ext}.h"
		if test -f $hdrfile ; then
			includes="#include \"$hdrfile\"\\\n$includes"
		fi
	done
done

cat $infile | \
	sed \
	-e "s'@EXT_INCLUDE_CODE@'$includes'" \
	-e "s'@EXT_MODULE_PTRS@'$module_ptrs'" \
	-e 's/\\n/\
/g'


