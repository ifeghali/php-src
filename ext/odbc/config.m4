dnl
dnl Figure out which library file to link with for the Solid support.
dnl
AC_DEFUN(AC_FIND_SOLID_LIBS,[
  AC_MSG_CHECKING([Solid library file])
  ac_solid_uname_s=`uname -s 2>/dev/null`
  case $ac_solid_uname_s in
    AIX) ac_solid_os=a3x;;
    HP-UX) ac_solid_os=h9x;;
    IRIX) ac_solid_os=irx;;
    Linux) ac_solid_os=lux;;
    SunOS) ac_solid_os=ssx;; # should we deal with SunOS 4?
    FreeBSD) ac_solid_os=fbx;;
    # "uname -s" on SCO makes no sense.
  esac
  ODBC_LIBS=`echo $1/scl${ac_solid_os}*.so | cut -d' ' -f1`
  if test ! -f $ODBC_LIBS; then
    ODBC_LIBS=`echo $1/scl${ac_solid_os}*.a | cut -d' ' -f1`
  fi
  if test ! -f $ODBC_LIBS; then
    ODBC_LIBS=`echo $1/scl2x${ac_solid_os}*.a | cut -d' ' -f1`
  fi
  if test ! -f $ODBC_LIBS; then
    ODBC_LIBS=`echo $1/scl2x${ac_solid_os}*.a | cut -d' ' -f1`
  fi
  if test ! -f $ODBC_LIBS; then
    ODBC_LIBS=`echo $1/bcl${ac_solid_os}*.so | cut -d' ' -f1`
  fi
  if test ! -f $ODBC_LIBS; then
    ODBC_LIBS=`echo $1/bcl${ac_solid_os}*.a | cut -d' ' -f1`
  fi
  AC_MSG_RESULT(`echo $ODBC_LIBS | sed -e 's!.*/!!'`)
])

dnl
dnl Figure out which library file to link with for the Empress support.
dnl
AC_DEFUN(AC_FIND_EMPRESS_LIBS,[
  AC_MSG_CHECKING([Empress library file])
  ODBC_LIBS=`echo $1/empodbc.so | cut -d' ' -f1`
  if test ! -f $ODBC_LIBS; then
    ODBC_LIBS=`echo $1/empodbc.a | cut -d' ' -f1`
  fi
  AC_MSG_RESULT(`echo $ODBC_LIBS | sed -e 's!.*/!!'`)
])

dnl
dnl Figure out the path where the newest DBMaker is installed.
dnl
AC_DEFUN(AC_FIND_DBMAKER_PATH,[
  AC_MSG_CHECKING([DBMaker version])
  if [ test -d "$1/4.0" ]; then
    DBMAKER_PATH=$1/4.0
  elif [ test -d "$1/3.6" ]; then
    DBMAKER_PATH=$1/3.6
  elif [ test -d "$1/3.5" ]; then
    DBMAKER_PATH=$1/3.5
  elif [ test -d "$1/3.01" ]; then
    DBMAKER_PATH=$1/3.01
  elif [ test -d "$1/3.0" ]; then
    DBMAKER_PATH=$1/3.0
  else
    DBMAKER_PATH=$1
  fi
  AC_MSG_RESULT(`echo $DBMAKER_PATH | sed -e 's!.*/!!'`)
])

