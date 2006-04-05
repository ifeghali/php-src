/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2006 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Stig S�ther Bakken <ssb@php.net>                            |
   |          Thies C. Arntzen <thies@thieso.net>                         |
   |                                                                      |
   | Collection support by Andy Sautins <asautins@veripost.net>           |
   | Temporary LOB support by David Benson <dbenson@mancala.com>          |
   | ZTS per process OCIPLogon by Harald Radi <harald.radi@nme.at>        |
   |                                                                      |
   | Redesigned by: Antony Dovgal <antony@zend.com>                       |
   |                Andi Gutmans <andi@zend.com>                          |
   |                Wez Furlong <wez@omniti.com>                          |
   +----------------------------------------------------------------------+
*/

/* $Id$ */



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_ini.h"

#if HAVE_OCI8

#include "php_oci8.h"
#include "php_oci8_int.h"

/* for import/export functions */
#include <fcntl.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

/* {{{ php_oci_lob_create()
 Create LOB descriptor and allocate all the resources needed */
php_oci_descriptor *php_oci_lob_create (php_oci_connection *connection, long type TSRMLS_DC)
{
	php_oci_descriptor *descriptor;

	switch (type) {
		case OCI_DTYPE_FILE:
		case OCI_DTYPE_LOB:
		case OCI_DTYPE_ROWID:
			/* these three are allowed */
			break;
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown descriptor type %ld.", type);
			return NULL;
			break;
	}

	descriptor = ecalloc(1, sizeof(php_oci_descriptor));
	descriptor->type = type;

	OCI_G(errcode) = PHP_OCI_CALL(OCIDescriptorAlloc, (connection->env, (dvoid*)&(descriptor->descriptor), descriptor->type, (size_t) 0, (dvoid **) 0));

	if (OCI_G(errcode) != OCI_SUCCESS) {
		connection->errcode = php_oci_error(OCI_G(err), OCI_G(errcode) TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		efree(descriptor);
		return NULL;
	}

	descriptor->connection = connection;

	PHP_OCI_REGISTER_RESOURCE(descriptor, le_descriptor);
	
	descriptor->lob_current_position = 0;
	descriptor->lob_size = -1;				/* we should set it to -1 to know, that it's just not initialized */
	descriptor->buffering = PHP_OCI_LOB_BUFFER_DISABLED;				/* buffering is off by default */

	if (descriptor->type == OCI_DTYPE_LOB || descriptor->type == OCI_DTYPE_FILE) {
		/* add Lobs & Files to hash. we'll flush them at the end */
		if (!connection->descriptors) {
			ALLOC_HASHTABLE(connection->descriptors);
			zend_hash_init(connection->descriptors, 0, NULL, php_oci_descriptor_flush_hash_dtor, 0);
		}

		zend_hash_next_index_insert(connection->descriptors,&descriptor,sizeof(php_oci_descriptor *),NULL);
	}
	return descriptor;

} /* }}} */

/* {{{ php_oci_lob_get_length() 
 Get length of the LOB. The length is cached so we don't need to ask Oracle every time */
int php_oci_lob_get_length (php_oci_descriptor *descriptor, ub4 *length TSRMLS_DC)
{
	php_oci_connection *connection = descriptor->connection;

	*length = 0;
	
	if (descriptor->lob_size >= 0) {
		*length = descriptor->lob_size;
		return 0;
	} else {
		if (descriptor->type == OCI_DTYPE_FILE) {
			connection->errcode = PHP_OCI_CALL(OCILobFileOpen, (connection->svc, connection->err, descriptor->descriptor, OCI_FILE_READONLY));
			if (connection->errcode != OCI_SUCCESS) {
				php_oci_error(connection->err, connection->errcode TSRMLS_CC);
				PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
				return 1;
			}
		}
		
		connection->errcode = PHP_OCI_CALL(OCILobGetLength, (connection->svc, connection->err, descriptor->descriptor, (ub4 *)length));

		if (connection->errcode != OCI_SUCCESS) {
			php_oci_error(connection->err, connection->errcode TSRMLS_CC);
			PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
			return 1;
		}

		descriptor->lob_size = *length;

		if (descriptor->type == OCI_DTYPE_FILE) {
			connection->errcode = PHP_OCI_CALL(OCILobFileClose, (connection->svc, connection->err, descriptor->descriptor));

			if (connection->errcode != OCI_SUCCESS) {
				php_oci_error(connection->err, connection->errcode TSRMLS_CC);
				PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
				return 1;
			}
		}
	}
	return 0;	
} /* }}} */

/* {{{ php_oci_lob_read() 
 Read specified portion of the LOB into the buffer */
int php_oci_lob_read (php_oci_descriptor *descriptor, long read_length, long initial_offset, char **data, ub4 *data_len TSRMLS_DC)
{
	php_oci_connection *connection = descriptor->connection;
	ub4 length = 0;
	int bytes_read, bytes_total = 0, offset = 0;
	int requested_len = read_length; /* this is by default */
#if defined(HAVE_OCI_LOB_READ2)
	int chars_read = 0, is_clob = 0;
#endif

	*data_len = 0;
	*data = NULL;

	if (php_oci_lob_get_length(descriptor, &length TSRMLS_CC)) {
		return 1;
	}

	if (length <= 0) {
		return 0;
	}
 	
	if (initial_offset > length) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Offset must be less than size of the LOB");
		return 1;
	}
		
	if (read_length == -1) {
		requested_len = length;
	}
	
	if (requested_len > (length - initial_offset)) {
		requested_len = length - initial_offset;
	}
	
	if (requested_len <= 0) {
		return 0;
	}

	if (descriptor->type == OCI_DTYPE_FILE) {
		connection->errcode = PHP_OCI_CALL(OCILobFileOpen, (connection->svc, connection->err, descriptor->descriptor, OCI_FILE_READONLY));

		if (connection->errcode != OCI_SUCCESS) {
			php_oci_error(connection->err, connection->errcode TSRMLS_CC);
			PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
			return 1;
		}
	}
#ifdef HAVE_OCI_LOB_READ2
	else {
		ub2 charset_id = 0;

		connection->errcode = PHP_OCI_CALL(OCILobCharSetId, (connection->env, connection->err, descriptor->descriptor, &charset_id));

		if (connection->errcode != OCI_SUCCESS) {
			php_oci_error(connection->err, connection->errcode TSRMLS_CC);
			PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
			return 1;
		}

		if (charset_id > 0) { /* charset_id is always > 0 for [N]CLOBs */
			is_clob = 1;
		}
	}
#endif

	*data = (char *)emalloc(requested_len + 1);
	bytes_read = requested_len;
	offset = initial_offset;
		
	/* TODO
	 * We need to make sure this function works with Unicode LOBs
	 * */

#if defined(HAVE_OCI_LOB_READ2)

	do {
		chars_read = 0;
		connection->errcode = PHP_OCI_CALL(OCILobRead2, 
			(
				connection->svc, 
				connection->err, 
				descriptor->descriptor, 
				(oraub8 *)&bytes_read,								/* IN/OUT bytes toread/read */
				(oraub8 *)&chars_read,
				(oraub8) offset + 1,								/* offset (starts with 1) */ 
				(dvoid *) ((char *) *data + *data_len),	
				(oraub8) requested_len,									/* size of buffer */ 
				0, 
				NULL,
				(OCICallbackLobRead2) 0,					/* callback... */ 
				(ub2) connection->charset,	/* The character set ID of the buffer data. */ 
				(ub1) SQLCS_IMPLICIT					/* The character set form of the buffer data. */
			)
		);

		bytes_total += bytes_read;
		if (is_clob) {
			offset += chars_read;
		} else {
			offset += bytes_read;
		}
		
		*data_len += bytes_read;
		
		if (connection->errcode != OCI_NEED_DATA) {
			break;
		}
		*data = erealloc(*data, *data_len + PHP_OCI_LOB_BUFFER_SIZE + 1);	
	} while (connection->errcode == OCI_NEED_DATA);

#else

	do {
		connection->errcode = PHP_OCI_CALL(OCILobRead, 
			(
				connection->svc, 
				connection->err, 
				descriptor->descriptor, 
				&bytes_read,								/* IN/OUT bytes toread/read */ 
				offset + 1,								/* offset (starts with 1) */ 
				(dvoid *) ((char *) *data + *data_len),	
				requested_len,									/* size of buffer */ 
				(dvoid *)0, 
				(OCICallbackLobRead) 0,					/* callback... */ 
				(ub2) connection->charset,	/* The character set ID of the buffer data. */ 
				(ub1) SQLCS_IMPLICIT					/* The character set form of the buffer data. */
			)
		);

		bytes_total += bytes_read;
		offset += bytes_read;
		
		*data_len += bytes_read;
		
		if (connection->errcode != OCI_NEED_DATA) {
			break;
		}
		*data = erealloc(*data, *data_len + PHP_OCI_LOB_BUFFER_SIZE + 1);	
	} while (connection->errcode == OCI_NEED_DATA);

#endif
	
	if (connection->errcode != OCI_SUCCESS) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		efree(*data);
		*data = NULL;
		return 1;
	}
	
	descriptor->lob_current_position = offset; 

	if (descriptor->type == OCI_DTYPE_FILE) {
		connection->errcode = PHP_OCI_CALL(OCILobFileClose, (connection->svc, connection->err, descriptor->descriptor));

		if (connection->errcode != OCI_SUCCESS) {
			php_oci_error(connection->err, connection->errcode TSRMLS_CC);
			PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
			efree(*data);
			*data = NULL;
			return 1;
		}
	}

	*data = erealloc(*data, *data_len + 1);
	(*data)[ *data_len ] = 0;

	return 0;
} /* }}} */

