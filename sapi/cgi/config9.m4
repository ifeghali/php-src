dnl
dnl $Id$
dnl

AC_ARG_ENABLE(cgi,
[  --disable-cgi           Disable building CGI version of PHP],
[
  PHP_SAPI_CGI=$enableval
],[
  PHP_SAPI_CGI=yes
])

AC_DEFUN([PHP_TEST_WRITE_STDOUT],[
  AC_CACHE_CHECK(whether writing to stdout works,ac_cv_write_stdout,[
    AC_TRY_RUN([
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#define TEXT "This is the test message -- "
        
main()
{
  int n;

  n = write(1, TEXT, sizeof(TEXT)-1);
  return (!(n == sizeof(TEXT)-1));
}
    ],[
      ac_cv_write_stdout=yes
    ],[
      ac_cv_write_stdout=no
    ],[
      ac_cv_write_stdout=no
    ])
  ])
  if test "$ac_cv_write_stdout" = "yes"; then
    AC_DEFINE(PHP_WRITE_STDOUT, 1, [whether write(2) works])
  fi
])


if test "$PHP_SAPI" = "default"; then
  AC_MSG_CHECKING(for CGI build)
  if test "$PHP_SAPI_CGI" != "no"; then
    AC_MSG_RESULT(yes)

    AC_MSG_CHECKING([for socklen_t in sys/socket.h])
    AC_EGREP_HEADER([socklen_t], [sys/socket.h],
      [AC_MSG_RESULT([yes])
       AC_DEFINE([HAVE_SOCKLEN_T], [1],
        [Define if the socklen_t typedef is in sys/socket.h])],
      AC_MSG_RESULT([no]))

    AC_MSG_CHECKING([for sun_len in sys/un.h])
    AC_EGREP_HEADER([sun_len], [sys/un.h],
      [AC_MSG_RESULT([yes])
       AC_DEFINE([HAVE_SOCKADDR_UN_SUN_LEN], [1],
        [Define if sockaddr_un in sys/un.h contains a sun_len component])],
      AC_MSG_RESULT([no]))

    AC_MSG_CHECKING([whether cross-process locking is required by accept()])
    case "`uname -sr`" in
      IRIX\ 5.* | SunOS\ 5.* | UNIX_System_V\ 4.0)	
        AC_MSG_RESULT([yes])
        AC_DEFINE([USE_LOCKING], [1], 
          [Define if cross-process locking is required by accept()])
      ;;
      *)
        AC_MSG_RESULT([no])
      ;;
    esac

    PHP_ADD_MAKEFILE_FRAGMENT($abs_srcdir/sapi/cgi/Makefile.frag)
    case $host_alias in
      *cygwin* )
        SAPI_CGI_PATH=sapi/cgi/php.exe
        ;;
      * )
        SAPI_CGI_PATH=sapi/cgi/php
        ;;
    esac
    PHP_SUBST(SAPI_CGI_PATH)

    PHP_TEST_WRITE_STDOUT

    INSTALL_IT="@echo \"Installing PHP CGI into: \$(INSTALL_ROOT)\$(bindir)/\"; \$(INSTALL) -m 0755 \$(SAPI_CGI_PATH) \$(INSTALL_ROOT)\$(bindir)/\$(program_prefix)php\$(program_suffix)\$(EXEEXT)"
    PHP_SELECT_SAPI(cgi, program, fastcgi.c cgi_main.c getopt.c, , '$(SAPI_CGI_PATH)')

    case $host_alias in
      *aix*)
        BUILD_CGI="echo '\#! .' > php.sym && echo >>php.sym && nm -BCpg \`echo \$(PHP_GLOBAL_OBJS) \$(PHP_SAPI_OBJS) | sed 's/\([A-Za-z0-9_]*\)\.lo/\1.o/g'\` | \$(AWK) '{ if (((\$\$2 == \"T\") || (\$\$2 == \"D\") || (\$\$2 == \"B\")) && (substr(\$\$3,1,1) != \".\")) { print \$\$3 } }' | sort -u >> php.sym && \$(LIBTOOL) --mode=link \$(CC) -export-dynamic \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) -Wl,-brtl -Wl,-bE:php.sym \$(PHP_RPATHS) \$(PHP_GLOBAL_OBJS) \$(PHP_SAPI_OBJS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) -o \$(SAPI_CGI_PATH)"
        ;;
      *darwin*)
        BUILD_CGI="\$(CC) \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(NATIVE_RPATHS) \$(PHP_GLOBAL_OBJS:.lo=.o) \$(PHP_SAPI_OBJS:.lo=.o) \$(PHP_FRAMEWORKS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) -o \$(SAPI_CGI_PATH)"
      ;;
      *)
        BUILD_CGI="\$(LIBTOOL) --mode=link \$(CC) -export-dynamic \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(PHP_RPATHS) \$(PHP_GLOBAL_OBJS) \$(PHP_SAPI_OBJS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) -o \$(SAPI_CGI_PATH)"
      ;;
    esac

    PHP_SUBST(BUILD_CGI)

  elif test "$PHP_SAPI_CLI" != "no"; then
    AC_MSG_RESULT(no)
    OVERALL_TARGET=
    PHP_SAPI=cli   
  else
    AC_MSG_ERROR([No SAPIs selected.])  
  fi
fi
