
if test "$PHP_SAPI" = "cgi"; then

  PHP_ARG_ENABLE(force-cgi-redirect,whether to force Apache CGI redirect,
[  --enable-force-cgi-redirect
                          Enable the security check for internal server
                          redirects.  You should use this if you are
                          running the CGI version with Apache. ])

  if test "$PHP_FORCE_CGI_REDIRECT" = "yes"; then
    AC_DEFINE(FORCE_CGI_REDIRECT, 1, [ ])
    REDIRECT=1
  else
    AC_DEFINE(FORCE_CGI_REDIRECT, 0, [ ])
    REDIRECT=0
  fi

  PHP_ARG_ENABLE(discard-path,whether to discard path_info + path_translated,
[  --enable-discard-path   If this is enabled, the PHP CGI binary
                          can safely be placed outside of the
                          web tree and people will not be able
                          to circumvent .htaccess security. ])

  if test "$PHP_DISCARD_PATH" = "yes"; then
    AC_DEFINE(DISCARD_PATH, 1, [ ])
    DISCARD_PATH=1
  else
    AC_DEFINE(DISCARD_PATH, 0, [ ])
    DISCARD_PATH=0
  fi

fi

if test "$PHP_SAPI" = "cgi" ; then
  INSTALL_IT="\$(INSTALL) -m 0755 $SAPI_PROGRAM \$(bindir)/$SAPI_CGI"
fi

AC_MSG_CHECKING(for fhttpd module support)
AC_ARG_WITH(fhttpd,
[  --with-fhttpd[=DIR]     Build fhttpd module.  DIR is the fhttpd sources
                          directory, defaults to /usr/local/src/fhttpd.],
[
    if test "$withval" = "yes"; then
      # fhttpd source directory
      withval=/usr/local/src/fhttpd
    fi
    if test "$withval" != "no"; then
# For fhttpd 0.3.x
      if test -f $withval/servproc.h; then 
        FHTTPD_INCLUDE=-I$withval/
        FHTTPD_LIB=$withval/servproc.o
        FHTTPD_TARGET=$withval/
        PHP_BUILD_STATIC
		PHP_SAPI=cgi
        AC_DEFINE(FHTTPD,1,[ ])
        AC_MSG_RESULT(yes - fhttpd 0.3.x)
      else
        AC_MSG_RESULT(no)
        AC_MSG_ERROR(Invalid fhttpd directory - unable to find servproc.h under $withval)
      fi
    else
      AC_MSG_RESULT(no)
    fi
],[
    AC_MSG_RESULT(no)
])
INCLUDES="$INCLUDES $FHTTPD_INCLUDE"
dnl## AC_SUBST(FHTTPD_INCLUDE)
AC_SUBST(FHTTPD_LIB)
AC_SUBST(FHTTPD_TARGET)