/* {{{ php_oci_lob_write() 
 Write data to the LOB */
int php_oci_lob_write (php_oci_descriptor *descriptor, ub4 offset, char *data, int data_len, ub4 *bytes_written TSRMLS_DC)
{
	OCILobLocator *lob         = (OCILobLocator *) descriptor->descriptor;
	php_oci_connection *connection = (php_oci_connection *) descriptor->connection;
	ub4 lob_length;
	
	*bytes_written = 0;
	if (php_oci_lob_get_length(descriptor, &lob_length TSRMLS_CC)) {
		return 1;
	}
	
	if (!data || data_len <= 0) {
		return 0;
	}
	
	if (offset < 0) {
		offset = 0;
	}
	
	if (offset > descriptor->lob_current_position) {
		offset = descriptor->lob_current_position;
	}
	
	connection->errcode = PHP_OCI_CALL(OCILobWrite, (connection->svc, connection->err, lob, (ub4 *)&data_len, (ub4) offset + 1, (dvoid *) data, (ub4) data_len, OCI_ONE_PIECE, (dvoid *)0, (OCICallbackLobWrite) 0, (ub2) 0, (ub1) SQLCS_IMPLICIT));

	if (connection->errcode) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		*bytes_written = 0;
		return 1;
	}
	*bytes_written = data_len;
	descriptor->lob_current_position += data_len;
	
	if (descriptor->lob_current_position > descriptor->lob_size) {
		descriptor->lob_size = descriptor->lob_current_position;
	}
	
	/* marking buffer as used */
	if (descriptor->buffering == PHP_OCI_LOB_BUFFER_ENABLED) {
		descriptor->buffering = PHP_OCI_LOB_BUFFER_USED;
	}
	
	return 0;
} /* }}} */

