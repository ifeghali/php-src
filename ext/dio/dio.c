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
   | Author: Sterling Hughes <sterling@php.net>                           |
   +----------------------------------------------------------------------+
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_dio.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define le_fd_name "Direct I/O File Descriptor"
static int le_fd;

function_entry dio_functions[] = {
    PHP_FE(dio_open,      NULL)
    PHP_FE(dio_truncate,  NULL)
    PHP_FE(dio_stat,      NULL)
	PHP_FE(dio_seek,      NULL)
	PHP_FE(dio_fcntl,     NULL)
	PHP_FE(dio_read,      NULL)
    PHP_FE(dio_write,     NULL)
    PHP_FE(dio_close,     NULL)
	{NULL, NULL, NULL}
};


zend_module_entry dio_module_entry = {
	STANDARD_MODULE_HEADER,
	"dio",
	dio_functions,
	PHP_MINIT(dio),
	NULL,
	NULL,	
	NULL,
	PHP_MINFO(dio),
    "0.1",
	STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_DIO
ZEND_GET_MODULE(dio)
#endif


static void _dio_close_fd(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	php_fd_t *f = (php_fd_t *) rsrc->ptr;
	close(f->fd);
	free(f);
}

#define RDIOC(c) REGISTER_LONG_CONSTANT(#c, c, CONST_CS | CONST_PERSISTENT)

PHP_MINIT_FUNCTION(dio)
{
    le_fd = zend_register_list_destructors_ex(_dio_close_fd, NULL, le_fd_name, module_number);

	RDIOC(O_RDONLY);
	RDIOC(O_WRONLY);
	RDIOC(O_RDWR);
	RDIOC(O_CREAT);
	RDIOC(O_EXCL);
	RDIOC(O_TRUNC);
	RDIOC(O_APPEND);
	RDIOC(O_NONBLOCK);
	RDIOC(O_NDELAY);
	RDIOC(O_SYNC);
	RDIOC(O_NOCTTY);
	RDIOC(S_IRWXU);
	RDIOC(S_IRUSR);
	RDIOC(S_IWUSR);
	RDIOC(S_IXUSR);
	RDIOC(S_IRWXG);
	RDIOC(S_IRGRP);
	RDIOC(S_IWGRP);
	RDIOC(S_IXGRP);
	RDIOC(S_IRWXO);
	RDIOC(S_IROTH);
	RDIOC(S_IWOTH);
	RDIOC(S_IXOTH);
	RDIOC(F_DUPFD);
	RDIOC(F_GETFD);
	RDIOC(F_GETFL);
	RDIOC(F_SETFL);
	RDIOC(F_GETLK);
	RDIOC(F_SETLK);
	RDIOC(F_SETLKW);
	RDIOC(F_SETOWN);
	RDIOC(F_GETOWN);
	RDIOC(F_UNLCK);
	RDIOC(F_RDLCK);
	RDIOC(F_WRLCK);
	
	return SUCCESS;
}

PHP_MINFO_FUNCTION(dio)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "dio support", "enabled");
	php_info_print_table_end();
}

static void new_php_fd(php_fd_t **f, int fd)
{
	*f = malloc(sizeof(php_fd_t));
	(*f)->fd = fd;
}

/* {{{ proto resource dio_open(string filename, int flags[, int mode])
   Open a new filename with specified permissions of flags and creation permissions of mode */
PHP_FUNCTION(dio_open)
{
	php_fd_t *f;
	char     *file_name;
	int       file_name_length;
	int       flags;
	mode_t    mode = 0;
	int       fd;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl|l", &file_name, &file_name_length, &flags, &mode) == FAILURE) {
		return;
	}

	if (ZEND_NUM_ARGS() == 3) {
		fd = open(file_name, flags, mode);
	}
	else {
		fd = open(file_name, flags);
	}

	if (fd == -1) {
		php_error(E_WARNING, "%s(): cannot open file %s with flags %d and permissions %d: %s", 
				  get_active_function_name(TSRMLS_C), file_name, flags, mode, strerror(errno));
		RETURN_FALSE;
	}

	new_php_fd(&f, fd);
	ZEND_REGISTER_RESOURCE(return_value, f, le_fd);
}
/* }}} */

/* {{{ proto string dio_read(resource fd[, int n])
   Read n bytes from fd and return them, if n is not specified, read 1k */
PHP_FUNCTION(dio_read)
{
	zval     *r_fd;
	php_fd_t *f;
	char     *data;
	int       bytes = 1024;
	ssize_t   res;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|l", &r_fd, &bytes) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(f, php_fd_t *, &r_fd, -1, le_fd_name, le_fd);

	data = emalloc(bytes + 1);
	res = read(f->fd, data, bytes);
	if (res <= 0) {
		RETURN_NULL();
	}
	
	data[res] = 0;
	RETURN_STRINGL(data, res, 0);
}
/* }}} */

/* {{{ proto int dio_write(resource fd, string data[, int len])
   Write data to fd with optional truncation at length */
PHP_FUNCTION(dio_write)
{
	zval     *r_fd;
	php_fd_t *f;
	char     *data;
	size_t    data_len;
	size_t    trunc_len = 0;
	ssize_t   res;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &r_fd, &data, &data_len, &trunc_len) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(f, php_fd_t *, &r_fd, -1, le_fd_name, le_fd);

	res = write(f->fd, data, trunc_len ? trunc_len : data_len);
	if (res == -1) {
		php_error(E_WARNING, "%s(): cannot write data to file descriptor %d, %s",
				  get_active_function_name(TSRMLS_C), f->fd, strerror(errno));
	}

	RETURN_LONG(res);
}
/* }}} */

/* {{{ proto bool dio_truncate(resource fd, int offset)
   Truncate file descriptor fd to offset bytes */
PHP_FUNCTION(dio_truncate)
{
	zval     *r_fd;
	php_fd_t *f;
	off_t     offset;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &r_fd, &offset) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(f, php_fd_t *, &r_fd, -1, le_fd_name, le_fd);

	if (ftruncate(f->fd, offset) == -1) {
		php_error(E_WARNING, "%s(): couldn't truncate %d to %d bytes: %s",
				  get_active_function_name(TSRMLS_C), f->fd, offset, strerror(errno));
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

#define ADD_FIELD(f, v) add_assoc_long_ex(return_value, (f), sizeof(f), v);

/* {{{ proto array dio_stat(resource fd)
   Get stat information about the file descriptor fd */
PHP_FUNCTION(dio_stat)
{
	zval        *r_fd;
	php_fd_t    *f;
	struct stat  s;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &r_fd) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(f, php_fd_t *, &r_fd, -1, le_fd_name, le_fd);

	if (fstat(f->fd, &s) == -1) {
		php_error(E_WARNING, "%s(): cannot stat %d: %s",
				  get_active_function_name(TSRMLS_C), f->fd, strerror(errno));
		RETURN_FALSE;
	}

	array_init(return_value);
	ADD_FIELD("device", s.st_dev);
	ADD_FIELD("inode", s.st_ino);
	ADD_FIELD("mode", s.st_mode);
	ADD_FIELD("nlink", s.st_nlink);
	ADD_FIELD("uid", s.st_uid);
	ADD_FIELD("gid", s.st_gid);
	ADD_FIELD("device_type", s.st_rdev);
	ADD_FIELD("size", s.st_size);
	ADD_FIELD("block_size", s.st_blksize);
	ADD_FIELD("blocks", s.st_blocks);
	ADD_FIELD("atime", s.st_atime);
	ADD_FIELD("mtime", s.st_mtime);
	ADD_FIELD("ctime", s.st_ctime);
}
/* }}} */

/* {{{ proto int dio_seek(resource fd, int pos, int whence)
   Seek to pos on fd from whence */
PHP_FUNCTION(dio_seek)
{
	zval     *r_fd;
	php_fd_t *f;
	off_t     offset;
	int       whence = SEEK_SET;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl|l", &r_fd, &offset, &whence) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(f, php_fd_t *, &r_fd, -1, le_fd_name, le_fd);

	RETURN_LONG(lseek(f->fd, offset, whence));
}
/* }}} */

