/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2003 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Marcus Boerger <helly@php.net>                              |
   | based on ext/db/db.c by:                                             |
   |          Rasmus Lerdorf <rasmus@php.net>                             |
   |          Jim Winstead <jimw@php.net>                                 |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_globals.h"
#include "safe_mode.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "flatfile.h"

/*
 * ret = -1 means that database was opened for read-only
 * ret = 0  success
 * ret = 1  key already exists - nothing done
 */

/* {{{ flatfile_store
 */
int flatfile_store(flatfile *dba, datum key_datum, datum value_datum, int mode TSRMLS_DC) {
	if (mode == FLATFILE_INSERT) {
		if (flatfile_findkey(dba, key_datum TSRMLS_CC)) {
			return 1;
		}
		php_stream_seek(dba->fp, 0L, SEEK_END);
		php_stream_printf(dba->fp TSRMLS_CC, "%d\n", key_datum.dsize);
		php_stream_flush(dba->fp);
		if (php_stream_write(dba->fp, key_datum.dptr, key_datum.dsize) < key_datum.dsize) {
			return -1;
		}
		php_stream_printf(dba->fp TSRMLS_CC, "%d\n", value_datum.dsize);
		php_stream_flush(dba->fp);
		if (php_stream_write(dba->fp, value_datum.dptr, value_datum.dsize) < value_datum.dsize) {
			return -1;
		}
	} else { /* FLATFILE_REPLACE */
		flatfile_delete(dba, key_datum TSRMLS_CC);
		php_stream_printf(dba->fp TSRMLS_CC, "%d\n", key_datum.dsize);
		php_stream_flush(dba->fp);
		if (php_stream_write(dba->fp, key_datum.dptr, key_datum.dsize) < key_datum.dsize) {
			return -1;
		}
		php_stream_printf(dba->fp TSRMLS_CC, "%d\n", value_datum.dsize);
		if (php_stream_write(dba->fp, value_datum.dptr, value_datum.dsize) < value_datum.dsize) {
			return -1;
		}
	}

	php_stream_flush(dba->fp);
	return 0;
}
/* }}} */

/* {{{ flatfile_fetch
 */
datum flatfile_fetch(flatfile *dba, datum key_datum TSRMLS_DC) {
	datum value_datum = {NULL, 0};
	size_t num=0;
	size_t buf_size = 1024;
	char *buf;	

	if (flatfile_findkey(dba, key_datum TSRMLS_CC)) {
		buf = emalloc((buf_size+1) * sizeof(char));
		if (php_stream_gets(dba->fp, buf, 15)) {
			num = atoi(buf);
			if (num > buf_size) {
				buf_size+=num;
				buf = erealloc(buf, (buf_size+1)*sizeof(char));
			}
			php_stream_read(dba->fp, buf, num);
			value_datum.dptr = buf;
			value_datum.dsize = num;
		} else {
			value_datum.dptr = NULL;
			value_datum.dsize = 0;
			efree(buf);
		}
	}
	return value_datum;
}
/* }}} */

/* {{{ flatfile_delete
 */
int flatfile_delete(flatfile *dba, datum key_datum TSRMLS_DC) {
	char *key = key_datum.dptr;
	size_t size = key_datum.dsize;

	char *buf;
	size_t num;
	size_t buf_size = 1024;
	size_t pos;

	php_stream_rewind(dba->fp);

	buf = emalloc((buf_size + 1)*sizeof(char));
	while(!php_stream_eof(dba->fp)) {
		/* read in the length of the key name */
		if (!php_stream_gets(dba->fp, buf, 15)) {
			break;
		}
		num = atoi(buf);
		if (num > buf_size) {
			buf_size += num;
			buf = erealloc(buf, (buf_size+1)*sizeof(char));
		}
		pos = php_stream_tell(dba->fp);

		/* read in the key name */
		num = php_stream_read(dba->fp, buf, sizeof(char)*num);
		if (num < 0)  {
			break;
		}
		*(buf+num) = '\0';

		if (size == num && !memcmp(buf, key, size)) {
			php_stream_seek(dba->fp, pos, SEEK_SET);
			php_stream_putc(dba->fp, 0);
			php_stream_flush(dba->fp);
			php_stream_seek(dba->fp, 0L, SEEK_END);
			if (buf)  {
				efree(buf);
			}
			return SUCCESS;
		}	

		/* read in the length of the value */
		if (!php_stream_gets(dba->fp, buf, 15))
			break;
		num = atoi(buf);
		if (num > buf_size) {
			buf_size+=num;
			buf = erealloc(buf, (buf_size+1)*sizeof(char));
		}
		/* read in the value */
		num = php_stream_read(dba->fp, buf, sizeof(char)*num);
		if (num < 0) {
			break;
		}
	}
	if (buf) {
		efree(buf);
	}
	return FAILURE;
}	
/* }}} */