/* {{{ php_oci_lob_set_buffering() 
 Turn buffering off/onn for this particular LOB */
int php_oci_lob_set_buffering (php_oci_descriptor *descriptor, int on_off TSRMLS_DC)
{
	php_oci_connection *connection = descriptor->connection;

	if (!on_off && descriptor->buffering == PHP_OCI_LOB_BUFFER_DISABLED) {
		/* disabling when it's already off */
		return 0;
	}
	
	if (on_off && descriptor->buffering != PHP_OCI_LOB_BUFFER_DISABLED) {
		/* enabling when it's already on */
		return 0;
	}
	
	if (on_off) {
		connection->errcode = PHP_OCI_CALL(OCILobEnableBuffering, (connection->svc, connection->err, descriptor->descriptor));
	}
	else {
		connection->errcode = PHP_OCI_CALL(OCILobDisableBuffering, (connection->svc, connection->err, descriptor->descriptor));
	}

	if (connection->errcode != OCI_SUCCESS) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		return 1;
	}
	descriptor->buffering = on_off ? PHP_OCI_LOB_BUFFER_ENABLED : PHP_OCI_LOB_BUFFER_DISABLED;
	return 0;
} /* }}} */

/* {{{ php_oci_lob_get_buffering()
 Return current buffering state for the LOB */
