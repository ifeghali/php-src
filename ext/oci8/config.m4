dnl
dnl $Id$
dnl

AC_DEFUN(PHP_OCI_IF_DEFINED,[
  old_CPPFLAGS=$CPPFLAGS
  CPPFLAGS=$3
  AC_EGREP_CPP(yes,[
#include <oci.h>
#if defined($1)
    yes
#endif
  ],[
    CPPFLAGS=$old_CPPFLAGS
    $2
  ],[
    CPPFLAGS=$old_CPPFLAGS
  ])
])

AC_DEFUN(AC_OCI8_CHECK_LIB_DIR,[
  PHP_CHECK_64BIT([ TMP_OCI8_LIB_DIR=lib32 ], [ TMP_OCI8_LIB_DIR=lib ])
  AC_MSG_CHECKING([OCI8 libraries dir])
  if test -d "$OCI8_DIR/lib" -a ! -d "$OCI8_DIR/lib32"; then
	OCI8_LIB_DIR=lib
  elif ! test -d "$OCI8_DIR/lib" -a -d "$OCI8_DIR/lib32"; then
	OCI8_LIB_DIR=lib32
  elif test -d "$OCI8_DIR/lib" -a -d "$OCI8_DIR/lib32"; then
    OCI8_LIB_DIR=$TMP_OCI8_LIB_DIR
  else
    AC_MSG_ERROR(Oracle-OCI8 libraries directory not found)
  fi
  AC_MSG_RESULT($OCI8_LIB_DIR)
])

AC_DEFUN(AC_OCI8_VERSION,[
  AC_MSG_CHECKING([Oracle version])
  if test -s "$OCI8_DIR/orainst/unix.rgs"; then
    OCI8_VERSION=`grep '"ocommon"' $OCI8_DIR/orainst/unix.rgs | sed 's/[ ][ ]*/:/g' | cut -d: -f 6 | cut -c 2-4`
    test -z "$OCI8_VERSION" && OCI8_VERSION=7.3
  elif test -f $OCI8_DIR/$OCI8_LIB_DIR/libclntsh.$SHLIB_SUFFIX_NAME.10.1; then
    OCI8_VERSION=10.1    
  elif test -f $OCI8_DIR/$OCI8_LIB_DIR/libclntsh.$SHLIB_SUFFIX_NAME.9.0; then
    OCI8_VERSION=9.0
  elif test -f $OCI8_DIR/$OCI8_LIB_DIR/libclntsh.$SHLIB_SUFFIX_NAME.8.0; then
    OCI8_VERSION=8.1
  elif test -f $OCI8_DIR/$OCI8_LIB_DIR/libclntsh.$SHLIB_SUFFIX_NAME.1.0; then
    OCI8_VERSION=8.0
  elif test -f $OCI8_DIR/$OCI8_LIB_DIR/libclntsh.a; then 
    if test -f $OCI8_DIR/$OCI8_LIB_DIR/libcore4.a; then 
      OCI8_VERSION=8.0
    else
      OCI8_VERSION=8.1
    fi
  else
    AC_MSG_ERROR(Oracle-OCI8 needed libraries not found)
  fi
  AC_MSG_RESULT($OCI8_VERSION)
])

PHP_ARG_WITH(oci8, for Oracle-OCI8 support,
[  --with-oci8[=DIR]       Include Oracle-oci8 support. Default DIR is ORACLE_HOME.])

