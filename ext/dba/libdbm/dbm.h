/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2002 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Marcus Boerger <helly@php.net>                               |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifndef PHP_LIBDBM_H
#define PHP_LIBDBM_H

typedef struct {
	char *dptr;
	int dsize;
} datum;

typedef struct {
	php_stream *fp;
	long CurrentFlatFilePos;
	datum nextkey;
} dba_dbm_data;

#define DBM_INSERT 0
#define DBM_REPLACE 1

PHPAPI int dbm_file_store(dba_dbm_data *dba, datum key_datum, datum value_datum, int mode TSRMLS_DC);
PHPAPI datum dbm_file_fetch(dba_dbm_data *dba, datum key_datum TSRMLS_DC);
PHPAPI int dbm_file_delete(dba_dbm_data *dba, datum key_datum TSRMLS_DC);
PHPAPI int dbm_file_findkey(dba_dbm_data *dba, datum key_datum TSRMLS_DC);
PHPAPI datum dbm_file_firstkey(dba_dbm_data *dba TSRMLS_DC);
PHPAPI datum dbm_file_nextkey(dba_dbm_data *dba TSRMLS_DC);

#endif