int php_oci_lob_get_buffering (php_oci_descriptor *descriptor TSRMLS_DC)
{
	if (descriptor->buffering != PHP_OCI_LOB_BUFFER_DISABLED) {
		return 1;
	} 
	else {
		return 0;
	}
} /* }}} */

/* {{{ php_oci_lob_copy()
 Copy one LOB (or its part) to another one */
int php_oci_lob_copy (php_oci_descriptor *descriptor_dest, php_oci_descriptor *descriptor_from, long length TSRMLS_DC)
{
	php_oci_connection *connection = descriptor_dest->connection;
	int length_dest, length_from, copy_len;
	
	if (php_oci_lob_get_length(descriptor_dest, &length_dest TSRMLS_CC)) {
		return 1;
	}
	
	if (php_oci_lob_get_length(descriptor_from, &length_from TSRMLS_CC)) {
		return 1;
	}

	if (length == -1) {
		copy_len = length_from - descriptor_from->lob_current_position;
	}
	else {
		copy_len = length;
	}

	if ((int)copy_len <= 0) {
		/* silently fail, there is nothing to copy */
		return 1;
	}

	connection->errcode = PHP_OCI_CALL(OCILobCopy, (connection->svc, connection->err, descriptor_dest->descriptor, descriptor_from->descriptor, copy_len, descriptor_dest->lob_current_position+1, descriptor_from->lob_current_position+1));

	if (connection->errcode != OCI_SUCCESS) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		return 1;
	}
	
	return 0;
} /* }}} */

/* {{{ php_oci_lob_close() 
 Close LOB */
int php_oci_lob_close (php_oci_descriptor *descriptor TSRMLS_DC)
{
	php_oci_connection *connection = descriptor->connection;
	int is_temporary;
	
	connection->errcode = PHP_OCI_CALL(OCILobClose, (connection->svc, connection->err, descriptor->descriptor));

	if (connection->errcode != OCI_SUCCESS) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		return 1;
	}
	
	connection->errcode = PHP_OCI_CALL(OCILobIsTemporary, (connection->env,connection->err, descriptor->descriptor, &is_temporary));
	
	if (connection->errcode != OCI_SUCCESS) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		return 1;
	}
	
	if (is_temporary) {
		
		connection->errcode = PHP_OCI_CALL(OCILobFreeTemporary, (connection->svc, connection->err, descriptor->descriptor));
		
		if (connection->errcode != OCI_SUCCESS) {
			php_oci_error(connection->err, connection->errcode TSRMLS_CC);
			PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
			return 1;
		}
	}
	return 0;
} /* }}} */

/* {{{ php_oci_lob_flush() 
 Flush buffers for the LOB (only if they have been used) */
int php_oci_lob_flush(php_oci_descriptor *descriptor, int flush_flag TSRMLS_DC)
{
	OCILobLocator *lob = descriptor->descriptor;
	php_oci_connection *connection = descriptor->connection;
	
	if (!lob) {
		return 1;
	}

	switch (flush_flag) {
		case 0:
		case OCI_LOB_BUFFER_FREE:
			/* only these two are allowed */
			break;
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid flag value: %d", flush_flag);
			return 1;
			break;
	}
	
	/* do not really flush buffer, but report success
	 * to suppress OCI error when flushing not used buffer
	 * */
	if (descriptor->buffering != PHP_OCI_LOB_BUFFER_USED) {
		return 0;
	}

	connection->errcode = PHP_OCI_CALL(OCILobFlushBuffer, (connection->svc, connection->err, lob, flush_flag));

	if (connection->errcode != OCI_SUCCESS) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		return 1;
	}

	/* marking buffer as enabled and not used */
	descriptor->buffering = PHP_OCI_LOB_BUFFER_ENABLED;
	return 0;
} /* }}} */

/* {{{ php_oci_lob_free()
 Close LOB descriptor and free associated resources */
void php_oci_lob_free (php_oci_descriptor *descriptor TSRMLS_DC)
{

	if (!descriptor || !descriptor->connection) {
		return;
	}

	if (descriptor->connection->descriptors) {
		/* delete descriptor from the hash */
		zend_hash_apply_with_argument(descriptor->connection->descriptors, php_oci_descriptor_delete_from_hash, (void *)&descriptor->id TSRMLS_CC);
	}
	
	/* flushing Lobs & Files with buffering enabled */
	if ((descriptor->type == OCI_DTYPE_FILE || descriptor->type == OCI_DTYPE_LOB) && descriptor->buffering == PHP_OCI_LOB_BUFFER_USED) {
		php_oci_lob_flush(descriptor, OCI_LOB_BUFFER_FREE TSRMLS_CC);
	}

	PHP_OCI_CALL(OCIDescriptorFree, (descriptor->descriptor, descriptor->type));

	zend_list_delete(descriptor->connection->rsrc_id);
	efree(descriptor);
} /* }}} */

/* {{{ php_oci_lob_import()
 Import LOB contents from the given file */
int php_oci_lob_import (php_oci_descriptor *descriptor, char *filename TSRMLS_DC)
{
	int fp, loblen;
	OCILobLocator *lob = (OCILobLocator *)descriptor->descriptor;
	php_oci_connection *connection = descriptor->connection;
	char buf[8192];
	ub4 offset = 1;
	
	if (php_check_open_basedir(filename TSRMLS_CC)) {
		return 1;
	}
	
	if ((fp = VCWD_OPEN(filename, O_RDONLY|O_BINARY)) == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Can't open file %s", filename);
		return 1;
	}

	while ((loblen = read(fp, &buf, sizeof(buf))) > 0) {	
		connection->errcode = PHP_OCI_CALL(
				OCILobWrite,
				(
					connection->svc, 
					connection->err, 
					lob, 
					&loblen, 
					(ub4) offset, 
					(dvoid *) &buf, 
					(ub4) loblen, 
					OCI_ONE_PIECE, 
					(dvoid *)0, 
					(OCICallbackLobWrite) 0, 
					(ub2) 0, 
					(ub1) SQLCS_IMPLICIT
				)
		);

		if (connection->errcode != OCI_SUCCESS) {
			php_oci_error(connection->err, connection->errcode TSRMLS_CC);
			PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
			close(fp);
			return 1;
		}
		offset += loblen;
	}
	close(fp);
	
	return 0;
} /* }}} */

/* {{{ php_oci_lob_append()
 Append data to the end of the LOB */
int php_oci_lob_append (php_oci_descriptor *descriptor_dest, php_oci_descriptor *descriptor_from TSRMLS_DC)
{
	php_oci_connection *connection = descriptor_dest->connection;
	OCILobLocator *lob_dest = descriptor_dest->descriptor;
	OCILobLocator *lob_from = descriptor_from->descriptor;
	ub4 dest_len, from_len;

	if (php_oci_lob_get_length(descriptor_dest, &dest_len TSRMLS_CC)) {
		return 1;
	}
	
	if (php_oci_lob_get_length(descriptor_from, &from_len TSRMLS_CC)) {
		return 1;
	}

	if (from_len <= 0) {
		return 0;
	}

	connection->errcode = PHP_OCI_CALL(OCILobAppend, (connection->svc, connection->err, lob_dest, lob_from));

	if (connection->errcode != OCI_SUCCESS) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		return 1;
	}
	return 0;
} /* }}} */

/* {{{ php_oci_lob_truncate()
 Truncate LOB to the given length */
