/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2004 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Wez Furlong <wez@php.net>                                    |
  |         Marcus Boerger <helly@php.net>                               |
  |         Sterling Hughes <sterling@php.net>                           |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

/* Stuff private to the PDO extension and not for consumption by PDO drivers
 * */
extern zend_class_entry *pdo_exception_ce;

extern zend_object_value pdo_dbh_new(zend_class_entry *ce TSRMLS_DC);
extern function_entry pdo_dbh_functions[];
extern zend_class_entry *pdo_dbh_ce;

extern zend_object_value pdo_dbstmt_new(zend_class_entry *ce TSRMLS_DC);
extern function_entry pdo_dbstmt_functions[];
extern zend_class_entry *pdo_dbstmt_ce;
void pdo_dbstmt_free_storage(zend_object *object TSRMLS_DC);
extern zend_object_handlers pdo_dbstmt_object_handlers;

extern pdo_driver_t *pdo_find_driver(const char *name, int namelen);

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
