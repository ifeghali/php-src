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
   | Authors: Slava Poliakov (slavapl@mailandnews.com)                    |
   |          Ilia Alshanetsky (iliaa@home.com)                           |
   +----------------------------------------------------------------------+
 */

#include "php.h"
#include "php_ini.h"
#include "php_shmop.h"
#include <sys/ipc.h>
#include <sys/shm.h>

#if HAVE_SHMOP

#ifdef ZTS
int shmop_globals_id;
#else
php_shmop_globals shmop_globals;
#endif

int shm_type;

/* Every user visible function must have an entry in shmop_functions[].
*/
function_entry shmop_functions[] = {
	PHP_FE(shm_open, NULL)
	PHP_FE(shm_read, NULL)
	PHP_FE(shm_close, NULL)
	PHP_FE(shm_size, NULL)
	PHP_FE(shm_write, NULL)
	PHP_FE(shm_delete, NULL)
	{NULL, NULL, NULL}	/* Must be the last line in shmop_functions[] */
};

zend_module_entry shmop_module_entry = {
	"shmop",
	shmop_functions,
	PHP_MINIT(shmop),
	PHP_MSHUTDOWN(shmop),
	NULL,
	NULL,
	PHP_MINFO(shmop),
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SHMOP
ZEND_GET_MODULE(shmop)
#endif

static void rsclean(struct php_shmop *shmop);

PHP_MINIT_FUNCTION(shmop)
{
	shm_type = register_list_destructors(rsclean, NULL);
	
	return SUCCESS;
}

static void rsclean(struct php_shmop *shmop)
{
	shmdt(shmop->addr);
	efree(shmop);
}

PHP_MSHUTDOWN_FUNCTION(shmop)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(shmop)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "shmop support", "enabled");
	php_info_print_table_end();
}

/* {{{ proto int shm_open (int key, int flags, int mode, int size)
   shm_open - gets and attaches a shared memory segment */
PHP_FUNCTION(shm_open)
{
	zval **key, **flags, **mode, **size;
	struct php_shmop *shmop;	
	struct shmid_ds shm;
	int rsid;
	int shmflg=0;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &key, &flags, &mode, &size) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(key);
	convert_to_string_ex(flags);
	convert_to_long_ex(mode);
	convert_to_long_ex(size);

	shmop = emalloc(sizeof(struct php_shmop));
	memset(shmop, 0, sizeof(struct php_shmop));

	shmop->key = (*key)->value.lval;
	shmop->shmflg |= (*mode)->value.lval;

	if (memchr((*flags)->value.str.val, 'a', (*flags)->value.str.len)) {
		shmflg = SHM_RDONLY;
		shmop->shmflg |= IPC_EXCL;
	} 
	else if (memchr((*flags)->value.str.val, 'c', (*flags)->value.str.len)) {
		shmop->shmflg |= IPC_CREAT;
		shmop->size = (*size)->value.lval;
	}
	else {
		php_error(E_WARNING, "shmopen: access mode invalid");
		efree(shmop);
		RETURN_FALSE;
	}

	shmop->shmid = shmget(shmop->key, shmop->size, shmop->shmflg);
	if (shmop->shmid == -1) {
		php_error(E_WARNING, "shmopen: can't get the block");
		efree(shmop);
		RETURN_FALSE;
	}

	if (shmctl(shmop->shmid, IPC_STAT, &shm)) {
		efree(shmop);
		php_error(E_WARNING, "shmopen: can't get information on the block");
		RETURN_FALSE;
	}	

	shmop->addr = shmat(shmop->shmid, 0, shmflg);
	if (shmop->addr == (char*) -1) {
		efree(shmop);
		php_error(E_WARNING, "shmopen: can't attach the memory block");
		RETURN_FALSE;
	}

	shmop->size = shm.shm_segsz;
	
	rsid = zend_list_insert(shmop, shm_type);
	RETURN_LONG(rsid);
}
/* }}} */


/* {{{ proto string shm_read (int shmid, int start, int count)
   shm_read - reads from a shm segment */
PHP_FUNCTION(shm_read)
{
	zval **shmid, **start, **count;
	struct php_shmop *shmop;
	int type;
	char *startaddr;
	int bytes;
	char *return_string;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &shmid, &start, &count) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(shmid);
	convert_to_long_ex(start);
	convert_to_long_ex(count);

	shmop = zend_list_find((*shmid)->value.lval, &type);	

	if (!shmop) {
		php_error(E_WARNING, "shmread: can't find this segment");
		RETURN_FALSE;
	}

	if ((*start)->value.lval < 0 || (*start)->value.lval > shmop->size) {
		php_error(E_WARNING, "shmread: start is out of range");
		RETURN_FALSE;
	}

	if (((*start)->value.lval+(*count)->value.lval) > shmop->size) {
		php_error(E_WARNING, "shmread: count is out of range");
		RETURN_FALSE;
	}

	if ((*count)->value.lval < 0 ){
		php_error(E_WARNING, "shmread: count is out of range");
		RETURN_FALSE;
	}

	startaddr = shmop->addr + (*start)->value.lval;
	bytes = (*count)->value.lval ? (*count)->value.lval : shmop->size-(*start)->value.lval;

	return_string = emalloc(bytes);
	memcpy(return_string, startaddr, bytes);

	RETURN_STRINGL(return_string, bytes, 0);
}
/* }}} */


/* {{{ proto void shm_close (int shmid)
   shm_close - closes a shared memory segment */
PHP_FUNCTION(shm_close)
{
	zval **shmid;
	struct php_shmop *shmop;
	int type;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &shmid) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	shmop = zend_list_find((*shmid)->value.lval, &type);

	if (!shmop) {
		php_error(E_WARNING, "shmclose: no such shmid");
		RETURN_FALSE;
	}
	zend_list_delete((*shmid)->value.lval);

	RETURN_LONG(0);
}
/* }}} */


/* {{{ proto int shm_size (int shmid)
   shm_size - returns the shm size */
PHP_FUNCTION(shm_size)
{
	zval **shmid;
	struct php_shmop *shmop;
	int type;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &shmid) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(shmid);

	shmop = zend_list_find((*shmid)->value.lval, &type);

	if (!shmop) {
		php_error(E_WARNING, "shmsize: no such segment");
		RETURN_FALSE;
	}

	RETURN_LONG(shmop->size);
}
/* }}} */


/* {{{ proto int shm_write (int shmid, string data, int offset)
   shm_write - writes to a shared memory segment */
PHP_FUNCTION(shm_write)
{
	zval **shmid, **data, **offset;
	struct php_shmop *shmop;
	int type;
	int writesize;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &shmid, &data, &offset) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(shmid);
	convert_to_string_ex(data);
	convert_to_long_ex(offset);

	shmop = zend_list_find((*shmid)->value.lval, &type);

	if (!shmop) {
		php_error(E_WARNING, "shmwrite: error no such segment");
		RETURN_FALSE;
	}

	if ( (*offset)->value.lval > shmop->size ) {
		php_error(E_WARNING, "shmwrite: offset out of range");
		RETURN_FALSE;
	}

	writesize = ((*data)->value.str.len<shmop->size-(*offset)->value.lval) ? (*data)->value.str.len : shmop->size-(*offset)->value.lval;	
	memcpy(shmop->addr+(*offset)->value.lval, (*data)->value.str.val, writesize);

	RETURN_LONG(writesize);
}
/* }}} */


/* {{{ proto bool shm_delete (int shmid)
   shm_delete - mark segment for deletion */
PHP_FUNCTION(shm_delete)
{
	zval **shmid;
	struct php_shmop *shmop;
	int type;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &shmid) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(shmid);

	shmop = zend_list_find((*shmid)->value.lval, &type);

	if (!shmop) {
		php_error(E_WARNING, "shmdelete: error no such segment");
		RETURN_FALSE;
	}

	if (shmctl(shmop->shmid, IPC_RMID, NULL)) {
		php_error(E_WARNING, "shmdelete: can't mark segment for deletion (are you the owner?)");
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

#endif	/* HAVE_SHMOP */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