int php_oci_lob_truncate (php_oci_descriptor *descriptor, long new_lob_length TSRMLS_DC)
{
	php_oci_connection *connection = descriptor->connection;
	OCILobLocator *lob = descriptor->descriptor;
	ub4 lob_length;
	
	if (php_oci_lob_get_length(descriptor, &lob_length TSRMLS_CC)) {
		return 1;
	}
	
	if (lob_length <= 0) {
		return 0;
	}

	if (new_lob_length < 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Size must be greater than or equal to 0");
		return 1;
	}

	if (new_lob_length > lob_length) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Size must be less than or equal to the current LOB size");
		return 1;
	}
	
	connection->errcode = PHP_OCI_CALL(OCILobTrim, (connection->svc, connection->err, lob, new_lob_length));

	if (connection->errcode != OCI_SUCCESS) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		return 1;
	}
	
	descriptor->lob_size = new_lob_length;
	return 0;
} /* }}} */

/* {{{ php_oci_lob_erase()
 Erase (or fill with whitespaces, depending on LOB type) the LOB (or its part) */
int php_oci_lob_erase (php_oci_descriptor *descriptor, long offset, long length, ub4 *bytes_erased TSRMLS_DC)
{
	php_oci_connection *connection = descriptor->connection;
	OCILobLocator *lob = descriptor->descriptor;
	ub4 lob_length;

	*bytes_erased = 0;
	
	if (php_oci_lob_get_length(descriptor, &lob_length TSRMLS_CC)) {
		return 1;
	}
	
	if (offset == -1) {
		offset = descriptor->lob_current_position;
	}

	if (length == -1) {
		length = lob_length;
	}
	
	connection->errcode = PHP_OCI_CALL(OCILobErase, (connection->svc, connection->err, lob, (ub4 *)&length, offset+1));

	if (connection->errcode != OCI_SUCCESS) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		return 1;
	}
	
	*bytes_erased = length;
	return 0;
} /* }}} */

/* {{{ php_oci_lob_is_equal() 
 Compare two LOB descriptors and figure out if they are pointing to the same LOB */
int php_oci_lob_is_equal (php_oci_descriptor *descriptor_first, php_oci_descriptor *descriptor_second, boolean *result TSRMLS_DC)
{
	php_oci_connection *connection = descriptor_first->connection;
	OCILobLocator *first_lob   = descriptor_first->descriptor;
	OCILobLocator *second_lob  = descriptor_second->descriptor;

	connection->errcode = PHP_OCI_CALL(OCILobIsEqual, (connection->env, first_lob, second_lob, result));

	if (connection->errcode) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		return 1;
	}
	return 0;
} /* }}} */

/* {{{ php_oci_lob_write_tmp() 
 Create temporary LOB and write data to it */
int php_oci_lob_write_tmp (php_oci_descriptor *descriptor, ub1 type, char *data, int data_len TSRMLS_DC)
{
	php_oci_connection *connection = descriptor->connection;
	OCILobLocator *lob         = descriptor->descriptor;
	ub4 bytes_written = 0;
	
	switch (type) {
		case OCI_TEMP_BLOB:
		case OCI_TEMP_CLOB:
			/* only these two are allowed */
			break;
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid temporary lob type: %d", type);
			return 1;
			break;
	}

	if (!data || data_len <= 0) {
		/* nothing to write, silently fail */
		return 1;
	}

	connection->errcode = PHP_OCI_CALL(OCILobCreateTemporary, (connection->svc, connection->err, lob, OCI_DEFAULT, OCI_DEFAULT, type, OCI_ATTR_NOCACHE, OCI_DURATION_SESSION));

	if (connection->errcode) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		return 1;
	}

	connection->errcode = PHP_OCI_CALL(OCILobOpen, (connection->svc, connection->err, lob, OCI_LOB_READWRITE));

	if (connection->errcode) {
		php_oci_error(connection->err, connection->errcode TSRMLS_CC);
		PHP_OCI_HANDLE_ERROR(connection, connection->errcode);
		return 1;
	}

	return php_oci_lob_write(descriptor, 0, data, data_len, &bytes_written TSRMLS_CC);
} /* }}} */

#endif /* HAVE_OCI8 */
