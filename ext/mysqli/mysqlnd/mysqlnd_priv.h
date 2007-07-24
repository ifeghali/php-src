/*
  +----------------------------------------------------------------------+
  | PHP Version 6                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2007 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Georg Richter <georg@mysql.com>                             |
  |          Andrey Hristov <andrey@mysql.com>                           |
  |          Ulf Wendel <uwendel@mysql.com>                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef MYSQLND_PRIV_H
#define MYSQLND_PRIV_H

#ifdef ZTS
#include "TSRM.h"
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef pestrndup
#define pestrndup(s, length, persistent) ((persistent)?zend_strndup((s),(length)):estrndup((s),(length)))
#endif


#define MYSQLND_CLASS_METHODS_START(class) static \
									 struct st_##class##_methods mysqlnd_##class##_methods = {
#define MYSQLND_CLASS_METHODS_END  				}							 
#define MYSQLND_METHOD(class, method) 			php_##class##_##method##_pub
#define MYSQLND_METHOD_PRIVATE(class, method)	php_##class##_##method##_priv

#if PHP_MAJOR_VERSION < 6
#define mysqlnd_array_init(arg, field_count) \
{ \
	ALLOC_HASHTABLE_REL(Z_ARRVAL_P(arg));\
	zend_hash_init(Z_ARRVAL_P(arg), (field_count), NULL, ZVAL_PTR_DTOR, 0); \
	Z_TYPE_P(arg) = IS_ARRAY;\
}
#else
#define mysqlnd_array_init(arg, field_count) \
{ \
	ALLOC_HASHTABLE_REL(Z_ARRVAL_P(arg));\
	zend_u_hash_init(Z_ARRVAL_P(arg), (field_count), NULL, ZVAL_PTR_DTOR, 0, 0);\
	Z_TYPE_P(arg) = IS_ARRAY;\
}
#endif



#define SERVER_STATUS_IN_TRANS				1	/* Transaction has started */
#define SERVER_STATUS_AUTOCOMMIT			2	/* Server in auto_commit mode */
#define SERVER_MORE_RESULTS_EXISTS			8	/* Multi query - next query exists */
/*
  The server was able to fulfill the clients request and opened a
  read-only non-scrollable cursor for a query. This flag comes
  in reply to COM_STMT_EXECUTE and COM_STMT_FETCH commands.
*/
#define SERVER_STATUS_CURSOR_EXISTS			64
/*
  This flag is sent when a read-only cursor is exhausted, in reply to
  COM_STMT_FETCH command.
*/
#define SERVER_STATUS_LAST_ROW_SENT			128
#define SERVER_STATUS_DB_DROPPED			256 /* A database was dropped */
#define SERVER_STATUS_NO_BACKSLASH_ESCAPES	512



/* Client Error codes */
#define CR_UNKNOWN_ERROR		2000
#define CR_CONNECTION_ERROR		2002
#define CR_SERVER_GONE_ERROR	2006
#define CR_OUT_OF_MEMORY		2008
#define CR_SERVER_LOST			2013
#define CR_COMMANDS_OUT_OF_SYNC	2014
#define CR_CANT_FIND_CHARSET	2019
#define CR_MALFORMED_PACKET		2027
#define CR_NOT_IMPLEMENTED		2054
#define CR_NO_PREPARE_STMT		2030
#define CR_PARAMS_NOT_BOUND		2031
#define CR_INVALID_PARAMETER_NO	2034
#define CR_INVALID_BUFFER_USE	2035

#define MYSQLND_EE_READ			 2
#define MYSQLND_EE_FILENOTFOUND	29

#define UNKNOWN_SQLSTATE		"HY000"

#define MAX_CHARSET_LEN			32


#define SET_ERROR_AFF_ROWS(s)	(s)->upsert_status.affected_rows = (mynd_ulonglong) ~0

/* Error handling */
#define SET_NEW_MESSAGE(buf, buf_len, message, len) \
	{\
		if ((buf)) { \
			efree((buf)); \
		} \
		(buf) = (message); \
		(buf_len) = (len); \
		/* Transfer ownership*/ \
		(message) = NULL; \
	}

#define SET_EMPTY_MESSAGE(buf, buf_len) \
	{\
		if ((buf)) { \
			efree((buf)); \
			(buf) = NULL; \
		} \
		(buf_len) = 0; \
	}


#define SET_EMPTY_ERROR(error_info) \
	{ \
		error_info.error_no = 0; \
		error_info.error[0] = '\0'; \
		strcpy(error_info.sqlstate, "00000"); \
	}

#define SET_CLIENT_ERROR(error_info, a, b, c) \
	{ \
		error_info.error_no = a; \
		strncpy(error_info.sqlstate, b, sizeof(error_info.sqlstate)); \
		strncpy(error_info.error, c, sizeof(error_info.error)); \
	}

#define SET_STMT_ERROR(stmt, a, b, c)	SET_CLIENT_ERROR(stmt->error_info, a, b, c)



/* PS stuff */
typedef void (*ps_field_fetch_func)(zval *zv, const MYSQLND_FIELD * const field,
									uint pack_len, zend_uchar **row,
									zend_bool everything_as_unicode TSRMLS_DC);
struct st_mysqlnd_perm_bind {
	ps_field_fetch_func func;
	/* should be signed int */
	int					pack_len;
	unsigned int		php_type;
	zend_bool			is_possibly_blob;
	zend_bool			can_ret_as_str_in_uni;
};

extern struct st_mysqlnd_perm_bind mysqlnd_ps_fetch_functions[MYSQL_TYPE_LAST + 1];

extern const char * mysqlnd_out_of_sync;


enum_func_status mysqlnd_handle_local_infile(MYSQLND *conn, const char *filename, zend_bool *is_warning TSRMLS_DC);


void _mysqlnd_init_ps_subsystem();/* This one is private, mysqlnd_library_init() will call it */

void mysqlnd_stmt_execute_store_params(MYSQLND_STMT *stmt, zend_uchar **buf, zend_uchar **p,
										size_t *buf_len, unsigned int null_byte_offset);

void ps_fetch_from_1_to_8_bytes(zval *zv, const MYSQLND_FIELD * const field,
								uint pack_len, zend_uchar **row, zend_bool as_unicode,
								unsigned int byte_count, zend_bool is_bit TSRMLS_DC);



int mysqlnd_set_sock_no_delay(php_stream *stream);
#endif	/* MYSQLND_PRIV_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
