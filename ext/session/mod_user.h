/* 
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Sascha Schumann <ss@2ns.de>                                 |
   +----------------------------------------------------------------------+
 */

#ifndef MOD_USER_H
#define MOD_USER_H

typedef union {
	char *names[6];
	struct {
		char *ps_open;
		char *ps_close;
		char *ps_read;
		char *ps_write;
		char *ps_destroy;
		char *ps_gc;
	} name;
} ps_user;

extern ps_module ps_mod_user;
#define ps_user_ptr &ps_mod_user

PS_FUNCS(user);

#endif
