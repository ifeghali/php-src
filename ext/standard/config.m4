dnl $Id$ -*- sh -*-

divert(3)dnl

dnl
dnl Check if flush should be called explicitly after buffered io
dnl
AC_DEFUN(AC_FLUSH_IO,[
  AC_CACHE_CHECK([whether flush should be called explicitly after a buffered io], ac_cv_flush_io,[
  AC_TRY_RUN( [
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	char *filename = tmpnam(NULL);
	char buffer[64];
	int result = 0;
	
	FILE *fp = fopen(filename, "wb");
	if (NULL == fp)
		return 0;
	fputs("line 1\n", fp);
	fputs("line 2\n", fp);
	fclose(fp);
	
	fp = fopen(filename, "rb+");
	if (NULL == fp)
		return 0;
	fgets(buffer, sizeof(buffer), fp);
	fputs("line 3\n", fp);
	rewind(fp);
	fgets(buffer, sizeof(buffer), fp);
	if (0 != strcmp(buffer, "line 1\n"))
		result = 1;
	fgets(buffer, sizeof(buffer), fp);
	if (0 != strcmp(buffer, "line 3\n"))
		result = 1;
	fclose(fp);
	unlink(filename);

	exit(result);
}
],[
  ac_cv_flush_io=no
],[
  ac_cv_flush_io=yes
],[
  ac_cv_flush_io=no
])])
  if test "$ac_cv_flush_io" = "yes"; then
    AC_DEFINE(HAVE_FLUSHIO, 1, [Define if flush should be called explicitly after a buffered io.])
  fi
])

dnl
dnl Check for crypt() capabilities
dnl
AC_DEFUN(AC_CRYPT_CAP,[

  if test "$ac_cv_func_crypt" = "no"; then
  AC_CHECK_LIB(crypt, crypt, [
    LIBS="-lcrypt $LIBS -lcrypt"
    AC_DEFINE(HAVE_CRYPT, 1, [ ])
  ])
  fi
  
  AC_CACHE_CHECK(for standard DES crypt, ac_cv_crypt_des,[
  AC_TRY_RUN([
#if HAVE_CRYPT_H
#include <crypt.h>
#endif

main() {
#if HAVE_CRYPT
    exit (strcmp((char *)crypt("rasmuslerdorf","rl"),"rl.3StKT.4T8M"));
#else
	exit(0);
#endif
}],[
    ac_cv_crypt_des=yes
  ],[
    ac_cv_crypt_des=no
  ],[
    ac_cv_crypt_des=yes
  ])
  ])
  if test "$ac_cv_crypt_des" = "yes"; then
    ac_result=1
  else
    ac_result=0
  fi
  AC_DEFINE_UNQUOTED(PHP_STD_DES_CRYPT, $ac_result, [Whether the system supports standard DES salt])

  AC_CACHE_CHECK(for extended DES crypt, ac_cv_crypt_ext_des,[
  AC_TRY_RUN([
#if HAVE_CRYPT_H
#include <crypt.h>
#endif

main() {
#if HAVE_CRYPT
    exit (strcmp((char *)crypt("rasmuslerdorf","_J9..rasm"),"_J9..rasmBYk8r9AiWNc"));
#else
	exit(0);
#endif
}],[
    ac_cv_crypt_ext_des=yes
  ],[
    ac_cv_crypt_ext_des=no
  ],[
    ac_cv_crypt_ext_des=no
  ])
  ])
  if test "$ac_cv_crypt_ext_des" = "yes"; then
    ac_result=1
  else
    ac_result=0
  fi
  AC_DEFINE_UNQUOTED(PHP_EXT_DES_CRYPT, $ac_result, [Whether the system supports extended DES salt])

  AC_CACHE_CHECK(for MD5 crypt, ac_cv_crypt_md5,[
  AC_TRY_RUN([
#if HAVE_CRYPT_H
#include <crypt.h>
#endif

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
    ac_cv_crypt_md5=yes
  ],[
    ac_cv_crypt_md5=no
  ],[
    ac_cv_crypt_md5=no
  ])
  ])
  if test "$ac_cv_crypt_md5" = "yes"; then
    ac_result=1
  else
    if test "$ac_cv_crypt_des" != "yes"; then
      PHP_DEBUG_MACRO(debug.log)
    fi
    ac_result=0
  fi
  AC_DEFINE_UNQUOTED(PHP_MD5_CRYPT, $ac_result, [Whether the system supports MD5 salt])

  AC_CACHE_CHECK(for Blowfish crypt, ac_cv_crypt_blowfish,[
  AC_TRY_RUN([
#if HAVE_CRYPT_H
#include <crypt.h>
#endif

main() {
#if HAVE_CRYPT
    char salt[30], answer[70];
    
    salt[0]='$'; salt[1]='2'; salt[2]='a'; salt[3]='$'; salt[4]='0'; salt[5]='7'; salt[6]='$'; salt[7]='\0';
    strcat(salt,"rasmuslerd............");
    strcpy(answer,salt);
    strcpy(&answer[29],"nIdrcHdxcUxWomQX9j6kvERCFjTg7Ra");
    exit (strcmp((char *)crypt("rasmuslerdorf",salt),answer));
#else
	exit(0);
#endif
}],[
    ac_cv_crypt_blowfish=yes
  ],[
    ac_cv_crypt_blowfish=no
  ],[
    ac_cv_crypt_blowfish=no
  ])
  ])
  if test "$ac_cv_crypt_blowfish" = "yes"; then
    ac_result=1
  else
    ac_result=0
  fi
  AC_DEFINE_UNQUOTED(PHP_BLOWFISH_CRYPT, $ac_result, [Whether the system supports BlowFish salt])
])

