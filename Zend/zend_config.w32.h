/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2000 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 0.92 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available at through the world-wide-web at                           |
   | http://www.zend.com/license/0_92.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


#ifndef _ZEND_CONFIG_W32_H
#define _ZEND_CONFIG_W32_H


#include <string.h>
#include <windows.h>
#include <float.h>

typedef unsigned long ulong;
typedef unsigned int uint;

#define HAVE_ALLOCA 1
#define HAVE_LIMITS_H 1
#include <malloc.h>

#undef HAVE_KILL
#define HAVE_GETPID 1
/* #define HAVE_ALLOCA_H 1 */
#define HAVE_MEMCPY 1
#define HAVE_STRDUP 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_STDIOSTR_H 1
#define HAVE_CLASS_ISTDIOSTREAM
#define istdiostream stdiostream

#ifdef inline
#undef inline
#endif

#define zend_sprintf sprintf

/* This will cause the compilation process to be MUCH longer, but will generate
 * a much quicker PHP binary
 */
#ifdef ZEND_WIN32_FORCE_INLINE
# define inline __forceinline
#else
# define inline
#endif

#define finite(A) _finite(A)

#ifdef LIBZEND_EXPORTS
#	define ZEND_API __declspec(dllexport)
#else
#	define ZEND_API __declspec(dllimport)
#endif

#define ZEND_DLEXPORT		__declspec(dllexport)

#endif /* _ZEND_CONFIG_W32_H */
