/*
   +----------------------------------------------------------------------+
   | msession 1.0                                                         |
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
   | Authors: Mark Woodward <markw@mohawksoft.com>                        |
   |    Portions copyright the PHP group.                                 |
   +----------------------------------------------------------------------+
 */
#include "php.h"
#include "php_ini.h"
#include "php_msession.h"
#include "reqclient.h"
#include "ext/standard/info.h"
#include "ext/session/php_session.h"


/* Macros and such */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifdef ERR_DEBUG
#define ELOG( str )	php_log_err( str )
#else
#define ELOG( str )
#endif

/* Test if system is OK fror use. */
/* Macros may be ugly, but I can globaly add debuging when needed. */
#define	IFCONNECT_BEGIN		if(s_reqb && s_conn) {
#define IFCONNECT_ENDVAL(V) 	} else { php_error(E_WARNING, s_szNoInit); return V; }
#define IFCONNECT_END		} else { php_error(E_WARNING, s_szNoInit); RETURN_FALSE; }
	
/* Uncomment to get debugging messages */
/* #define ERR_DEBUG */

/* This sets the PHP API version used in the file. */
/* If this module does not compile on the version of PHP you are using, look for */
/* this value in Zend/zend_modules.h, and set appropriately */

#if (ZEND_MODULE_API_NO <=  20001222)
#define PHP_4_0
#define TSRMLS_CC
#define TSRMLS_FETCH()
/* Comment out this line if you wish to have msession without php sessions */
#define HAVE_PHP_SESSION
#warning Backward compatible msession extension requires PHP sessions. If PHP compiles and links, you can ignore this warning.
#elif (ZEND_MODULE_API_NO >= 20010901)
#define PHP_4_1
#endif

/*
 * Please do not remove backward compatibility from this module.
 * this same source must also work with the 4.0.6 version of PHP.
 *
 * Module Variable naming hints:
 * All global scope variables begin with "g_" for global.
 * All static file scoped variables begin with "s_" for static.
 * Zero terminated strings generally start with "sz" for "string followed by zero."
 * integers that hold string or data lengths generally start with "cb" for "count of bytes."
 * Also, please to not reformat braces ;-)
 * -MLW
 */

#if HAVE_MSESSION
#ifdef HAVE_PHP_SESSION
/* If the PHP Session module is compiled or available, include support */
PS_FUNCS(msession);

ps_module ps_mod_msession = {
	PS_MOD(msession)
};
#endif

/* Static strings */
static char s_szNoInit[]="Msession not initialized";
static char s_szErrFmt[]="MSession Error :%s";

/* Per-process variables need by msession */
static char	s_szdefhost[]="localhost";
static char *	s_szhost=s_szdefhost;
static int	s_port=8086;
static void *	s_conn=NULL;
static REQB *	s_reqb=NULL;

function_entry msession_functions[] = {
	PHP_FE(msession_connect,NULL)
	PHP_FE(msession_disconnect,NULL)
	PHP_FE(msession_lock,NULL)
	PHP_FE(msession_unlock,NULL)
	PHP_FE(msession_count,NULL)
	PHP_FE(msession_create,NULL)
	PHP_FE(msession_destroy,NULL)
	PHP_FE(msession_set,NULL)
	PHP_FE(msession_get,NULL)
	PHP_FE(msession_find,NULL)
	PHP_FE(msession_get_array,NULL)
	PHP_FE(msession_set_array,NULL)
	PHP_FE(msession_timeout,NULL)
	PHP_FE(msession_inc,NULL)
	PHP_FE(msession_set_data,NULL)
	PHP_FE(msession_get_data,NULL)
	PHP_FE(msession_listvar,NULL)
	PHP_FE(msession_list,NULL)
	PHP_FE(msession_uniq,NULL)
	PHP_FE(msession_randstr,NULL)
	PHP_FE(msession_plugin,NULL)
	PHP_FE(msession_call,NULL)
	{NULL, NULL, NULL}
};