if test -z "$ODBC_TYPE"; then
AC_MSG_CHECKING(for Adabas support)
AC_ARG_WITH(adabas,
[  --with-adabas[=DIR]     Include Adabas D support.  DIR is the Adabas base
                          install directory, defaults to /usr/local.],
[
  if test "$withval" = "yes"; then
    withval=/usr/local
  fi
  if test "$withval" != "no"; then
    AC_ADD_INCLUDE($withval/incl)
    AC_ADD_LIBPATH($withval/lib)
    ODBC_OBJS="$withval/lib/odbclib.a"
    ODBC_LIB="$abs_builddir/ext/odbc/libodbc_adabas.a"
    $srcdir/build/shtool mkdir -f -p ext/odbc
    rm -f "$ODBC_LIB"
    cp "$ODBC_OBJS" "$ODBC_LIB"
	AC_ADD_LIBRARY(sqlptc)
	AC_ADD_LIBRARY(sqlrte)
    AC_ADD_LIBRARY_WITH_PATH(odbc_adabas, $abs_builddir/ext/odbc)
    ODBC_TYPE=adabas
    AC_DEFINE(HAVE_ADABAS,1,[ ])
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
fi

if test -z "$ODBC_TYPE"; then
AC_MSG_CHECKING(for Solid support)
AC_ARG_WITH(solid,
[  --with-solid[=DIR]      Include Solid support.  DIR is the Solid base
                          install directory, defaults to /usr/local/solid],
[
  if test "$withval" = "yes"; then
    withval=/usr/local/solid
  fi
  if test "$withval" != "no"; then
    ODBC_INCDIR=$withval/include
    ODBC_LIBDIR=$withval/lib
    ODBC_INCLUDE=-I$ODBC_INCDIR
    ODBC_TYPE=solid
    AC_DEFINE(HAVE_SOLID,1,[ ])
    AC_MSG_RESULT(yes)
    AC_FIND_SOLID_LIBS($ODBC_LIBDIR)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
fi

if test -z "$ODBC_TYPE"; then
AC_MSG_CHECKING(for IBM DB2 support)
AC_ARG_WITH(ibm-db2,
[  --with-ibm-db2[=DIR]    Include IBM DB2 support.  DIR is the DB2 base
                          install directory, defaults to /home/db2inst1/sqllib],
[
  if test "$withval" != "no"; then
    if test "$withval" = "yes"; then
        ODBC_INCDIR=/home/db2inst1/sqllib/include
        ODBC_LIBDIR=/home/db2inst1/sqllib/lib
    else
        ODBC_INCDIR=$withval/include
        ODBC_LIBDIR=$withval/lib
    fi
    ODBC_INCLUDE=-I$ODBC_INCDIR
    ODBC_TYPE=db2
    ODBC_LIBS="-ldb2"
    AC_DEFINE(HAVE_IBMDB2,1,[ ])

    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
fi

if test -z "$ODBC_TYPE"; then
AC_MSG_CHECKING(for Empress support)
AC_ARG_WITH(empress,
[  --with-empress[=DIR]    Include Empress support.  DIR is the Empress base
                          install directory, defaults to \$EMPRESSPATH],
[
  if test "$withval" != "no"; then
    if test "$withval" = "yes"; then
      ODBC_INCDIR=$EMPRESSPATH/odbccl/include
      ODBC_LIBDIR=$EMPRESSPATH/odbccl/lib
    else
      ODBC_INCDIR=$withval/include
      ODBC_LIBDIR=$withval/lib
    fi
    ODBC_INCLUDE=-I$ODBC_INCDIR
    ODBC_TYPE=empress
    AC_DEFINE(HAVE_EMPRESS,1,[ ])
    AC_MSG_RESULT(yes)
    AC_FIND_EMPRESS_LIBS($ODBC_LIBDIR)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
fi

if test -z "$ODBC_TYPE"; then
AC_MSG_CHECKING(for Velocis support)
AC_ARG_WITH(velocis,
[  --with-velocis[=DIR]    Include Velocis support.  DIR is the Velocis base
                          install directory, defaults to /usr/local/velocis.],
[
  if test "$withval" != "no"; then
    if test "$withval" = "yes"; then
      ODBC_INCDIR=/usr/local/velocis/include
      ODBC_LIBDIR=/usr/local/velocis
    else
      ODBC_INCDIR=$withval/include
      ODBC_LIBDIR=$withval
    fi
    ODBC_INCLUDE=-I$ODBC_INCDIR
    ODBC_LIBDIR="$ODBC_LIBDIR/bin"
    case `uname` in
      FreeBSD|BSD/OS)
        ODBC_LIBS="$ODBC_LIBDIR/../lib/rdscli.a -lcompat";;
      *)
        ODBC_LIBS="-l_rdbc -l_sql";;
    esac
    ODBC_TYPE=velocis
    AC_DEFINE(HAVE_VELOCIS,1,[ ])
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
fi

if test -z "$ODBC_TYPE"; then
AC_MSG_CHECKING(for a custom ODBC support)
AC_ARG_WITH(custom-odbc,
[  --with-custom-odbc[=DIR]      
                          Include a user defined ODBC support.
                          The DIR is ODBC install base directory, 
                          which defaults to /usr/local.
                          Make sure to define CUSTOM_ODBC_LIBS and
                          have some odbc.h in your include dirs.
                          E.g., you should define following for 
                          Sybase SQL Anywhere 5.5.00 on QNX, prior to
                          run configure script:
                              CFLAGS=\"-DODBC_QNX -DSQLANY_BUG\"
                              LDFLAGS=-lunix
                              CUSTOM_ODBC_LIBS=\"-ldblib -lodbc\".],
[
  if test "$withval" = "yes"; then
    withval=/usr/local
  fi
  if test "$withval" != "no"; then
    ODBC_INCDIR=$withval/include
    ODBC_LIBDIR=$withval/lib
    ODBC_LFLAGS=-L$ODBC_LIBDIR
    ODBC_INCLUDE=-I$ODBC_INCDIR
    ODBC_LIBS=$CUSTOM_ODBC_LIBS
    ODBC_TYPE=custom
    AC_DEFINE(HAVE_CODBC,1,[ ])
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
fi

if test -z "$ODBC_TYPE"; then
AC_MSG_CHECKING(for iODBC support)
AC_ARG_WITH(iodbc,
[  --with-iodbc[=DIR]      Include iODBC support.  DIR is the iODBC base
                          install directory, defaults to /usr/local.],
[
  if test "$withval" = "yes"; then
    withval=/usr/local
  fi
  if test "$withval" != "no"; then
    AC_ADD_LIBRARY_WITH_PATH(iodbc, $withval/lib)
    AC_ADD_INCLUDE($withval/include)
    ODBC_TYPE=iodbc
    AC_DEFINE(HAVE_IODBC,1,[ ])
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
fi

if test -z "$ODBC_TYPE"; then
AC_MSG_CHECKING(for Easysoft ODBC-ODBC Bridge support)
AC_ARG_WITH(esoob,
[  --with-esoob[=DIR]      Include Easysoft OOB support. DIR is the OOB base
                          install directory,
                          defaults to /usr/local/easysoft/oob/client.],
[
  if test "$withval" = "yes"; then
    withval=/usr/local/easysoft/oob/client
  fi
  if test "$withval" != "no"; then
    ODBC_INCDIR=$withval/include
    ODBC_LIBDIR=$withval/lib
    ODBC_LFLAGS=-L$ODBC_LIBDIR
    ODBC_INCLUDE=-I$ODBC_INCDIR
    ODBC_LIBS=-lesoobclient
    ODBC_TYPE=ESOOB
    AC_DEFINE(HAVE_ESOOB,1,[ ])
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
fi

if test -z "$ODBC_TYPE"; then
AC_MSG_CHECKING(for unixODBC support)
AC_ARG_WITH(unixODBC,
[  --with-unixODBC[=DIR]   Include unixODBC support.  DIR is the unixODBC base
                          install directory, defaults to /usr/local.],
[
  if test "$withval" = "yes"; then
    withval=/usr/local
  fi
  if test "$withval" != "no"; then
    ODBC_INCDIR=$withval/include
    ODBC_LIBDIR=$withval/lib
    ODBC_LFLAGS=-L$ODBC_LIBDIR
    ODBC_INCLUDE=-I$ODBC_INCDIR
    ODBC_LIBS=-lodbc
    ODBC_TYPE=unixODBC
    AC_DEFINE(HAVE_UNIXODBC,1,[ ])
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
fi

if test -z "$ODBC_TYPE"; then
AC_MSG_CHECKING(for OpenLink ODBC support)
AC_ARG_WITH(openlink,
[  --with-openlink[=DIR]   Include OpenLink ODBC support.  DIR is the
                          OpenLink base install directory, defaults to
                          /usr/local/openlink.],
[
  if test "$withval" = "yes"; then
    withval=/usr/local/openlink
  fi
  if test "$withval" != "no"; then
    ODBC_INCDIR=$withval/odbcsdk/include
    ODBC_LIBDIR=$withval/odbcsdk/lib
    ODBC_LFLAGS=-L$ODBC_LIBDIR
    ODBC_INCLUDE=-I$ODBC_INCDIR
    ODBC_LIBS=-liodbc
    ODBC_TYPE=openlink
    AC_DEFINE(HAVE_OPENLINK,1,[ ])
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
fi

if test -z "$ODBC_TYPE"; then
AC_MSG_CHECKING(for DBMaker support)
AC_ARG_WITH(dbmaker,
[  --with-dbmaker[=DIR]    Include DBMaker support.  DIR is the DBMaker base
                          install directory, defaults to where the latest 
                          version of DBMaker is installed (such as
                          /home/dbmaker/3.6).],
[
  if test "$withval" = "yes"; then
    # find dbmaker's home directory
    DBMAKER_HOME=`grep "^dbmaker:" /etc/passwd | awk -F: '{print $6}'`
    AC_FIND_DBMAKER_PATH($DBMAKER_HOME)
    withval=$DBMAKER_PATH
  fi
  if test "$withval" != "no"; then
    ODBC_INCDIR=$withval/include
    ODBC_LIBDIR=$withval/lib
    ODBC_INCLUDE=-I$ODBC_INCDIR
    ODBC_LFLAGS=-L$ODBC_LIBDIR
    ODBC_INCLUDE=-I$ODBC_INCDIR
    ODBC_LIBS="-ldmapic -lc"
    ODBC_TYPE=dbmaker
    AC_DEFINE(HAVE_DBMAKER,1,[ ])
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])
fi

if test -n "$ODBC_TYPE"; then
  INCLUDES="$INCLUDES $ODBC_INCLUDE"
  EXTRA_LIBS="$EXTRA_LIBS $ODBC_LFLAGS $ODBC_LIBS"
  AC_DEFINE(HAVE_UODBC,1,[ ])
  PHP_SUBST(ODBC_INCDIR)
  PHP_SUBST(ODBC_INCLUDE)
  PHP_SUBST(ODBC_LIBDIR)
  PHP_SUBST(ODBC_LIBS)
  PHP_SUBST(ODBC_LFLAGS)
  PHP_SUBST(ODBC_TYPE)
  PHP_EXTENSION(odbc)
fi