AC_CHECK_FUNCS(getcwd getwd asinh acosh atanh log1p hypot)

AC_CRYPT_CAP
AC_FLUSH_IO

divert(5)dnl

AC_ARG_WITH(regex,
[  --with-regex=TYPE       regex library type: system, apache, php. Default: php
                          WARNING: Do NOT use unless you know what you are doing!],
[
  case $withval in 
    system)
      if test "$PHP_SAPI" = "apache" || test "$PHP_SAPI" = "apache2filter" || test "$PHP_SAPI" = "apache2handler"; then
        REGEX_TYPE=php
      else
        REGEX_TYPE=system
      fi
      ;;
    apache)
      REGEX_TYPE=apache
      ;;
    php)
      REGEX_TYPE=php
      ;;
    *)
      REGEX_TYPE=php
      AC_MSG_WARN(Invalid regex library type. Using default value: php)
      ;;
  esac
],[
  REGEX_TYPE=php
])

AC_FUNC_FNMATCH	

dnl Take a look and see if there is a support means of creating a new process
dnl and defining which handles it receives
AC_DEFUN([PHP_CHECK_IF_SUPPORT_PROC_OPEN],[
  
  AC_CACHE_VAL(php_can_support_proc_open,[
    AC_CHECK_FUNCS(fork CreateProcess, [
      php_can_support_proc_open=yes
      break
    ],[
      php_can_support_proc_open=no
    ])
  ])
  
  AC_MSG_CHECKING([if your OS can spawn processes with inherited handles])
  if test "$php_can_support_proc_open" = "yes"; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(PHP_CAN_SUPPORT_PROC_OPEN,1, [Define if your system has fork/vfork/CreateProcess])
  else
    AC_MSG_RESULT(no)
  fi

])

PHP_CHECK_IF_SUPPORT_PROC_OPEN

dnl getopt long options disabled for now
dnl as we can't be sure that we get the right getopt.h here
dnl using the standard AC_CHECK macros
dnl AC_CHECK_HEADERS(getopt.h)
dnl AC_CHECK_FUNCS(getopt_long getopt_long_only)

AC_CHECK_FUNCS(glob strfmon nice)

if test "$PHP_SAPI" = "cgi" -o "$PHP_SAPI" = "cli" -o "$PHP_SAPI" = "embed"; then
  AC_DEFINE(ENABLE_CHROOT_FUNC, 1, [Whether to enable chroot() function])
fi

dnl
dnl Detect library functions needed by php dns_xxx functions
dnl ext/standard/dns.h will collect these in a single define: HAVE_DNS_FUNCS
dnl
PHP_CHECK_FUNC(res_nmkquery, resolv, bind, socket)
PHP_CHECK_FUNC(res_nsend, resolv, bind, socket)
PHP_CHECK_FUNC(dn_expand, resolv, bind, socket)
dnl already done PHP_CHECK_FUNC(dn_skipname, resolv, bind, socket)

PHP_NEW_EXTENSION(standard, array.c base64.c basic_functions.c browscap.c crc32.c crypt.c \
                            cyr_convert.c datetime.c dir.c dl.c dns.c exec.c file.c filestat.c \
                            flock_compat.c formatted_print.c fsock.c head.c html.c image.c \
                            info.c iptc.c lcg.c link.c mail.c math.c md5.c metaphone.c \
                            microtime.c pack.c pageinfo.c parsedate.c quot_print.c rand.c \
                            reg.c soundex.c string.c scanf.c syslog.c type.c uniqid.c url.c \
                            url_scanner.c var.c versioning.c assert.c strnatcmp.c levenshtein.c \
                            incomplete_class.c url_scanner_ex.c ftp_fopen_wrapper.c \
                            http_fopen_wrapper.c php_fopen_wrapper.c credits.c css.c \
                            var_unserializer.c ftok.c aggregation.c sha1.c user_filters.c \
                            filters.c proc_open.c sunfuncs.c streamsfuncs.c)

PHP_ADD_MAKEFILE_FRAGMENT
