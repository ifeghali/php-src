dnl $Id$

AC_MSG_CHECKING(for ASPELL support)
AC_ARG_WITH(aspell,
[  --with-aspell[=DIR]     Include ASPELL support.],
[
  if test "$withval" != "no"; then
    if test "$withval" = "yes"; then
      ASPELL_DIR=/usr/local
    else
      ASPELL_DIR=$withval
    fi

    AC_ADD_INCLUDE($ASPELL_DIR/include)
    AC_ADD_LIBRARY_WITH_PATH(aspell, $ASPELL_DIR/lib)

    if test ! -f "$ASPELL_DIR/include/aspell-c.h"; then
      AC_MSG_ERROR(Could not find aspell-c.h in $ASPELL_DIR/include - please copy it manually from the aspell sources to $ASPELL_DIR/include)
    fi
    AC_DEFINE(HAVE_ASPELL)
    AC_MSG_RESULT(yes)
	PHP_EXTENSION(aspell)
  else
    AC_MSG_ERROR(no)
  fi
],[
  AC_MSG_RESULT(no)
])
