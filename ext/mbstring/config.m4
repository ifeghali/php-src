dnl
dnl $Id$
dnl

AC_DEFUN([PHP_MBSTRING_INIT], [
  PHP_MBSTRING_SOURCES=""
  PHP_MBSTRING_EXTRA_INCLUDES=""
  PHP_MBSTRING_EXTRA_BUILD_DIRS=""
  PHP_MBSTRING_EXTRA_CONFIG_HEADERS=""
  PHP_MBSTRING=""
  PHP_MBREGEX=""
  PHP_MBSTRING_CFLAGS=""
])

AC_DEFUN([PHP_MBSTRING_ADD_SOURCES], [
  PHP_MBSTRING_SOURCES="$PHP_MBSTRING_SOURCES $1"
])

AC_DEFUN([PHP_MBSTRING_ADD_BUILD_DIR], [
  PHP_MBSTRING_EXTRA_BUILD_DIRS="$PHP_MBSTRING_EXTRA_BUILD_DIRS $1"
])

AC_DEFUN([PHP_MBSTRING_ADD_INCLUDE], [
  PHP_MBSTRING_EXTRA_INCLUDES="$PHP_MBSTRING_EXTRA_INCLUDES $1"
])

AC_DEFUN([PHP_MBSTRING_CONFIG_HEADER], [
  PHP_MBSTRING_EXTRA_CONFIG_HEADERS="$PHP_MBSTRING_EXTRA_CONFIG_HEADERS $1"
])

AC_DEFUN([PHP_MBSTRING_ADD_CFLAG], [
  PHP_MBSTRING_CFLAGS="$PHP_MBSTRING_CFLAGS $1"
])

AC_DEFUN([PHP_MBSTRING_EXTENSION], [
  PHP_NEW_EXTENSION(mbstring, $PHP_MBSTRING_SOURCES, $ext_shared,, \\$(PHP_MBSTRING_CFLAGS))
  for dir in $PHP_MBSTRING_EXTRA_INCLUDES; do
    PHP_ADD_INCLUDE([$ext_builddir/$dir])
  done
  for dir in $PHP_MBSTRING_EXTRA_BUILD_DIRS; do
    PHP_ADD_BUILD_DIR([$ext_builddir/$dir])
  done
  for cfg in $PHP_MBSTRING_EXTRA_CONFIG_HEADERS; do
    AC_CONFIG_HEADER([$ext_builddir/$cfg])
  done
  PHP_SUBST(PHP_MBSTRING_CFLAGS)
])


AC_DEFUN([PHP_MBSTRING_SETUP], [
  PHP_ARG_ENABLE(mbstring, whether to enable multibyte string support,
  [  --enable-mbstring       Enable multibyte string support])
  if test "$PHP_MBSTRING" != "no"; then
    PHP_ARG_WITH(libmbfl-dir, [for libmbfl installation prefix],
      [  --with-libmbfl-dir[=DIR]   Include libmbfl support where DIR is libmbfl install prefix.
                           If DIR is not set, the bundled libmbfl will be used.],
      [yes], [no]
    )
    AC_DEFINE([HAVE_MBSTRING],1,[whether to have multibyte string support])

    if test -z "$PHP_MBSTRING" -o "$PHP_MBSTRING" = "all" -o "$PHP_MBSTRING" = "ja"; then
      AC_DEFINE([HAVE_MBSTR_JA],1,[whether to have japanese support])
    fi
    if test "$PHP_MBSTRING" = "all" -o "$PHP_MBSTRING" = "cn"; then
      AC_DEFINE([HAVE_MBSTR_CN],1,[whether to have simplified chinese support])
    fi
    if test "$PHP_MBSTRING" = "all" -o  "$PHP_MBSTRING" = "tw"; then
      AC_DEFINE([HAVE_MBSTR_TW],1,[whether to have traditional chinese support])
    fi
    if test "$PHP_MBSTRING" = "all" -o  "$PHP_MBSTIRNG" = "kr"; then
      AC_DEFINE([HAVE_MBSTR_KR],1,[whether to have korean support])
    fi
    if test "$PHP_MBSTRING" = "all" -o "$PHP_MBSTRING" = "ru"; then
      AC_DEFINE([HAVE_MBSTR_RU],1,[whether to have russian support])
    fi

    if test "$PHP_LIBMBFL_DIR" = "yes"; then
      AC_DEFINE([HAVE_LIBMBFL], 1, [whether to have libmbfl support])
      PHP_MBSTRING_ADD_BUILD_DIR([libmbfl])
      PHP_MBSTRING_ADD_INCLUDE([libmbfl])
      PHP_MBSTRING_ADD_INCLUDE([libmbfl/mbfl])
      PHP_MBSTRING_ADD_SOURCES([
        libmbfl/filters/html_entities.c
        libmbfl/filters/mbfilter_7bit.c
        libmbfl/filters/mbfilter_ascii.c
        libmbfl/filters/mbfilter_base64.c
        libmbfl/filters/mbfilter_big5.c
        libmbfl/filters/mbfilter_byte2.c
        libmbfl/filters/mbfilter_byte4.c
        libmbfl/filters/mbfilter_cp1251.c
        libmbfl/filters/mbfilter_cp1252.c
        libmbfl/filters/mbfilter_cp866.c
        libmbfl/filters/mbfilter_cp932.c
        libmbfl/filters/mbfilter_cp936.c
        libmbfl/filters/mbfilter_euc_cn.c
        libmbfl/filters/mbfilter_euc_jp.c
        libmbfl/filters/mbfilter_euc_jp_win.c
        libmbfl/filters/mbfilter_euc_kr.c
        libmbfl/filters/mbfilter_euc_tw.c
        libmbfl/filters/mbfilter_htmlent.c
        libmbfl/filters/mbfilter_hz.c
        libmbfl/filters/mbfilter_iso2022_kr.c
        libmbfl/filters/mbfilter_iso8859_1.c
        libmbfl/filters/mbfilter_iso8859_10.c
        libmbfl/filters/mbfilter_iso8859_13.c
        libmbfl/filters/mbfilter_iso8859_14.c
        libmbfl/filters/mbfilter_iso8859_15.c
        libmbfl/filters/mbfilter_iso8859_2.c
        libmbfl/filters/mbfilter_iso8859_3.c
        libmbfl/filters/mbfilter_iso8859_4.c
        libmbfl/filters/mbfilter_iso8859_5.c
        libmbfl/filters/mbfilter_iso8859_6.c
        libmbfl/filters/mbfilter_iso8859_7.c
        libmbfl/filters/mbfilter_iso8859_8.c
        libmbfl/filters/mbfilter_iso8859_9.c
        libmbfl/filters/mbfilter_jis.c
        libmbfl/filters/mbfilter_koi8r.c
        libmbfl/filters/mbfilter_qprint.c
        libmbfl/filters/mbfilter_sjis.c
        libmbfl/filters/mbfilter_ucs2.c
        libmbfl/filters/mbfilter_ucs4.c
        libmbfl/filters/mbfilter_uhc.c
        libmbfl/filters/mbfilter_utf16.c
        libmbfl/filters/mbfilter_utf32.c
        libmbfl/filters/mbfilter_utf7.c
        libmbfl/filters/mbfilter_utf7imap.c
        libmbfl/filters/mbfilter_utf8.c
        libmbfl/filters/mbfilter_uuencode.c
        libmbfl/mbfl/mbfilter.c
        libmbfl/mbfl/mbfilter_8bit.c
        libmbfl/mbfl/mbfilter_pass.c
        libmbfl/mbfl/mbfilter_wchar.c
        libmbfl/mbfl/mbfl_convert.c
        libmbfl/mbfl/mbfl_encoding.c
        libmbfl/mbfl/mbfl_filter_output.c
        libmbfl/mbfl/mbfl_ident.c
        libmbfl/mbfl/mbfl_language.c
        libmbfl/mbfl/mbfl_memory_device.c
        libmbfl/mbfl/mbfl_string.c
        libmbfl/mbfl/mbfl_allocators.c
        libmbfl/nls/nls_de.c
        libmbfl/nls/nls_en.c
        libmbfl/nls/nls_ja.c
        libmbfl/nls/nls_kr.c
        libmbfl/nls/nls_neutral.c
        libmbfl/nls/nls_ru.c
        libmbfl/nls/nls_uni.c
        libmbfl/nls/nls_zh.c
      ])
    else
      PHP_LIBMBFL_INCLUDE=
      PHP_LIBMBFL_LIB=

      if test "$PHP_LIBMBFL_DIR" != "no"; then
        for prefix in "$PHP_LIBMBFL_DIR" "/usr" "/usr/local"; do
          for inc in "$prefix/include/mbfl" "$prefix/include/mbfl-1.0" "$prefix/include"; do
            if test -f "$inc/mbfilter.h"; then
              PHP_LIBMBFL_INCLUDE="$inc"
            fi
          done
        done
        for prefix in "$PHP_LIBMBFL_DIR" "/usr" "/usr/local"; do
          for lib in "$prefix/lib" "$prefix/lib/mbfl" "$prefix/lib/mbfl-1.0"; do
            if test -f "$lib/libmbfl.so" || test -f "$lib/libmbfl.a"; then
              PHP_CHECK_LIBRARY(mbfl, [mbfl_buffer_converter_new], [
                PHP_LIBMBFL_LIB="$lib"
              ],[], [-L$lib $MBSTRING_SHARED_LIBADD])
            fi
          done
        done
        if test -z "$PHP_LIBMBFL_INCLUDE" || test -z "$PHP_LIBMBFL_LIB"; then
          AC_MSG_ERROR([Please reinstall libmbfl library.])
        fi
        AC_DEFINE([HAVE_LIBMBFL], 1, [whether to have libmbfl support])
        PHP_ADD_LIBRARY_WITH_PATH(mbfl, "$PHP_LIBMBFL_LIB", MBSTRING_SHARED_LIBADD)
        PHP_ADD_INCLUDE([$PHP_LIBMBFL_INCLUDE])
        PHP_SUBST([MBSTRING_SHARED_LIBADD])
      fi
    fi
    	
    PHP_MBSTRING_ADD_SOURCES([
      mbstring.c php_unicode.c mb_gpc.c
    ])
  fi
])

AC_DEFUN([PHP_MBSTRING_SETUP_MBREGEX], [
  PHP_ARG_ENABLE([mbregex], [whether to enable multibyte regex support],
  [  --disable-mbregex       Disable multibyte regex support], yes, no)

  if test "$PHP_MBREGEX" != "no" -a "$PHP_MBSTRING" != "no"; then
    AC_CACHE_CHECK(for variable length prototypes and stdarg.h, cv_php_mbstring_stdarg, [
      AC_TRY_COMPILE([#include <stdarg.h>], [
int foo(int x, ...) {
	va_list va;
	va_start(va, x);
	va_arg(va, int);
	va_arg(va, char *);
	va_arg(va, double);
	return 0;
}
int main() { return foo(10, "", 3.14); }
      ], [cv_php_mbstring_stdarg=yes], [cv_php_mbstring_stdarg=no])
    ])
    if test "$cv_php_mbstring_stdarg" = "yes"; then
      AC_DEFINE([HAVE_STDARG_PROTOTYPES], 1, [Define if stdarg.h is available])
    fi
    AC_DEFINE([HAVE_MBREGEX], 1, [whether to have multibyte regex support])
    PHP_MBSTRING_ADD_CFLAG([-DNOT_RUBY])

    PHP_MBSTRING_ADD_BUILD_DIR([oniguruma])
    PHP_MBSTRING_CONFIG_HEADER([oniguruma/config.h])
    PHP_MBSTRING_ADD_SOURCES([
      php_mbregex.c
      oniguruma/regcomp.c
      oniguruma/regerror.c
      oniguruma/regexec.c
      oniguruma/reggnu.c
      oniguruma/regparse.c
      oniguruma/regposerr.c
    ])
  fi
])

PHP_MBSTRING_INIT
PHP_MBSTRING_SETUP
PHP_MBSTRING_SETUP_MBREGEX
PHP_MBSTRING_EXTENSION