/* {{{ flatfile_findkey
 */
int flatfile_findkey(flatfile *dba, datum key_datum TSRMLS_DC) {
	char *buf = NULL;
	size_t num;
	size_t buf_size = 1024;
	int ret=0;
	void *key = key_datum.dptr;
	size_t size = key_datum.dsize;

	php_stream_rewind(dba->fp);
	buf = emalloc((buf_size+1)*sizeof(char));
	while (!php_stream_eof(dba->fp)) {
		if (!php_stream_gets(dba->fp, buf, 15))  {
			break;
		}
		num = atoi(buf);
		if (num > buf_size) {
			buf_size+=num;
			buf = erealloc(buf, (buf_size+1)*sizeof(char));
		}
		num = php_stream_read(dba->fp, buf, sizeof(char)*num);
		if (num<0) break;
		*(buf+num) = '\0';
		if (size == num) {
			if (!memcmp(buf, key, size)) {
				ret = 1;
				break;
			}
		}	
		if (!php_stream_gets(dba->fp, buf, 15)) {
			break;
		}
		num = atoi(buf);
		if (num > buf_size) {
			buf_size+=num;
			buf = erealloc(buf, (buf_size+1)*sizeof(char));
		}
		num = php_stream_read(dba->fp, buf, sizeof(char)*num);
		if (num < 0) {
			break;
		}
		*(buf+num) = '\0';
	}
	if (buf) {
		efree(buf);
	}
	return(ret);
}
/* }}} */

/* {{{ flatfile_firstkey
 */
datum flatfile_firstkey(flatfile *dba TSRMLS_DC) {
	datum buf;
	size_t num;
	size_t buf_size = 1024;

	php_stream_rewind(dba->fp);
	buf.dptr = emalloc((buf_size+1)*sizeof(char));
	while(!php_stream_eof(dba->fp)) {
		if (!php_stream_gets(dba->fp, buf.dptr, 15)) {
			break;
		}
		num = atoi(buf.dptr);
		if (num > buf_size) {
			buf_size+=num;
			buf.dptr = erealloc(buf.dptr, (buf_size+1)*sizeof(char));
		}
		num = php_stream_read(dba->fp, buf.dptr, num);
		if (num < 0) {
			break;
		}
		buf.dsize = num;
		if (*(buf.dptr)!=0) {
			dba->CurrentFlatFilePos = php_stream_tell(dba->fp);
			return(buf);
		}
		if (!php_stream_gets(dba->fp, buf.dptr, 15)) break;
		num = atoi(buf.dptr);
		if (num > buf_size) {
			buf_size+=num;
			buf.dptr = erealloc(buf.dptr, (buf_size+1)*sizeof(char));
		}
		num = php_stream_read(dba->fp, buf.dptr, num);
		if (num < 0) {
			break;
		}
	}
	if (buf.dptr)  {
		efree(buf.dptr);
	}
	buf.dptr = NULL;
	return(buf);
}
/* }}} */

/* {{{ flatfile_nextkey
 */
datum flatfile_nextkey(flatfile *dba TSRMLS_DC) {
	datum buf;
	size_t num;
	size_t buf_size = 1024;

	php_stream_seek(dba->fp, dba->CurrentFlatFilePos, SEEK_SET);
	buf.dptr = emalloc((buf_size+1)*sizeof(char));
	while(!php_stream_eof(dba->fp)) {
		if (!php_stream_gets(dba->fp, buf.dptr, 15)) {
			break;
		}
		num = atoi(buf.dptr);
		if (num > buf_size) {
			buf_size+=num;
			buf.dptr = erealloc(buf.dptr, (buf_size+1)*sizeof(char));
		}
		num = php_stream_read(dba->fp, buf.dptr, num);
		if (num < 0)  {
			break;
		}
		if (!php_stream_gets(dba->fp, buf.dptr, 15)) {
			break;
		}
		num = atoi(buf.dptr);
		if (num > buf_size) {
			buf_size+=num;
			buf.dptr = erealloc(buf.dptr, (buf_size+1)*sizeof(char));
		}
		num = php_stream_read(dba->fp, buf.dptr, num);
		if (num < 0) {
			break;
		}
		buf.dsize = num;
		if (*(buf.dptr)!=0) {
			dba->CurrentFlatFilePos = php_stream_tell(dba->fp);
			return(buf);
		}
	}
	if (buf.dptr) {
		efree(buf.dptr);
	}
	buf.dptr = NULL;
	return(buf);
}	
/* }}} */

/* {{{ flatfile_version */
char *flatfile_version() 
{
	return "1.0, $Revision$";
}
/* }}} */ 

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