if test "$PHP_OCI8" != "no"; then
  AC_MSG_CHECKING([Oracle Install-Dir])
  if test "$PHP_OCI8" = "yes"; then
    OCI8_DIR=$ORACLE_HOME
  else
    OCI8_DIR=$PHP_OCI8
  fi
  AC_MSG_RESULT($OCI8_DIR)

  AC_OCI8_CHECK_LIB_DIR($OCI8_DIR)

  if test -d "$OCI8_DIR/rdbms/public"; then
    PHP_ADD_INCLUDE($OCI8_DIR/rdbms/public)
    OCI8_INCLUDES="$OCI8_INCLUDES -I$OCI8_DIR/rdbms/public"
  fi
  if test -d "$OCI8_DIR/rdbms/demo"; then
    PHP_ADD_INCLUDE($OCI8_DIR/rdbms/demo)
    OCI8_INCLUDES="$OCI8_INCLUDES -I$OCI8_DIR/rdbms/demo"
  fi
  if test -d "$OCI8_DIR/network/public"; then
    PHP_ADD_INCLUDE($OCI8_DIR/network/public)
    OCI8_INCLUDES="$OCI8_INCLUDES -I$OCI8_DIR/network/public"
  fi
  if test -d "$OCI8_DIR/plsql/public"; then
    PHP_ADD_INCLUDE($OCI8_DIR/plsql/public)
    OCI8_INCLUDES="$OCI8_INCLUDES -I$OCI8_DIR/plsql/public"
  fi

  if test -f "$OCI8_DIR/$OCI8_LIB_DIR/sysliblist"; then
    PHP_EVAL_LIBLINE(`cat $OCI8_DIR/$OCI8_LIB_DIR/sysliblist`, OCI8_SYSLIB)
  elif test -f "$OCI8_DIR/rdbms/$OCI8_LIB_DIR/sysliblist"; then
    PHP_EVAL_LIBLINE(`cat $OCI8_DIR/rdbms/$OCI8_LIB_DIR/sysliblist`, OCI8_SYSLIB)
  fi

  AC_OCI8_VERSION($OCI8_DIR)

  case $OCI8_VERSION in
    8.0)
      PHP_ADD_LIBRARY_WITH_PATH(nlsrtl3, "", OCI8_SHARED_LIBADD)
      PHP_ADD_LIBRARY_WITH_PATH(core4, "", OCI8_SHARED_LIBADD)
      PHP_ADD_LIBRARY_WITH_PATH(psa, "", OCI8_SHARED_LIBADD)
      PHP_ADD_LIBRARY_WITH_PATH(clntsh, $OCI8_DIR/$OCI8_LIB_DIR, OCI8_SHARED_LIBADD)
      ;;

    8.1)
      PHP_ADD_LIBRARY(clntsh, 1, OCI8_SHARED_LIBADD)
      PHP_ADD_LIBPATH($OCI8_DIR/$OCI8_LIB_DIR, OCI8_SHARED_LIBADD)

      dnl 
      dnl OCI_ATTR_STATEMENT is not available in all 8.1.x versions
      dnl 
      PHP_OCI_IF_DEFINED(OCI_ATTR_STATEMENT, [AC_DEFINE(HAVE_OCI8_ATTR_STATEMENT,1,[ ])], $OCI8_INCLUDES)
      ;;

    9.0)
      PHP_ADD_LIBRARY(clntsh, 1, OCI8_SHARED_LIBADD)
      PHP_ADD_LIBPATH($OCI8_DIR/$OCI8_LIB_DIR, OCI8_SHARED_LIBADD)
      AC_DEFINE(HAVE_OCI8_ATTR_STATEMENT,1,[ ])

      dnl These functions are only available in version >= 9.2
      PHP_CHECK_LIBRARY(clntsh, OCIEnvNlsCreate,
      [
        PHP_CHECK_LIBRARY(clntsh, OCINlsCharSetNameToId,
        [
          AC_DEFINE(HAVE_OCI_9_2,1,[ ])
          OCI8_VERSION=9.2
        ], [], [
          -L$OCI8_DIR/$OCI8_LIB_DIR $OCI8_SHARED_LIBADD
        ])
      ], [], [
        -L$OCI8_DIR/$OCI8_LIB_DIR $OCI8_SHARED_LIBADD
      ])
      ;;
      
    10.1)
      PHP_ADD_LIBRARY(clntsh, 1, OCI8_SHARED_LIBADD)
      PHP_ADD_LIBPATH($OCI8_DIR/$OCI8_LIB_DIR, OCI8_SHARED_LIBADD)
      AC_DEFINE(HAVE_OCI8_ATTR_STATEMENT,1,[ ])
      AC_DEFINE(HAVE_OCI_9_2,1,[ ])
      ;;
    *)
      AC_MSG_ERROR(Unsupported Oracle version!)
      ;;
  esac

  dnl
  dnl Check if we need to add -locijdbc8 
  dnl
  PHP_CHECK_LIBRARY(clntsh, OCILobIsTemporary,
  [
    AC_DEFINE(HAVE_OCI8_TEMP_LOB,1,[ ])
  ], [
    PHP_CHECK_LIBRARY(ocijdbc8, OCILobIsTemporary,
    [
      PHP_ADD_LIBRARY(ocijdbc8, 1, OCI8_SHARED_LIBADD)
      AC_DEFINE(HAVE_OCI8_TEMP_LOB,1,[ ])
    ], [], [
      -L$OCI8_DIR/$OCI8_LIB_DIR $OCI8_SHARED_LIBADD
    ])
  ], [
    -L$OCI8_DIR/$OCI8_LIB_DIR $OCI8_SHARED_LIBADD
  ])

  dnl
  dnl Check if we have collections
  dnl
  PHP_CHECK_LIBRARY(clntsh, OCICollAssign,
  [
    AC_DEFINE(PHP_OCI8_HAVE_COLLECTIONS,1,[ ])
  ], [], [
    -L$OCI8_DIR/$OCI8_LIB_DIR $OCI8_SHARED_LIBADD
  ])


  PHP_NEW_EXTENSION(oci8, oci8.c, $ext_shared)
  AC_DEFINE(HAVE_OCI8,1,[ ])

  PHP_SUBST_OLD(OCI8_SHARED_LIBADD)
  PHP_SUBST_OLD(OCI8_DIR)
  PHP_SUBST_OLD(OCI8_VERSION)
  
fi
