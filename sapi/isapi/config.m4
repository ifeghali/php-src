dnl
dnl $Id$
dnl

RESULT=no
AC_MSG_CHECKING(for Zeus ISAPI support)
AC_ARG_WITH(isapi,
[  --with-isapi[=DIR]      Build PHP as an ISAPI module for use with Zeus], [
  PHP_ISAPI=$withval
], [
  PHP_ISAPI=no
])

if test "$PHP_ISAPI" != "no"; then
  if test "$PHP_ISAPI" = "yes"; then
    ZEUSPATH=/usr/local/zeus # the default
  else
    ZEUSPATH=$PHP_ISAPI
  fi
  test -f "$ZEUSPATH/web/include/httpext.h" || AC_MSG_ERROR(Unable to find httpext.h in $ZEUSPATH/web/include)
  PHP_BUILD_THREAD_SAFE
  AC_DEFINE(WITH_ZEUS,1,[ ])
  PHP_ADD_INCLUDE($ZEUSPATH/web/include)
  PHP_SELECT_SAPI(isapi, shared, php5isapi.c)
  INSTALL_IT="\$(SHELL) \$(srcdir)/install-sh -m 0755 $SAPI_SHARED \$(INSTALL_ROOT)$ZEUSPATH/web/bin/"
  RESULT=yes
])
AC_MSG_RESULT($RESULT)

dnl ## Local Variables:
dnl ## tab-width: 4
dnl ## End:
