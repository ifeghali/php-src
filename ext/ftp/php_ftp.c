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
   | Authors: Andrew Skalski <askalski@chek.com>                          |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#if HAVE_FTP

#include "ext/standard/info.h"
#include "ext/standard/file.h"

#include "php_ftp.h"
#include "ftp.h"

static int	le_ftpbuf;
#define le_ftpbuf_name "FTP Buffer"

function_entry php_ftp_functions[] = {
	PHP_FE(ftp_connect,			NULL)
	PHP_FE(ftp_login,			NULL)
	PHP_FE(ftp_pwd,				NULL)
	PHP_FE(ftp_cdup,			NULL)
	PHP_FE(ftp_chdir,			NULL)
	PHP_FE(ftp_exec,			NULL)
	PHP_FE(ftp_mkdir,			NULL)
	PHP_FE(ftp_rmdir,			NULL)
	PHP_FE(ftp_nlist,			NULL)
	PHP_FE(ftp_rawlist,			NULL)
	PHP_FE(ftp_systype,			NULL)
	PHP_FE(ftp_pasv,			NULL)
	PHP_FE(ftp_get,				NULL)
	PHP_FE(ftp_fget,			NULL)
	PHP_FE(ftp_put,				NULL)
	PHP_FE(ftp_fput,			NULL)
	PHP_FE(ftp_size,			NULL)
	PHP_FE(ftp_mdtm,			NULL)
	PHP_FE(ftp_rename,			NULL)
	PHP_FE(ftp_delete,			NULL)
	PHP_FE(ftp_site,			NULL)
	PHP_FE(ftp_close,			NULL)
	PHP_FE(ftp_set_option,		NULL)
	PHP_FE(ftp_get_option,		NULL)
	PHP_FALIAS(ftp_quit, ftp_close, NULL)
	{NULL, NULL, NULL}
};

zend_module_entry php_ftp_module_entry = {
    STANDARD_MODULE_HEADER,
	"ftp",
	php_ftp_functions,
	PHP_MINIT(ftp),
	NULL,
	NULL,
	NULL,
	PHP_MINFO(ftp),
    NO_VERSION_YET,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_FTP
ZEND_GET_MODULE(php_ftp)
#endif

static void ftp_destructor_ftpbuf(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	ftpbuf_t *ftp = (ftpbuf_t *)rsrc->ptr;

	ftp_close(ftp);
}

PHP_MINIT_FUNCTION(ftp)
{
	le_ftpbuf = zend_register_list_destructors_ex(ftp_destructor_ftpbuf, NULL, le_ftpbuf_name, module_number);
	REGISTER_LONG_CONSTANT("FTP_ASCII",	 FTPTYPE_ASCII, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("FTP_TEXT",   FTPTYPE_ASCII, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("FTP_BINARY", FTPTYPE_IMAGE, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("FTP_IMAGE",  FTPTYPE_IMAGE, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("FTP_TIMEOUT_SEC", PHP_FTP_OPT_TIMEOUT_SEC, CONST_PERSISTENT | CONST_CS);
	return SUCCESS;
}

PHP_MINFO_FUNCTION(ftp)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "FTP support", "enabled");
	php_info_print_table_end();
}

#define	XTYPE(xtype, mode)	{ \
								if (mode != FTPTYPE_ASCII && mode != FTPTYPE_IMAGE) { \
									php_error(E_WARNING, "%s(): Mode must be FTP_ASCII or FTP_BINARY", get_active_function_name(TSRMLS_C)); \
									RETURN_FALSE; \
								} \
								xtype = mode; \
							}


/* {{{ proto resource ftp_connect(string host [, int port [, int timeout)]])
   Opens a FTP stream */
PHP_FUNCTION(ftp_connect)
{
	ftpbuf_t	*ftp;
	char		*host;
	int			host_len, port = 0;
	long		timeout_sec = FTP_DEFAULT_TIMEOUT;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|ll", &host, &host_len, &port, &timeout_sec) == FAILURE) {
		return;
	}

	if (timeout_sec <= 0) {
		php_error(E_WARNING, "%s(): Timeout has to be greater than 0",
				  get_active_function_name(TSRMLS_C));
		RETURN_FALSE;
	}

	/* connect */
	ftp = ftp_open(host, (short)port, timeout_sec);
	if (ftp == NULL) {
		RETURN_FALSE;
	}

	ZEND_REGISTER_RESOURCE(return_value, ftp, le_ftpbuf);
}
/* }}} */

/* {{{ proto bool ftp_login(resource stream, string username, string password)
   Logs into the FTP server */
PHP_FUNCTION(ftp_login)
{
	zval 		*z_ftp;
	ftpbuf_t	*ftp;
	char *user, *pass;
	int user_len, pass_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_ftp, &user, &user_len, &pass, &pass_len) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	/* log in */
	if (!ftp_login(ftp, user, pass)) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto string ftp_pwd(resource stream)
   Returns the present working directory */
PHP_FUNCTION(ftp_pwd)
{
	zval 		*z_ftp;
	ftpbuf_t	*ftp;
	const char	*pwd;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_ftp) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	pwd = ftp_pwd(ftp);
	if (pwd == NULL) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	RETURN_STRING((char*) pwd, 1);
}
/* }}} */

/* {{{ proto bool ftp_cdup(resource stream)
   Changes to the parent directory */
PHP_FUNCTION(ftp_cdup)
{
	zval 		*z_ftp;
	ftpbuf_t	*ftp;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_ftp) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	if (!ftp_cdup(ftp)) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ftp_chdir(resource stream, string directory)
   Changes directories */
PHP_FUNCTION(ftp_chdir)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	char		*dir;
	int			dir_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_ftp, &dir, &dir_len) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	/* change directories */
	if (!ftp_chdir(ftp, dir)) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ftp_exec(resource stream, string command)
   Requests execution of a program on the FTP server */
PHP_FUNCTION(ftp_exec)
{
	pval		*z_ftp;
	ftpbuf_t	*ftp;
	char		*cmd;
	int			cmd_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_ftp, &cmd, &cmd_len) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	/* execute serverside command */
	if (!ftp_exec(ftp, cmd)) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto string ftp_mkdir(resource stream, string directory)
   Creates a directory and returns the absolute path for the new directory or false on error */
PHP_FUNCTION(ftp_mkdir)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	char		*dir, *tmp;
	int			dir_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_ftp, &dir, &dir_len) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	/* create directorie */
	if (NULL == (tmp = ftp_mkdir(ftp, dir))) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	RETURN_STRING(tmp, 1);
}
/* }}} */

/* {{{ proto bool ftp_rmdir(resource stream, string directory)
   Removes a directory */
PHP_FUNCTION(ftp_rmdir)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	char		*dir;
	int			dir_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_ftp, &dir, &dir_len) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	/* remove directorie */
	if (!ftp_rmdir(ftp, dir)) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto array ftp_nlist(resource stream, string directory)
   Returns an array of filenames in the given directory */
PHP_FUNCTION(ftp_nlist)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	char		**nlist, **ptr, *dir;
	int			dir_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_ftp, &dir, &dir_len) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	/* get list of files */
	if (NULL == (nlist = ftp_nlist(ftp, dir))) {
		RETURN_FALSE;
	}

	array_init(return_value);
	for (ptr = nlist; *ptr; ptr++)
		add_next_index_string(return_value, *ptr, 1);
	free(nlist);
}
/* }}} */

/* {{{ proto array ftp_rawlist(resource stream, string directory [, bool recursive])
   Returns a detailed listing of a directory as an array of output lines */
PHP_FUNCTION(ftp_rawlist)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	char		**llist, **ptr, *dir;
	int			dir_len;
	zend_bool	recursive;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|b", &z_ftp, &dir, &dir_len, &recursive) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	/* get raw directory listing */
	if (NULL == (llist = ftp_list(ftp, dir, recursive))) {
		RETURN_FALSE;
	}

	array_init(return_value);
	for (ptr = llist; *ptr; ptr++)
		add_next_index_string(return_value, *ptr, 1);
	free(llist);
}
/* }}} */

/* {{{ proto string ftp_systype(resource stream)
   Returns the system type identifier */
PHP_FUNCTION(ftp_systype)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	const char	*syst;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_ftp) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	syst = ftp_syst(ftp);
	if (NULL == (syst = ftp_syst(ftp))) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	RETURN_STRING((char*) syst, 1);
}
/* }}} */

/* {{{ proto bool ftp_fget(resource stream, resource fp, string remote_file, int mode)
   Retrieves a file from the FTP server and writes it to an open file */
PHP_FUNCTION(ftp_fget)
{
	zval		*z_ftp, *z_file;
	ftpbuf_t	*ftp;
	ftptype_t	xtype;
	php_stream	*stream;
	char		*file;
	int			file_len, mode;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rrsl", &z_ftp, &z_file, &file, &file_len, &mode) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);
	ZEND_FETCH_RESOURCE(stream, php_stream*, &z_file, -1, "File-Handle", php_file_le_stream());
	XTYPE(xtype, mode);

	if (!ftp_get(ftp, stream, file, xtype) || php_stream_error(stream)) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	if (php_stream_error(stream)) {
		php_error(E_WARNING, "%s(): Error writing %s", get_active_function_name(TSRMLS_C), Z_STRVAL_P(z_file));
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ftp_pasv(resource stream, bool pasv)
   Turns passive mode on or off */
PHP_FUNCTION(ftp_pasv)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	zend_bool	pasv;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rb", &z_ftp, &pasv) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	if (!ftp_pasv(ftp, pasv ? 1 : 0)) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ftp_get(resource stream, string local_file, string remote_file, int mode)
   Retrieves a file from the FTP server and writes it to a local file */
PHP_FUNCTION(ftp_get)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	ftptype_t	xtype;
	php_stream * tmpstream, *outstream;
	char		*local, *remote;
	int			local_len, remote_len, mode;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rssl", &z_ftp, &local, &local_len, &remote, &remote_len, &mode) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);
	XTYPE(xtype, mode);

	/* get to temporary file, so if there is an error, no existing
	 * file gets clobbered
	 */
	tmpstream = php_stream_fopen_tmpfile();
	if (tmpstream == NULL) {
		RETURN_FALSE;
	}

	if (!ftp_get(ftp, tmpstream, remote, xtype) || php_stream_error(tmpstream)) {
		php_stream_close(tmpstream);
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	outstream = php_stream_fopen(local, "wb", NULL);

	if (outstream == NULL)	{
		php_stream_close(tmpstream);
		php_error(E_WARNING, "%s(): Error opening %s", get_active_function_name(TSRMLS_C), local);
		RETURN_FALSE;
	}

	php_stream_rewind(tmpstream);
	if (php_stream_copy_to_stream(tmpstream, outstream, PHP_STREAM_COPY_ALL) == 0)	{	
		php_error(E_WARNING, "%s(): Error writing %s", get_active_function_name(TSRMLS_C), local);
		RETVAL_FALSE;
	}
	else
		RETVAL_TRUE;

	php_stream_close(tmpstream);
	php_stream_close(outstream);
}
/* }}} */

/* {{{ proto bool ftp_fput(resource stream, string remote_file, resource fp, int mode)
   Stores a file from an open file to the FTP server */
PHP_FUNCTION(ftp_fput)
{
	zval		*z_ftp, *z_file;
	ftpbuf_t	*ftp;
	ftptype_t	xtype;
	int			mode, remote_len;
	php_stream	*stream;
	char		*remote;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsrl", &z_ftp, &remote, &remote_len, &z_file, &mode) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);
   	ZEND_FETCH_RESOURCE(stream, php_stream*, &z_file, -1, "File-Handle", php_file_le_stream());
	XTYPE(xtype, mode);

	if (!ftp_put(ftp, remote, stream, xtype)) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ftp_put(resource stream, string remote_file, string local_file, int mode)
   Stores a file on the FTP server */
PHP_FUNCTION(ftp_put)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	ftptype_t	xtype;
	char		*remote, *local;
	int			remote_len, local_len, mode;
	php_stream * instream;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rssl", &z_ftp, &remote, &remote_len, &local, &local_len, &mode) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);
	XTYPE(xtype, mode);

	instream = php_stream_fopen(local, "rb", NULL);

	if (instream == NULL)	{
		RETURN_FALSE;
	}

	if (!ftp_put(ftp, remote, instream, xtype) || php_stream_error(instream)) {
		php_stream_close(instream);
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}
	php_stream_close(instream);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int ftp_size(resource stream, string filename)
   Returns the size of the file, or -1 on error */
PHP_FUNCTION(ftp_size)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	char		*file;
	int			file_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_ftp, &file, &file_len) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	/* get file size */
	RETURN_LONG(ftp_size(ftp, file));
}
/* }}} */

/* {{{ proto int ftp_mdtm(resource stream, string filename)
   Returns the last modification time of the file, or -1 on error */
PHP_FUNCTION(ftp_mdtm)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	char		*file;
	int			file_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_ftp, &file, &file_len) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	/* get file mod time */
	RETURN_LONG(ftp_mdtm(ftp, file));
}
/* }}} */

/* {{{ proto bool ftp_rename(resource stream, string src, string dest)
   Renames the given file to a new path */
PHP_FUNCTION(ftp_rename)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	char		*src, *dest;
	int			src_len, dest_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &z_ftp, &src, &src_len, &dest, &dest_len) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	/* rename the file */
	if (!ftp_rename(ftp, src, dest)) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ftp_delete(resource stream, string file)
   Deletes a file */
PHP_FUNCTION(ftp_delete)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	char		*file;
	int			file_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_ftp, &file, &file_len) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	/* delete the file */
	if (!ftp_delete(ftp, file)) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool ftp_site(resource stream, string cmd)
   Sends a SITE command to the server */
PHP_FUNCTION(ftp_site)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;
	char		*cmd;
	int			cmd_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &z_ftp, &cmd, &cmd_len) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	/* send the site command */
	if (!ftp_site(ftp, cmd)) {
		php_error(E_WARNING, "%s(): %s", get_active_function_name(TSRMLS_C), ftp->inbuf);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void ftp_close(resource stream)
   Closes the FTP stream */
PHP_FUNCTION(ftp_close)
{
	zval		*z_ftp;
	ftpbuf_t	*ftp;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_ftp) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	zend_list_delete(Z_LVAL_P(z_ftp));
}
/* }}} */
	
/* {{{ proto bool ftp_set_option(resource stream, int option, mixed value)
   Sets an FTP option */
PHP_FUNCTION(ftp_set_option)
{
	zval	*z_ftp, *z_value;
	long	option;
	ftpbuf_t	*ftp;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rlz", &z_ftp, &option, &z_value) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	switch (option) {
		case PHP_FTP_OPT_TIMEOUT_SEC:
			if (Z_TYPE_P(z_value) != IS_LONG) {
				php_error(E_WARNING, "%s(): Option TIMEOUT_SEC expects value of type long, %s given",
						  get_active_function_name(TSRMLS_C), zend_zval_type_name(z_value));
				RETURN_FALSE;
			}
			if (Z_LVAL_P(z_value) <= 0) {
				php_error(E_WARNING, "%s(): Timeout has to be greater than 0",
						  get_active_function_name(TSRMLS_C));
				RETURN_FALSE;
			}
			ftp->timeout_sec = Z_LVAL_P(z_value);
			RETURN_TRUE;
			break;
		default:
			php_error(E_WARNING, "%s(): Unknown option '%d'", get_active_function_name(TSRMLS_C), option);
			RETURN_FALSE;
			break;
	}
}
/* }}} */

/* {{{ proto mixed ftp_get_option(resource stream, int option)
   Gets an FTP option */
PHP_FUNCTION(ftp_get_option)
{
	zval	*z_ftp;
	long	option;
	ftpbuf_t	*ftp;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &z_ftp, &option) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(ftp, ftpbuf_t*, &z_ftp, -1, le_ftpbuf_name, le_ftpbuf);

	switch (option) {
		case PHP_FTP_OPT_TIMEOUT_SEC:
			RETURN_LONG(ftp->timeout_sec);
			break;
		default:
			php_error(E_WARNING, "%s(): Unknown option '%d'", get_active_function_name(TSRMLS_C), option);
			RETURN_FALSE;
			break;
	}
}
/* }}} */

#endif /* HAVE_FTP */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
