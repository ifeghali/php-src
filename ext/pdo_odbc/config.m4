dnl $Id$
dnl config.m4 for extension pdo_odbc
dnl vim:et:sw=2:ts=2:

AC_DEFUN(PDO_ODBC_HELP_TEXT, [[
 --with-pdo-odbc=flavour,dir  Add support for "flavour" ODBC drivers,
                              looking for include and lib dirs under "dir"
         
        flavour can be one of:
           ibm-db2, unixODBC, ODBCRouter, generic

        You may omit the ,dir part to use a reasonable default for
        the flavour you have selected. e.g.:
            --with-pdo-odbc=unixODBC
        will check for unixODBC under /usr/local

        You may attempt to use an otherwise unsupported driver using
        the generic flavour.  The syntax for generic ODBC support is:
            --with-pdo-odbc=generic,dir,ldflags,cflags

        This extension will always be created as a shared extension
        named pdo_odbc.so
]])

PHP_ARG_WITH(pdo-odbc,  ODBC v3 driver for PDO, PDO_ODBC_HELP_TEXT)

AC_DEFUN([PDO_ODBC_CHECK_HEADER],[
  if test -f $PDO_ODBC_INCDIR/$1 ; then
    AC_DEFINE([HAVE_]translit($1,a-z_.-,A-Z___), 1, [ ])
  fi
])
                                  
AC_MSG_CHECKING(which ODBC flavour you want)
if test "$PHP_PDO_ODBC" != "no" && test "$PHP_PDO_ODBC" != "yes" ; then

  AC_MSG_CHECKING([for PDO includes])
  if test -f $prefix/include/php/ext/pdo/php_pdo_driver.h; then
    pdo_inc_path=$prefix/include/php/ext
  elif test -f $abs_srcdir/include/php/ext/pdo/php_pdo_driver.h; then
    pdo_inc_path=$abs_srcdir/ext
  elif test -f ext/pdo/php_pdo_driver.h; then
    pdo_inc_path=ext
  else
    AC_MSG_ERROR([Cannot find php_pdo_driver.h.])
  fi
  AC_MSG_RESULT($pdo_inc_path)

  pdo_odbc_flavour=`echo $withval | cut -d, -f1`
  pdo_odbc_dir=`echo $withval | cut -d, -f2`

  if test "$pdo_odbc_dir" = "$withval" ; then
    pdo_odbc_dir=""
  fi

  case $pdo_odbc_flavour in
    ibm-db2)
        pdo_odbc_def_libdir=/home/db2inst1/sqllib/lib
        pdo_odbc_def_incdir=/home/db2inst1/sqllib/include
        pdo_odbc_def_lib=-ldb2
        ;;

    unixODBC)
        pdo_odbc_def_libdir=/usr/local/lib
        pdo_odbc_def_incdir=/usr/local/include
        pdo_odbc_def_lib=-lodbc
        ;;

    ODBCRouter)
        pdo_odbc_def_libdir=/usr/lib
        pdo_odbc_def_incdir=/usr/include
        pdo_odbc_def_lib=-lodbcsdk
        ;;

    generic)
        pdo_odbc_def_lib="`echo $withval | cut -d, -f3`"
        pdo_odbc_def_cflags="`echo $withval | cut -d, -f4`"
        pdo_odbc_flavour="$pdo_odbc_flavour ($pdo_odbc_def_lib)"
        ;;

      *)
        AC_MSG_ERROR(Unknown ODBC flavour $pdo_odbc_flavour
PDO_ODBC_HELP_TEXT      
)
        ;;
  esac

  if test "$pdo_odbc_dir" != "" ; then
    PDO_ODBC_INCDIR="$pdo_odbc_dir/include"
    PDO_ODBC_LIBDIR="$pdo_odbc_dir/lib"
  else
    PDO_ODBC_INCDIR="$pdo_odbc_def_incdir"
    PDO_ODBC_LIBDIR="$pdo_odbc_def_libdir"
  fi
  
  AC_MSG_RESULT([$pdo_odbc_flavour
          libs       $PDO_ODBC_LIBDIR,
          headers    $PDO_ODBC_INCDIR])

  if ! test -d "$PDO_ODBC_LIBDIR" ; then
    AC_MSG_WARN([library dir $PDO_ODBC_LIBDIR does not exist])
  fi

  dnl yick time
  PDO_ODBC_CHECK_HEADER(odbc.h)
  PDO_ODBC_CHECK_HEADER(odbcsdk.h)
  PDO_ODBC_CHECK_HEADER(iodbc.h)
  PDO_ODBC_CHECK_HEADER(sqlunix.h)
  PDO_ODBC_CHECK_HEADER(sqltypes.h)
  PDO_ODBC_CHECK_HEADER(sqlucode.h)
  PDO_ODBC_CHECK_HEADER(sql.h)
  PDO_ODBC_CHECK_HEADER(isql.h)
  PDO_ODBC_CHECK_HEADER(sqlext.h)
  PDO_ODBC_CHECK_HEADER(isqlext.h)
  PDO_ODBC_CHECK_HEADER(udbcext.h)
  PDO_ODBC_CHECK_HEADER(sqlcli1.h)
  PDO_ODBC_CHECK_HEADER(LibraryManager.h)
  PDO_ODBC_CHECK_HEADER(cli0core.h)
  PDO_ODBC_CHECK_HEADER(cli0ext.h)
  PDO_ODBC_CHECK_HEADER(cli0cli.h)
  PDO_ODBC_CHECK_HEADER(cli0defs.h)
  PDO_ODBC_CHECK_HEADER(cli0env.h)

  PDO_ODBC_INCLUDE="$pdo_odbc_def_cflags -I$PDO_ODBC_INCDIR -DPDO_ODBC_TYPE=\\\"$pdo_odbc_flavour\\\""
  PDO_ODBC_LFLAGS="-L$PDO_ODBC_LIBDIR"
  PDO_ODBC_LIBS="$pdo_odbc_def_lib"

  LDFLAGS="$PDO_ODBC_LFLAGS $PDO_ODBC_LIBS -lm -ldl"
  dnl Check for an ODBC 1.0 function to assert that the libraries work
  AC_TRY_LINK_FUNC([SQLBindCol],[],[
    AC_MSG_ERROR([[Your ODBC library does not exist]])
  ])
  dnl And now check for an ODBC 3.0 function to assert that they're
  dnl *good* libraries.
  AC_TRY_LINK_FUNC([SQLAllocHandle],[],[
    AC_MSG_ERROR([[
Your ODBC library does not appear to be ODBC 3 compatible.
You should consider using unixODBC instead, and loading your
libraries as a driver in that environment; it will emulate the
functions required for PDO support.
]])
  ])
  LDFLAGS=$save_old_LDFLAGS
  PHP_EVAL_LIBLINE($PDO_ODBC_LIBS $PDO_ODBC_LFLAGS, [PDO_ODBC_SHARED_LIBADD])
  PHP_SUBST(PDO_ODBC_SHARED_LIBADD)

  PHP_NEW_EXTENSION(pdo_odbc, pdo_odbc.c odbc_driver.c odbc_stmt.c, $ext_shared,,-I$pdo_inc_path $PDO_ODBC_INCLUDE)
  PHP_ADD_EXTENSION_DEP(pdo_odbc, pdo)
else
  AC_MSG_ERROR(
Unknown ODBC flavour $PHP_PDO_ODBC
PDO_ODBC_HELP_TEXT
)
fi

