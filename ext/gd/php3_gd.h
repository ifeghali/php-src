/* 
   +----------------------------------------------------------------------+
   | PHP HTML Embedded Scripting Language Version 3.0                     |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997 PHP Development Team (See Credits file)           |
   +----------------------------------------------------------------------+
   | This program is free software; you can redistribute it and/or modify |
   | it under the terms of one of the following licenses:                 |
   |                                                                      |
   |  A) the GNU General Public License as published by the Free Software |
   |     Foundation; either version 2 of the License, or (at your option) |
   |     any later version.                                               |
   |                                                                      |
   |  B) the PHP License as published by the PHP Development Team and     |
   |     included in the distribution in the file: LICENSE                |
   |                                                                      |
   | This program is distributed in the hope that it will be useful,      |
   | but WITHOUT ANY WARRANTY; without even the implied warranty of       |
   | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        |
   | GNU General Public License for more details.                         |
   |                                                                      |
   | You should have received a copy of both licenses referred to here.   |
   | If you did not, or have any questions about PHP licensing, please    |
   | contact core@php.net.                                                |
   +----------------------------------------------------------------------+
   | Authors: Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   |          Stig Bakken <ssb@guardian.no>                               |
   +----------------------------------------------------------------------+
 */


/* $Id$ */

#ifndef _PHP3_GD_H
#define _PHP3_GD_H


#if defined(HAVE_LIBTTF) && defined(HAVE_FREETYPE_H)
#define ENABLE_GD_TTF
#endif

#if COMPILE_DL
#undef HAVE_LIBGD
#define HAVE_LIBGD 1
#endif

#if HAVE_LIBGD

#include <gd.h>

extern php3_module_entry gd_module_entry;
#define phpext_gd_ptr &gd_module_entry

/* gd.c functions */
void php3_info_gd(ZEND_MODULE_INFO_FUNC_ARGS);
extern int php3_minit_gd(INIT_FUNC_ARGS);
extern int php3_mend_gd(SHUTDOWN_FUNC_ARGS);
extern int gdImageColorResolve(gdImagePtr, int, int, int);
PHP_FUNCTION(imagearc);
PHP_FUNCTION(imagechar);
PHP_FUNCTION(imagecharup);
PHP_FUNCTION(imagecolorallocate);
PHP_FUNCTION(imagecolorat);
PHP_FUNCTION(imagecolorclosest);
PHP_FUNCTION(imagecolordeallocate);
PHP_FUNCTION(imagecolorresolve);
PHP_FUNCTION(imagecolorexact);
PHP_FUNCTION(imagecolorset);
PHP_FUNCTION(imagecolorstotal);
PHP_FUNCTION(imagecolorsforindex);
PHP_FUNCTION(imagecolortransparent);
PHP_FUNCTION(imagecopy);
PHP_FUNCTION(imagecopyresized);
PHP_FUNCTION(imagecreate);
PHP_FUNCTION(imagecreatefromgif );
PHP_FUNCTION(imagedestroy);
PHP_FUNCTION(imagefill);
PHP_FUNCTION(imagefilledpolygon);
PHP_FUNCTION(imagefilledrectangle);
PHP_FUNCTION(imagefilltoborder);
PHP_FUNCTION(imagefontwidth);
PHP_FUNCTION(imagefontheight);
PHP_FUNCTION(imagegif );
PHP_FUNCTION(imageinterlace);
PHP_FUNCTION(imageline);
PHP_FUNCTION(imageloadfont);
PHP_FUNCTION(imagepolygon);
PHP_FUNCTION(imagerectangle);
PHP_FUNCTION(imagesetpixel);
PHP_FUNCTION(imagestring);
PHP_FUNCTION(imagestringup);
PHP_FUNCTION(imagesxfn);
PHP_FUNCTION(imagesyfn);
void php3_free_gd_font(gdFontPtr);
void _php3_gdimagecharup(gdImagePtr, gdFontPtr, int, int, int, int);
PHP_FUNCTION(imagedashedline);
#ifdef ENABLE_GD_TTF
PHP_FUNCTION(imagettfbbox);
PHP_FUNCTION(imagettftext);
#endif
#else

#define phpext_gd_ptr NULL

#endif

#endif /* _PHP3_GD_H */
