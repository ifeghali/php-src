dnl $Id: acinclude.m4,v 1.4 1999/04/21 22:49:13 ssb Exp $
dnl
dnl This file contains local autoconf functions.

dnl
dnl See if we have broken header files like SunOS has.
dnl
AC_DEFUN(AC_MISSING_FCLOSE_DECL,[
  AC_MSG_CHECKING([for fclose declaration])
  AC_TRY_COMPILE([#include <stdio.h>],[int (*func)() = fclose],[
    AC_DEFINE(MISSING_FCLOSE_DECL,0)
    AC_MSG_RESULT(ok)
  ],[
    AC_DEFINE(MISSING_FCLOSE_DECL,1)
    AC_MSG_RESULT(missing)
  ])
])

dnl
dnl Check for broken sprintf()
dnl
AC_DEFUN(AC_BROKEN_SPRINTF,[
  AC_MSG_CHECKING([for broken sprintf])
  AC_TRY_RUN([main() { char buf[20]; exit (sprintf(buf,"testing 123")!=11); }],[
    AC_DEFINE(BROKEN_SPRINTF,0)
    AC_MSG_RESULT(ok)
  ],[
    AC_DEFINE(BROKEN_SPRINTF,1)
    AC_MSG_RESULT(broken)
  ],[
    AC_DEFINE(BROKEN_SPRINTF,0)
    AC_MSG_RESULT(cannot check, guessing ok)
  ])
])

dnl
dnl Check for crypt() capabilities
dnl
AC_DEFUN(AC_CRYPT_CAP,[

  AC_MSG_CHECKING([for standard DES crypt])
  AC_TRY_RUN([
main() {
#if HAVE_CRYPT
    exit (strcmp((char *)crypt("rasmuslerdorf","rl"),"rl.3StKT.4T8M"));
#else
	exit(0);
#endif
}],[
    AC_DEFINE(PHP3_STD_DES_CRYPT,1)
    AC_MSG_RESULT(yes)
  ],[
    AC_DEFINE(PHP3_STD_DES_CRYPT,0)
    AC_MSG_RESULT(no)
  ],[
    AC_DEFINE(PHP3_STD_DES_CRYPT,1)
    AC_MSG_RESULT(cannot check, guessing yes)
  ])

  AC_MSG_CHECKING([for extended DES crypt])
  AC_TRY_RUN([
main() {
#if HAVE_CRYPT
    exit (strcmp((char *)crypt("rasmuslerdorf","_J9..rasm"),"_J9..rasmBYk8r9AiWNc"));
#else
	exit(0);
#endif
}],[
    AC_DEFINE(PHP3_EXT_DES_CRYPT,1)
    AC_MSG_RESULT(yes)
  ],[
    AC_DEFINE(PHP3_EXT_DES_CRYPT,0)
    AC_MSG_RESULT(no)
  ],[
    AC_DEFINE(PHP3_EXT_DES_CRYPT,0)
    AC_MSG_RESULT(cannot check, guessing no)
  ])

  AC_MSG_CHECKING([for MD5 crypt])
  AC_TRY_RUN([
main() {
#if HAVE_CRYPT
    char salt[15], answer[40];

    salt[0]='$'; salt[1]='1'; salt[2]='$'; 
    salt[3]='r'; salt[4]='a'; salt[5]='s';
    salt[6]='m'; salt[7]='u'; salt[8]='s';
    salt[9]='l'; salt[10]='e'; salt[11]='$';
    salt[12]='\0';
    strcpy(answer,salt);
    strcat(answer,"rISCgZzpwk3UhDidwXvin0");
    exit (strcmp((char *)crypt("rasmuslerdorf",salt),answer));
#else
	exit(0);
#endif
}],[
    AC_DEFINE(PHP3_MD5_CRYPT,1)
    AC_MSG_RESULT(yes)
  ],[
    AC_DEFINE(PHP3_MD5_CRYPT,0)
    AC_MSG_RESULT(no)
  ],[
    AC_DEFINE(PHP3_MD5_CRYPT,0)
    AC_MSG_RESULT(cannot check, guessing no)
  ])

  AC_MSG_CHECKING([for Blowfish crypt])
  AC_TRY_RUN([
main() {
#if HAVE_CRYPT
    char salt[25], answer[70];
    
    salt[0]='$'; salt[1]='2'; salt[2]='a'; salt[3]='$'; salt[4]='0'; salt[5]='7'; salt[6]='$'; salt[7]='\0';
    strcat(salt,"rasmuslerd");
    strcpy(answer,salt);
    strcpy(&answer[16],"O............gl95GkTKn53Of.H4YchXl5PwvvW.5ri");
    exit (strcmp((char *)crypt("rasmuslerdorf",salt),answer));
#else
	exit(0);
#endif
}],[
    AC_DEFINE(PHP3_BLOWFISH_CRYPT,1)
    AC_MSG_RESULT(yes)
  ],[
    AC_DEFINE(PHP3_BLOWFISH_CRYPT,0)
    AC_MSG_RESULT(no)
  ],[
    AC_DEFINE(PHP3_BLOWFISH_CRYPT,0)
    AC_MSG_RESULT(cannot check, guessing no)
  ])
])

dnl
dnl Stuff to do when setting up a new extension.
dnl XXX have to change the hardcoding of ".a" when we want to be able
dnl to make dynamic libraries as well.
dnl
AC_DEFUN(PHP_EXTENSION,[
  EXT_SUBDIRS="$EXT_SUBDIRS $1"
  _extlib="libphpext_$1.a"
  EXT_LIBS="$EXT_LIBS $1/$_extlib"
  EXTINFO_DEPS="$EXTINFO_DEPS ../ext/$1/extinfo.c.stub"
])

AC_SUBST(EXT_SUBDIRS)
AC_SUBST(EXT_LIBS)
AC_SUBST(EXTINFO_DEPS)
