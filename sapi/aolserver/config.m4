dnl ## $Id: config.m4,v 1.6 1999/12/29 21:24:37 sas Exp $ -*- sh -*-

RESULT=no
AC_MSG_CHECKING(for AOLserver support)
AC_ARG_WITH(aolserver,
[  --with-aolserver=DIR],
[
	if test ! -d $withval ; then
		AC_MSG_ERROR(You did not specify a directory)
	fi
	PHP_BUILD_THREAD_SAFE
	NS_DIR=$withval
	AC_ADD_INCLUDE($NS_DIR/include)
	AC_DEFINE(HAVE_AOLSERVER,1,[Whether you have AOLserver])
	PHP_SAPI=aolserver
	PHP_BUILD_SHARED
	INSTALL_IT="\$(INSTALL) -m 0755 $SAPI_SHARED $NS_DIR/root/bin/"
	RESULT=yes
])
AC_MSG_RESULT($RESULT)

dnl ## Local Variables:
dnl ## tab-width: 4
dnl ## End:
