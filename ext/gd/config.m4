AC_DEFUN(PHP_GD_JPEG,[
        AC_MSG_CHECKING([for libjpeg (needed by gd-1.8+)])
        AC_ARG_WITH(jpeg-dir,
        [  --with-jpeg-dir[=DIR]   jpeg dir for gd-1.8+],[
          AC_MSG_RESULT(yes)
          if test "$withval" = "yes"; then
            withval="/usr/local"
          fi
          old_LIBS=$LIBS
          LIBS="$LIBS -L$withval/lib"
          AC_CHECK_LIB(jpeg,jpeg_read_header, [LIBS="$LIBS -L$withval/lib -ljpeg"],[AC_MSG_RESULT(no)],)
          LIBS=$old_LIBS
          AC_ADD_LIBRARY_WITH_PATH(jpeg, $withval/lib)
          LIBS="$LIBS -L$withval/lib -ljpeg"
        ],[
          AC_MSG_RESULT(no)
          AC_MSG_WARN(If configure fails try --with-jpeg-dir=<DIR>)
        ]) 
        AC_CHECK_LIB(gd, gdImageCreateFromJpeg, [AC_DEFINE(HAVE_GD_JPG, 1, [ ])])
])


AC_DEFUN(PHP_GD_XPM,[
        AC_MSG_CHECKING([for libXpm (needed by gd-1.8+)])
        AC_ARG_WITH(xpm-dir,
        [  --with-xpm-dir[=DIR]    xpm dir for gd-1.8+],[
          AC_MSG_RESULT(yes)
          if test "$withval" = "yes"; then
            withval="/usr/X11R6"
          fi
          old_LIBS=$LIBS
          LIBS="$LIBS -L$withval/lib"
          AC_CHECK_LIB(Xpm,XpmFreeXpmImage, [LIBS="$LIBS -L$withval/lib -lXpm"],[AC_MSG_RESULT(no)],)
          LIBS=$old_LIBS
          AC_ADD_LIBRARY_WITH_PATH(Xpm, $withval/lib)
          AC_ADD_LIBRARY_WITH_PATH(X11, $withval/lib)
          LIBS="$LIBS -L$withval/lib -lXpm -lX11"
        ],[
          AC_MSG_RESULT(no)
          AC_MSG_WARN(If configure fails try --with-xpm-dir=<DIR>)
        ]) 
        AC_CHECK_LIB(gd, gdImageCreateFromXpm, [AC_DEFINE(HAVE_GD_XPM, 1, [ ])])
])


shared=no
AC_MSG_CHECKING(whether to include GD support)
AC_ARG_WITH(gd,
[  --without-gd            Disable GD support.
  --with-gd[=DIR]         Include GD support (DIR is GD's install dir).
                          Set DIR to "shared" to build as a dl, or 
                          "shared,DIR" to build as a dl and still specify DIR.],
[
  PHP_WITH_SHARED

  case "$withval" in
    no)
      AC_MSG_RESULT(no) ;;
    yes)
      AC_DEFINE(HAVE_LIBGD,1,[ ])
      if test "$shared" = "yes"; then
        AC_MSG_RESULT(yes (shared))
        GD_LIBS="-lgd"
      else
        AC_MSG_RESULT(yes (static))
        AC_ADD_LIBRARY(gd)
      fi
        old_LDFLAGS=$LDFLAGS
		old_LIBS=$LIBS
        AC_CHECK_LIB(gd, gdImageString16, [ AC_DEFINE(HAVE_LIBGD13,1,[ ]) ])
        AC_CHECK_LIB(gd, gdImagePaletteCopy, [ AC_DEFINE(HAVE_LIBGD15,1,[ ]) ])
        AC_CHECK_LIB(gd, gdImageColorClosestHWB, [ AC_DEFINE(HAVE_COLORCLOSESTHWB,1,[ ]) ])
		AC_CHECK_LIB(z,compress, LIBS="$LIBS -lz",,)
		AC_CHECK_LIB(png,png_info_init, LIBS="$LIBS -lpng",,)
        AC_CHECK_LIB(gd, gdImageColorResolve, [AC_DEFINE(HAVE_GDIMAGECOLORRESOLVE,1,[ ])])
dnl Some versions of GD support both PNG and GIF. Check for both.
        AC_CHECK_LIB(gd, gdImageCreateFromPng, [AC_DEFINE(HAVE_GD_PNG, 1, [ ])])
        AC_CHECK_LIB(gd, gdImageCreateFromGif, [AC_DEFINE(HAVE_GD_GIF, 1, [ ])])
        AC_CHECK_LIB(gd, gdImageCreateFromXbm, [AC_DEFINE(HAVE_GD_XBM, 1, [ ])])
        AC_CHECK_LIB(gd, gdImageWBMP, [AC_DEFINE(HAVE_GD_WBMP, 1, [ ])])
        
        LIBS=$old_LIBS
        LDFLAGS=$old_LDFLAGS
        if test "$ac_cv_lib_gd_gdImageCreateFromPng" = "yes"; then
          AC_ADD_LIBRARY(png)
          AC_ADD_LIBRARY(z)
        fi

        PHP_GD_JPEG
        PHP_GD_XPM
        ac_cv_lib_gd_gdImageLine=yes
      ;;
    *)
dnl A whole whack of possible places where this might be
      test -f $withval/include/gd1.3/gd.h && GD_INCLUDE="$withval/include/gd1.3"
      test -f $withval/include/gd/gd.h && GD_INCLUDE="$withval/include/gd"
      test -f $withval/include/gd.h && GD_INCLUDE="$withval/include"
      test -f $withval/gd1.3/gd.h && GD_INCLUDE="$withval/gd1.3"
      test -f $withval/gd/gd.h && GD_INCLUDE="$withval/gd"
      test -f $withval/gd.h && GD_INCLUDE="$withval"

      test -f $withval/lib/libgd.so && GD_LIB="$withval/lib"
      test -f $withval/lib/gd/libgd.so && GD_LIB="$withval/lib/gd"
      test -f $withval/lib/gd1.3/libgd.so && GD_LIB="$withval/lib/gd1.3"
      test -f $withval/libgd.so && GD_LIB="$withval"
      test -f $withval/gd/libgd.so && GD_LIB="$withval/gd"
      test -f $withval/gd1.3/libgd.so && GD_LIB="$withval/gd1.3"

      test -f $withval/lib/libgd.a && GD_LIB="$withval/lib"
      test -f $withval/lib/gd/libgd.a && GD_LIB="$withval/lib/gd"
      test -f $withval/lib/gd1.3/libgd.a && GD_LIB="$withval/lib/gd1.3"
      test -f $withval/libgd.a && GD_LIB="$withval"
      test -f $withval/gd/libgd.a && GD_LIB="$withval/gd"
      test -f $withval/gd1.3/libgd.a && GD_LIB="$withval/gd1.3"
      if test -n "$GD_INCLUDE" && test -n "$GD_LIB" ; then
        AC_DEFINE(HAVE_LIBGD,1,[ ])
        if test "$shared" = "yes"; then
          AC_MSG_RESULT(yes (shared))
          GD_LIBS="-lgd"
          GD_LFLAGS="-L$GD_LIB"
        else
          AC_MSG_RESULT(yes (static))
          AC_ADD_LIBRARY_WITH_PATH(gd, $GD_LIB)
        fi
        old_LDFLAGS=$LDFLAGS
        LDFLAGS="$LDFLAGS -L$GD_LIB"
		old_LIBS=$LIBS
        AC_CHECK_LIB(gd, gdImageString16, [ AC_DEFINE(HAVE_LIBGD13,1, [ ]) ])
		AC_CHECK_LIB(z,compress, LIBS="$LIBS -lz",,)
		AC_CHECK_LIB(png,png_info_init, LIBS="$LIBS -lpng",,)
        AC_CHECK_LIB(gd, gdImageColorResolve, [AC_DEFINE(HAVE_GDIMAGECOLORRESOLVE,1,[ ])])
        AC_CHECK_LIB(gd, gdImageCreateFromPng, [AC_DEFINE(HAVE_GD_PNG, 1, [ ])])
        AC_CHECK_LIB(gd, gdImageCreateFromGif, [AC_DEFINE(HAVE_GD_GIF, 1, [ ])])
        AC_CHECK_LIB(gd, gdImageCreateFromXbm, [AC_DEFINE(HAVE_GD_XBM, 1, [ ])])
        AC_CHECK_LIB(gd, gdImageWBMP, [AC_DEFINE(HAVE_GD_WBMP, 1, [ ])])
        
        LIBS=$old_LIBS
        LDFLAGS=$old_LDFLAGS
        if test "$ac_cv_lib_gd_gdImageCreateFromPng" = "yes"; then
          AC_ADD_LIBRARY(z)
          AC_ADD_LIBRARY(png)
        fi
        PHP_GD_JPEG
	PHP_GD_XPM

        ac_cv_lib_gd_gdImageLine=yes
      else
        AC_MSG_ERROR([Unable to find libgd.(a|so) anywhere under $withval])
      fi ;;
  esac
],[
  AC_CHECK_LIB(gd, gdImageLine)
  AC_CHECK_LIB(gd, gdImageString16, [ AC_DEFINE(HAVE_LIBGD13,1, [ ]) ])
  if test "$ac_cv_lib_gd_gdImageLine" = "yes"; then
		old_LIBS=$LIBS
        AC_CHECK_LIB(gd, gdImageString16, [ AC_DEFINE(HAVE_LIBGD13,1, [ ]) ])
		AC_CHECK_LIB(z,compress, LIBS="$LIBS -lz",,)
		AC_CHECK_LIB(png,png_info_init, LIBS="$LIBS -lpng",,)
        AC_CHECK_LIB(gd, gdImageColorResolve, [AC_DEFINE(HAVE_GDIMAGECOLORRESOLVE,1, [ ])])
        AC_CHECK_LIB(gd, gdImageCreateFromPng, [AC_DEFINE(HAVE_GD_PNG, 1, [ ])])
        AC_CHECK_LIB(gd, gdImageCreateFromGif, [AC_DEFINE(HAVE_GD_GIF, 1, [ ])])
        
        LIBS=$old_LIBS
        LDFLAGS=$old_LDFLAGS
        if test "$ac_cv_lib_gd_gdImageCreateFromPng" = "yes"; then
          AC_ADD_LIBRARY(z)
          AC_ADD_LIBRARY(png)
        fi
        ac_cv_lib_gd_gdImageLine=yes
  fi
])
if test "$with_gd" != "no" && test "$ac_cv_lib_gd_gdImageLine" = "yes"; then
  CHECK_TTF="yes"
  AC_ARG_WITH(ttf,
  [  --with-ttf[=DIR]        Include Freetype support],[
    if test $withval = "no" ; then
      CHECK_TTF=""
    else
      CHECK_TTF="$withval"
    fi
  ])

  AC_MSG_CHECKING(whether to include ttf support)
  if test -n "$CHECK_TTF" ; then
    for i in /usr /usr/local "$CHECK_TTF" ; do
      if test -f "$i/include/truetype.h" ; then
        FREETYPE_DIR="$i"
      fi
      if test -f "$i/include/freetype.h" ; then
        TTF_DIR="$i"
        unset TTF_INC_DIR
      fi
      if test -f "$i/include/freetype/freetype.h"; then
        TTF_DIR="$i"
        TTF_INC_DIR="$i/include/freetype"
      fi
    done
    if test -n "$FREETYPE_DIR" ; then
      AC_DEFINE(HAVE_LIBFREETYPE,1,[ ])
      if test "$shared" = "yes"; then
        GD_LIBS="$GD_LIBS -lfreetype"
        GD_LFLAGS="$GD_LFLAGS -L$FREETYPE_DIR/lib"
      else 
        AC_ADD_LIBRARY_WITH_PATH(freetype, $FREETYPE_DIR/lib)
      fi
      AC_ADD_INCLUDE($FREETYPE_DIR/include)
      AC_MSG_RESULT(yes)
    else
      if test -n "$TTF_DIR" ; then
        AC_DEFINE(HAVE_LIBTTF,1,[ ])
        if test "$shared" = "yes"; then
          GD_LIBS="$GD_LIBS -lttf"
          GD_LFLAGS="$GD_LFLAGS -L$TTF_DIR/lib"
        else
          AC_ADD_LIBRARY_WITH_PATH(ttf, $TTF_DIR/lib)
        fi
        if test -z "$TTF_INC_DIR"; then
          TTF_INC_DIR="$TTF_DIR/include"
        fi
        AC_ADD_INCLUDE($TTF_INC_DIR)
        AC_MSG_RESULT(yes)
      else
        AC_MSG_RESULT(no)
      fi
    fi
  else
    AC_MSG_RESULT(no)
  fi
  
AC_MSG_CHECKING(for T1lib support)
AC_ARG_WITH(t1lib,
[  --with-t1lib[=DIR]      Include T1lib support.],
[
  if test "$withval" != "no"; then
    if test "$withval" = "yes"; then
      for i in /usr/local /usr; do
        if test -f "$i/include/t1lib.h"; then
          AC_ADD_LIBRARY_WITH_PATH(t1, "$i/lib")
          AC_ADD_INCLUDE("$i/include")
        fi
      done
    else
      if test -f "$withval/include/t1lib.h"; then
        AC_ADD_LIBRARY_WITH_PATH(t1, "$withval/lib")
        AC_ADD_INCLUDE("$withval/include")
      fi
    fi
    AC_CHECK_LIB(t1, T1_GetExtend, [AC_DEFINE(HAVE_LIBT1,1,[ ])])
    if test "$ac_cv_lib_t1_T1_GetExtend" = "yes"; then
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
    fi
  else
    AC_MSG_RESULT(no)
  fi
],[
  AC_MSG_RESULT(no)
])

dnl NetBSD package structure
  if test -f /usr/pkg/include/gd/gd.h -a -z "$GD_INCLUDE" ; then
    GD_INCLUDE="/usr/pkg/include/gd"
  fi

dnl SuSE 6.x package structure
  if test -f /usr/include/gd/gd.h -a -z "$GD_INCLUDE" ; then
    GD_INCLUDE="/usr/include/gd"
  fi

  AC_MSG_CHECKING(whether to enable 4bit antialias hack with FreeType2)
  AC_ARG_ENABLE(freetype-4bit-antialias-hack,
  [  --enable-freetype-4bit-antialias-hack  
                          Include support for FreeType2 (experimental).],[
  if test "$enableval" = "yes" ; then
    AC_DEFINE(FREETYPE_4BIT_ANTIALIAS_HACK, 1, [ ])
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
  ],[
    AC_MSG_RESULT(no)
  ])
  
  AC_EXPAND_PATH($GD_INCLUDE, GD_INCLUDE)
  AC_ADD_INCLUDE($GD_INCLUDE)
  PHP_EXTENSION(gd, $shared)
  if test "$shared" != "yes"; then
    GD_STATIC="libphpext_gd.la"
  else 
    GD_SHARED="gd.la"
  fi
fi

PHP_SUBST(GD_LFLAGS)
PHP_SUBST(GD_LIBS)