/* {{{ proto mixed dio_fcntl(resource fd, int cmd[, mixed arg])
   Perform a c library fcntl on fd */
PHP_FUNCTION(dio_fcntl)
{
	zval     *r_fd;
	zval     *arg = NULL;
	php_fd_t *f;
	int       cmd;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl|z", &r_fd, &cmd, &arg) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(f, php_fd_t *, &r_fd, -1, le_fd_name, le_fd);

	switch (cmd) {
	case F_SETLK:
	case F_SETLKW: {
		zval          **element;
		struct flock    lk = {0};
		HashTable      *fh;

		if (!arg) {
			php_error(E_WARNING, "%s() expects argument 3 to be array or int, none given",
					  get_active_function_name(TSRMLS_C));
			RETURN_FALSE;
		}
		if (Z_TYPE_P(arg) == IS_ARRAY) {
			fh = HASH_OF(arg);
			if (zend_hash_find(fh, "start", 5, (void **) &element) == FAILURE) {
				lk.l_start = 0;
			}
			else {
				lk.l_start = Z_LVAL_PP(element);
			}

			if (zend_hash_find(fh, "length", 6, (void **) &element) == FAILURE) {
				lk.l_len = 0;
			}
			else {
				lk.l_len = Z_LVAL_PP(element);
			}

			if (zend_hash_find(fh, "whence", 6, (void **) &element) == FAILURE) {
				lk.l_whence = 0;
			}
			else {
				lk.l_whence = SEEK_SET;
			}

			if (zend_hash_find(fh, "type", 6, (void **) &element) == FAILURE) {
				lk.l_type = 0;
			}
			else {
				lk.l_type = Z_LVAL_PP(element);
			}
		} else if (Z_TYPE_P(arg) == IS_LONG) {
			lk.l_start  = 0;
			lk.l_len    = 0;
			lk.l_whence = SEEK_SET;
			lk.l_type   = Z_LVAL_P(arg);
		} else {
			php_error(E_WARNING, "%s() expects argument 3 to be array or int, %s given",
					  get_active_function_name(TSRMLS_C), zend_zval_type_name(arg));
			RETURN_FALSE;
		}

		RETURN_LONG(fcntl(f->fd, cmd, &lk));
		break;
	}
	case F_GETLK: {
		struct flock lk = {0};
		
		fcntl(f->fd, cmd, &lk);

		array_init(return_value);
		add_assoc_long(return_value, "type", lk.l_type);
		add_assoc_long(return_value, "whence", lk.l_whence);
		add_assoc_long(return_value, "start", lk.l_start);
		add_assoc_long(return_value, "length", lk.l_len);
		add_assoc_long(return_value, "pid", lk.l_pid);

		break;
	}
	case F_DUPFD: {
		php_fd_t *new_f;

		if (!arg || Z_TYPE_P(arg) != IS_LONG) {
			php_error(E_WARNING, "%s() expects argument 3 to be int",
					  get_active_function_name(TSRMLS_C));
			RETURN_FALSE;
		}

		new_php_fd(&new_f, fcntl(f->fd, cmd, Z_LVAL_P(arg)));
		ZEND_REGISTER_RESOURCE(return_value, new_f, le_fd);
		break;
	}
	default:
		if (!arg || Z_TYPE_P(arg) != IS_LONG) {
			php_error(E_WARNING, "%s() expects argument 3 to be int",
					  get_active_function_name(TSRMLS_C));
			RETURN_FALSE;
		}

		RETURN_LONG(fcntl(f->fd, cmd, Z_LVAL_P(arg)));
	}
}
/* }}} */

/* {{{ proto void dio_close(resource fd)
   Close the file descriptor given by fd */
PHP_FUNCTION(dio_close)
{
	zval     *r_fd;
	php_fd_t *f;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &r_fd) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(f, php_fd_t *, &r_fd, -1, le_fd_name, le_fd);

	zend_list_delete(Z_LVAL_P(r_fd));
}
/* }}} */

/*
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: sw=4 ts=4 noet
 */