zend_module_entry msession_module_entry = {
#ifdef PHP_4_1
	STANDARD_MODULE_HEADER,
#endif
	"msession",
	msession_functions,
	PHP_MINIT(msession),
	PHP_MSHUTDOWN(msession),
	PHP_RINIT(msession),
	PHP_RSHUTDOWN(msession),
	PHP_MINFO(msession),
#ifdef PHP_4_1
	NO_VERSION_YET,
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_MSESSION
ZEND_GET_MODULE(msession)
#endif

PHP_MINIT_FUNCTION(msession)
{
	s_conn = NULL;
	s_szhost = s_szdefhost;
	
#ifdef HAVE_PHP_SESSION
	php_session_register_module(&ps_mod_msession);
#endif
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(msession)
{
	return SUCCESS;
}

PHP_RINIT_FUNCTION(msession)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(msession)
{
	if(s_conn)
	{
		CloseReqConn(s_conn);
		s_conn = NULL;
	}

	if(s_reqb)
	{
		FreeRequestBuffer(s_reqb);
		s_reqb=NULL;
	}
	return SUCCESS;
}

PHP_MINFO_FUNCTION(msession)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "msession support", "enabled");
	php_info_print_table_end();
}

int PHPMsessionConnect(const char *szhost, int nport)
{

	TSRMLS_FETCH();
	
	if(!s_reqb)
		s_reqb = AllocateRequestBuffer(2048);

	if(!s_reqb) // no buffer, it won't work!
		return 0;

	if(s_conn)
	{
		CloseReqConn(s_conn);
		php_log_err("Call to connect with non-null s_conn" TSRMLS_CC);
	}
	if(strcmp(s_szhost, szhost))
	{
		if(s_szhost != s_szdefhost)
			free(s_szhost);
		s_szhost = strdup(szhost);
	}
	if(nport)
		s_port = nport;

	s_conn = OpenReqConn(s_szhost, s_port);

#ifdef ERR_DEBUG
{
	char buffer[256];
	sprintf(buffer,"Connect: %s [%d] = %d (%X)\n", 
		s_szhost, s_port, (s_conn != NULL), (unsigned)s_conn);
	php_log_err(buffer);
}
#endif
	return (s_conn) ? 1 : 0;
}

void PHPMsessionDisconnect()
{
	if(s_conn)
	{
		CloseReqConn(s_conn);
		s_conn = NULL;
	}
	if(s_reqb)
	{
		FreeRequestBuffer(s_reqb);
		s_reqb = NULL;
	}
}

char *PHPMsessionGetData(const char *session)
{
	char *ret = NULL;

	IFCONNECT_BEGIN

	FormatRequest(&s_reqb, REQ_DATAGET, session,"","",0);
	DoRequest(s_conn, &s_reqb);

	if(s_reqb->req.stat==REQ_OK)
		ret = safe_estrdup(s_reqb->req.datum);
	else if(s_reqb->req.param !=  REQE_NOSESSION)
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
	IFCONNECT_ENDVAL(0)
	
	return ret;
}
int PHPMsessionSetData(const char *session, const char *data)
{

	IFCONNECT_BEGIN
	int ret=0;

	FormatRequest(&s_reqb, REQ_DATASET, session,"",data,0);
	DoRequest(s_conn,&s_reqb);
	ret = (s_reqb->req.stat==REQ_OK);
	if(s_reqb->req.stat!=REQ_OK)
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));

	return ret;

	IFCONNECT_ENDVAL(0)
}

int PHPMsessionDestroy(const char *session)
{
	IFCONNECT_BEGIN

	int ret=0;
	FormatRequest(&s_reqb, REQ_DROP, session, "","",0);
	DoRequest(s_conn,&s_reqb);
	ret = (s_reqb->req.stat==REQ_OK);
	if(s_reqb->req.stat!=REQ_OK)
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
	return ret;
	
	IFCONNECT_ENDVAL(0)
}

/* {{{ proto bool msession_connect(string host, string port)
	 Connect to msession sever */
PHP_FUNCTION(msession_connect)
{
	char *szhost;
	int nport;
	
	zval **zhost;
	zval **zport;
	
	if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &zhost, &zport) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}
	
	convert_to_string_ex(zhost);
	convert_to_string_ex(zport);

	szhost = Z_STRVAL_PP(zhost);
	nport = atoi(Z_STRVAL_PP(zport));

	if(PHPMsessionConnect(szhost,nport))
	{
		RETURN_TRUE;
	}
	else
	{
		php_error(E_WARNING, "MSession connect failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto void msession_disconnect(void)
   Disconnect from msession server */
PHP_FUNCTION(msession_disconnect)
{
	PHPMsessionDisconnect();
	RETURN_NULL();
}
/* }}} */

/* {{{ proto int msession_count(void)
   Get session count */
PHP_FUNCTION(msession_count)
{
	IFCONNECT_BEGIN

	int count;
	FormatRequest(&s_reqb, REQ_COUNT, "", "","",0);
	DoRequest(s_conn,&s_reqb);
	
	count = (s_reqb->req.stat == REQ_OK) ? s_reqb->req.param : 0;

	RETURN_LONG(count);

	IFCONNECT_END
}
/* }}} */

/* {{{ proto bool msession_create(string session)
   Create a session */
PHP_FUNCTION(msession_create)
{
	IFCONNECT_BEGIN
/* 	int stat; */
	char *szsession;
	zval **session;
	
	if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &session) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(session);
	szsession = Z_STRVAL_PP(session);
	FormatRequest(&s_reqb, REQ_CREATE, szsession, "","",0);
	DoRequest(s_conn,&s_reqb);
	if(s_reqb->req.stat==REQ_OK)
	{
		RETURN_TRUE;
	}
	else
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_FALSE;
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto bool msession_destroy(string name)
   Destroy a session */
PHP_FUNCTION(msession_destroy)
{
	char *szsession;
	zval **session;
	if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &session) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(session);
	szsession = Z_STRVAL_PP(session);
	PHPMsessionDestroy(szsession);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int msession_lock(string name)
   Lock a session */
PHP_FUNCTION(msession_lock)
{
	IFCONNECT_BEGIN
	char *szsession;
	zval **session;
	
	if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &session) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(session);
	szsession = Z_STRVAL_PP(session);
	FormatRequest(&s_reqb, REQ_SLOCK, szsession, "","",0);
	DoRequest(s_conn,&s_reqb);

	if(s_reqb->req.stat==REQ_OK)
	{
		RETURN_LONG(s_reqb->req.param);
	}
	else
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_FALSE;
	}

	IFCONNECT_END

}
/* }}} */

/* {{{ proto int msession_unlock(string session, int key)
   Unlock a session */
PHP_FUNCTION(msession_unlock)
{
	IFCONNECT_BEGIN
	char *szsession;
	long lkey;
	zval **session;
	zval **key;
	
	if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &session, &key) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(session);
	szsession = Z_STRVAL_PP(session);
	convert_to_long_ex(key);
	lkey = Z_LVAL_PP(key);
	FormatRequest(&s_reqb, REQ_SUNLOCK, szsession, "","",lkey);
	DoRequest(s_conn,&s_reqb);

	if(s_reqb->req.stat==REQ_OK)
	{
		RETURN_LONG(s_reqb->req.param);
	}
	else
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_FALSE;
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto bool msession_set(string session, string name, string value)
   Set value in session */
PHP_FUNCTION(msession_set)
{
	IFCONNECT_BEGIN

	char *szsession;
	char *szname;
	char *szvalue;
	zval **session;
	zval **name;
	zval **value;
	
	if(ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3,&session,&name,&value) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(session);
	convert_to_string_ex(name);
	convert_to_string_ex(value);

	szsession = Z_STRVAL_PP(session);
	szname = Z_STRVAL_PP(name);
	szvalue = Z_STRVAL_PP(value);

	FormatRequest(&s_reqb, REQ_SETVAL, szsession, szname, szvalue, 0);
	DoRequest(s_conn,&s_reqb);

	if(s_reqb->req.stat==REQ_OK)
	{
		RETURN_TRUE;
	}
	else
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_FALSE;
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto string msession_get(string session, string name, string default_value)
   Get value from session */
PHP_FUNCTION(msession_get)
{
	IFCONNECT_BEGIN
	char *szsession;
	char *szname;
	char *szvalue;
	zval **session;
	zval **name;
	zval **value;
	
	if(ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3,&session,&name,&value) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(session);
	convert_to_string_ex(name);
	convert_to_string_ex(value);
	szsession = Z_STRVAL_PP(session);
	szname = Z_STRVAL_PP(name);
	szvalue = Z_STRVAL_PP(value);

	FormatRequest(&s_reqb, REQ_GETVAL, szsession, szname, szvalue,0);
	DoRequest(s_conn, &s_reqb);

	if(s_reqb->req.stat==REQ_OK)
	{
		szvalue = safe_estrdup(s_reqb->req.datum);
		RETURN_STRING(szvalue, 0)
	}
	else
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_NULL();
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto string msession_uniq(int num_chars)
   Get uniq id */
PHP_FUNCTION(msession_uniq)
{
	IFCONNECT_BEGIN

	long val;
	zval **param;
	
	if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1,&param) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(param);
	val = Z_LVAL_PP(param);

	FormatRequest(&s_reqb, REQ_UNIQ,"", "", "",val);
	DoRequest(s_conn, &s_reqb);

	if(s_reqb->req.stat==REQ_OK)
	{
		char *szval = safe_estrdup(s_reqb->req.datum);
		RETURN_STRING(szval, 0)
	}
	else
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_NULL();
	}
	IFCONNECT_END

}
/* }}} */

/* {{{ proto string msession_randstr(int num_chars)
   Get random string */
PHP_FUNCTION(msession_randstr)
{
	IFCONNECT_BEGIN
	long val;
	zval **param;
	
	if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1,&param) == FAILURE)
	{
			WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(param);
	val = Z_LVAL_PP(param);

	FormatRequest(&s_reqb, REQ_RANDSTR,"", "", "",val);
	DoRequest(s_conn, &s_reqb);

	if(s_reqb->req.stat==REQ_OK)
	{
		char *szval = safe_estrdup(s_reqb->req.datum);
		RETURN_STRING(szval, 0)
	}
	else
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_NULL();
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto array msession_find(string name, string value)
   Find all sessions with name and value */
PHP_FUNCTION(msession_find)
{
	IFCONNECT_BEGIN

	char *szname;
	char *szvalue;
	zval **name;
	zval **value;
	
	if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &name, &value) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(name);
	convert_to_string_ex(value);
	szname = Z_STRVAL_PP(name);
	szvalue = Z_STRVAL_PP(value);

	FormatRequest(&s_reqb, REQ_FIND, "", szname, szvalue,0);
	DoRequest(s_conn,&s_reqb);

	if(s_reqb->req.stat==REQ_OK && s_reqb->req.param)
	{
		int i;
		char *str = s_reqb->req.datum;
		array_init(return_value);

		for(i=0; i < s_reqb->req.param; i++)
		{
			int cbstr = strlen(str);
			char *data = safe_estrdup(str);
			add_index_string(return_value, i, data, 0);
			str += (cbstr+1);
		}
	}
	else if(s_reqb->req.stat != REQ_OK)
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_NULL();
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto array msession_list(void)
   List all sessions  */
PHP_FUNCTION(msession_list)
{
	IFCONNECT_BEGIN
	FormatRequest(&s_reqb, REQ_LIST, "", "", "",0);
	DoRequest(s_conn,&s_reqb);

	if(s_reqb->req.stat==REQ_OK && s_reqb->req.param)
	{
		int i;
		char *str = s_reqb->req.datum;
		array_init(return_value);

		for(i=0; i < s_reqb->req.param; i++)
		{
			int cbstr = strlen(str);
			char *data = safe_estrdup(str);
			add_index_string(return_value, i, data, 0);
			str += (cbstr+1);
		}
	}
	else if(s_reqb->req.stat != REQ_OK)
	{
		// May this should be an error?
		if(s_reqb->req.param !=  REQE_NOSESSION)
			php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_NULL();
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto array msession_get_array(string session)
   Get array of msession variables  */
PHP_FUNCTION(msession_get_array)
{
	IFCONNECT_BEGIN
	char *szsession;
	zval **session;
	if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &session) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
  }
	convert_to_string_ex(session);
	szsession = Z_STRVAL_PP(session);
	FormatRequest(&s_reqb, REQ_GETALL, szsession, "", "",0);
	DoRequest(s_conn,&s_reqb);

	array_init(return_value);

	if(s_reqb->req.stat == REQ_OK)
	{
		int i;
		char *str = s_reqb->req.datum;
		int num = s_reqb->req.param*2;

		for(i=0; i < num; i+=2)
		{
			int cbvalue;
			int cbname;
			char *szvalue;
			char *szname;

			cbname = strlen(str);
			szname = safe_estrndup(str,cbname);
			str += (cbname+1);

			cbvalue = strlen(str);
			szvalue = safe_estrndup(str,cbvalue);
			str += (cbvalue+1);
			add_assoc_string(return_value, szname, szvalue, 0);
		}
	}
	else
	{
		if(s_reqb->req.param !=  REQE_NOSESSION)
			php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_NULL();
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto bool msession_set_array(string session, array tuples)
   Set msession variables from an array*/
PHP_FUNCTION(msession_set_array)
{
	IFCONNECT_BEGIN
	zval **session;
	zval **tuples;
	HashPosition pos;
	zval **entry;
	char *key;
#ifdef PHP_4_1
	uint keylen;
#endif
#ifdef PHP_4_0
	ulong keylen;
#endif
	ulong numndx;
	int ndx=0;
	char **pairs;
	HashTable *htTuples;
	int i;
	
	int countpair; 
	
	if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &session, &tuples) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
	}
	htTuples = Z_ARRVAL_PP(tuples);

	countpair = zend_hash_num_elements(htTuples);

	pairs = (char **)emalloc(sizeof(char *) * countpair * 2);

	if(!pairs)
	{
		ELOG("no pairs");
		RETURN_FALSE;
	}

	ELOG("have pairs");

	// Initializes pos
	zend_hash_internal_pointer_reset_ex(htTuples, &pos);

	ELOG("reset pointer");

	for(i=0; i < countpair; i++)
	{
		if(zend_hash_get_current_data_ex(htTuples, (void **)&entry, &pos) != SUCCESS) 
			break;

		if(entry)
		{
			char *szentry;
			convert_to_string_ex(entry);
			szentry = Z_STRVAL_PP(entry);
			
       			if(zend_hash_get_current_key_ex(htTuples,&key,&keylen,&numndx,0,&pos)== HASH_KEY_IS_STRING)
			{
#ifdef ERR_DEBUG
{
				char buffer [256];
				sprintf(buffer, "%s=%s\n", key, szentry);
				ELOG(buffer);
}
#endif
				pairs[ndx++] = key;
				pairs[ndx++] = szentry;
			}
		}
		zend_hash_move_forward_ex(htTuples, &pos);
	}

	ELOG("FormatMulti");
	FormatRequestMulti(&s_reqb, REQ_SETVAL, Z_STRVAL_PP(session), countpair, pairs,0);
	DoRequest(s_conn,&s_reqb);

	if(s_reqb->req.stat != REQ_OK)
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
	efree((void *)pairs);
	IFCONNECT_END
}
/* }}} */

/* {{{ proto array msession_listvar(string name)
   return associative array of value:session for all sessions with a variable named 'name' */
PHP_FUNCTION(msession_listvar)
{
	IFCONNECT_BEGIN
	zval **name;
	if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &name) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(name);
	FormatRequest(&s_reqb, REQ_LISTVAR, "", Z_STRVAL_PP(name), "",0);
	DoRequest(s_conn,&s_reqb);

	array_init(return_value);

	if(s_reqb->req.stat == REQ_OK)
	{
		int i;
		char *str = s_reqb->req.datum;
		int num = s_reqb->req.param*2;

		for(i=0; i < num; i+=2)
		{
			int cbvalue;
			int cbname;
			char *szvalue;
			char *szname;

			cbname= strlen(str);
			szname= safe_estrndup(str,cbname);
			str += (cbname+1);

			cbvalue = strlen(str);
			szvalue = safe_estrndup(str,cbvalue);
			str += (cbvalue+1);
			add_assoc_string(return_value, szname, szvalue, 0);
		}
	}
	else
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_NULL();
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto int msession_timeout(string session [, int param ])
   Set/get session timeout */
PHP_FUNCTION(msession_timeout)
{
	IFCONNECT_BEGIN
	zval **session;
	int ac = ZEND_NUM_ARGS();
	int zstat = FAILURE;
	int timeout = 0;
	if(ac == 1)
	{
		zstat = zend_get_parameters_ex(1, &session);
	}
	else if(ac == 2)
	{
		zval **param;
		zstat = zend_get_parameters_ex(2, &session, &param);
		convert_to_long_ex(param);
		timeout = Z_LVAL_PP(param);
	}
	if(zstat == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(session);
	FormatRequest(&s_reqb, REQ_TIMEOUT, Z_STRVAL_PP(session), "","",timeout);
	DoRequest(s_conn,&s_reqb);

	if(s_reqb->req.stat == REQ_OK)
	{
		RETURN_LONG( s_reqb->req.param);
	}
	else
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_NULL();
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto string msession_inc(string session, string name) 
   Increment value in session */
PHP_FUNCTION(msession_inc)
{
	IFCONNECT_BEGIN
	char *val;
	zval **session;
	zval **name;
	
	if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &session, &name) == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(session);
	convert_to_string_ex(name);

	FormatRequest(&s_reqb, REQ_INC, Z_STRVAL_PP(session), Z_STRVAL_PP(name),0,0);
	DoRequest(s_conn, &s_reqb);

	if(s_reqb->req.stat==REQ_OK)
	{
		val = safe_estrdup(s_reqb->req.datum);
		RETURN_STRING(val, 0)
	}
	else
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_FALSE;
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto string msession_get_data(string session)
   Get data session unstructured data. (PHP sessions use this)  */
PHP_FUNCTION(msession_get_data)
{
	IFCONNECT_BEGIN
	char *val;
	zval **session;
	
	if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &session) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(session);

	val = PHPMsessionGetData(Z_STRVAL_PP(session));

	if(val)
	{
		RETURN_STRING(val, 0)
	}
	else
	{
		RETURN_NULL();
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto bool msession_set_data(string session, string value)
   Set data session unstructured data. (PHP sessions use this)  */
PHP_FUNCTION(msession_set_data)
{
	IFCONNECT_BEGIN
	zval **session;
	zval **value;
	
	if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &session, &value) == FAILURE)
 	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(session);
	convert_to_string_ex(value);

	if(PHPMsessionSetData(Z_STRVAL_PP(session),Z_STRVAL_PP(value)))
	{
		RETURN_TRUE;
	}
	else
	{
		RETURN_FALSE;
	}
	IFCONNECT_END
}
/* }}} */

/* {{{ proto string msession_plugin(string session, string val [, string param ])
   Call the personality plugin escape function */
PHP_FUNCTION(msession_plugin)
{
	IFCONNECT_BEGIN
	int ret;
	char *retval;
	zval **session;
	zval **val;
	zval **param=NULL;
	
	if(ZEND_NUM_ARGS() == 3)
	{
		ret = zend_get_parameters_ex(3, &session, &val, &param);
		convert_to_string_ex(param);
	}
	else if(ZEND_NUM_ARGS() == 2)
	{
		ret = zend_get_parameters_ex(2, &session, &val);
	}
	else
 	{
		WRONG_PARAM_COUNT;
	}
	if(ret == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(session);
	convert_to_string_ex(val);

	ret = atoi(Z_STRVAL_PP(val));

	FormatRequest(&s_reqb, REQ_PLUGIN, Z_STRVAL_PP(session), Z_STRVAL_PP(val), param ? Z_STRVAL_PP(param) : "",ret);
	DoRequest(s_conn, &s_reqb);

	if(s_reqb->req.stat==REQ_OK && s_reqb->req.len)
	{
		retval = safe_estrdup(s_reqb->req.datum);
		RETURN_STRING(retval, 0)
	}
	else if(s_reqb->req.stat != REQ_OK)
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_FALSE;
		
	}
	IFCONNECT_END
}
/* }}} */
#define MAX_EXT_VAL	5
/* {{{ proto string msession_call(string fn_name, [, string param1 ], ... [,string param4])
   Call the plugin function named fn_name */
PHP_FUNCTION(msession_call)
{
	IFCONNECT_BEGIN
	int n;
	int i;
	int ret;
	char *retval;
	zval **val[MAX_EXT_VAL];
	char *strings[MAX_EXT_VAL+1];
/* 	zval **param=NULL; */
	
	n = ZEND_NUM_ARGS();

	if((n < 1) || (n > MAX_EXT_VAL))
 	{
		WRONG_PARAM_COUNT;
	}

	ret = zend_get_parameters_ex(ZEND_NUM_ARGS(), &val[0],&val[1],&val[2],&val[3],&val[4]);

	if(ret == FAILURE)
	{
		WRONG_PARAM_COUNT;
	}
	for(i=0; i < n; i++)
	{
		convert_to_string_ex(val[i]);
		strings[i] = Z_STRVAL_PP(val[i]);
	}

	FormatRequestStrings(&s_reqb, REQ_CALL, NULL , n, strings);
	DoRequest(s_conn, &s_reqb);

	if(s_reqb->req.stat==REQ_OK && s_reqb->req.len)
	{
		retval = safe_estrdup(s_reqb->req.datum);
		RETURN_STRING(retval, 0)
	}
	else if(s_reqb->req.stat != REQ_OK)
	{
		php_error(E_WARNING, s_szErrFmt, ReqErr(s_reqb->req.param));
		RETURN_FALSE;
	}
	IFCONNECT_END
}
/* }}} */

#ifdef HAVE_PHP_SESSION

PS_OPEN_FUNC(msession)
{
	ELOG( "ps_open_msession");
	PS_SET_MOD_DATA((void *)1); // session.c needs a non-zero here!
	return PHPMsessionConnect(save_path, 8086) ? SUCCESS : FAILURE;
}

PS_CLOSE_FUNC(msession)
{
	ELOG( "ps_close_msession");
	PHPMsessionDisconnect();
	return SUCCESS;
}

PS_READ_FUNC(msession)
{
	ELOG( "ps_read_msession");
	*val = PHPMsessionGetData(key);
	if(*val)
		{
			*vallen = strlen(*val);
		}
	else
	{
		*val = emalloc(1);
		**val=0;
		*vallen = 0;
	}
	return SUCCESS;
}

PS_WRITE_FUNC(msession)
{
	ELOG( "ps_write_msession");
	return (PHPMsessionSetData(key,val)) ? SUCCESS : FAILURE;
}

PS_DESTROY_FUNC(msession)
{
	ELOG( "ps_destroy_msession");
	return (PHPMsessionDestroy(key)) ? SUCCESS : FAILURE;
}

PS_GC_FUNC(msession)
{
	ELOG( "ps_gc_msession");
	return SUCCESS;
}
#endif	/* HAVE_PHP_SESSION */
#endif	/* HAVE_MSESSION */

