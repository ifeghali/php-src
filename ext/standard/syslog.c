/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999 The PHP Group                         |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Stig S�ther Bakken <ssb@guardian.no>                        |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "php.h"
#include "php_ini.h"
#include "zend_globals.h"

#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if MSVC5
#include "win32/syslog.h"
#else
#include <syslog.h>
#endif

#include <string.h>
#include <errno.h>

#include <stdio.h>
#include "php3_syslog.h"

static int syslog_started;
static char *syslog_device;
static void start_syslog(void);

int php3_minit_syslog(INIT_FUNC_ARGS)
{
	ELS_FETCH();
	
	/* error levels */
	REGISTER_LONG_CONSTANT("LOG_EMERG", LOG_EMERG, CONST_CS | CONST_PERSISTENT); /* system unusable */
	REGISTER_LONG_CONSTANT("LOG_ALERT", LOG_ALERT, CONST_CS | CONST_PERSISTENT); /* immediate action required */
	REGISTER_LONG_CONSTANT("LOG_CRIT", LOG_CRIT, CONST_CS | CONST_PERSISTENT); /* critical conditions */
	REGISTER_LONG_CONSTANT("LOG_ERR", LOG_ERR, CONST_CS | CONST_PERSISTENT); 
	REGISTER_LONG_CONSTANT("LOG_WARNING", LOG_WARNING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_NOTICE", LOG_NOTICE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_INFO", LOG_INFO, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_DEBUG", LOG_DEBUG, CONST_CS | CONST_PERSISTENT);
	/* facility: type of program logging the message */
	REGISTER_LONG_CONSTANT("LOG_KERN", LOG_KERN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_USER", LOG_USER, CONST_CS | CONST_PERSISTENT); /* generic user level */
	REGISTER_LONG_CONSTANT("LOG_MAIL", LOG_MAIL, CONST_CS | CONST_PERSISTENT); /* log to email */
	REGISTER_LONG_CONSTANT("LOG_DAEMON", LOG_DAEMON, CONST_CS | CONST_PERSISTENT); /* other system daemons */
	REGISTER_LONG_CONSTANT("LOG_AUTH", LOG_AUTH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_SYSLOG", LOG_SYSLOG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LPR", LOG_LPR, CONST_CS | CONST_PERSISTENT);
#ifdef LOG_NEWS
	/* No LOG_NEWS on HP-UX */
	REGISTER_LONG_CONSTANT("LOG_NEWS", LOG_NEWS, CONST_CS | CONST_PERSISTENT); /* usenet new */
#endif
#ifdef LOG_UUCP
	/* No LOG_UUCP on HP-UX */
	REGISTER_LONG_CONSTANT("LOG_UUCP", LOG_UUCP, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef LOG_CRON
	/* apparently some systems don't have this one */
	REGISTER_LONG_CONSTANT("LOG_CRON", LOG_CRON, CONST_CS | CONST_PERSISTENT);
#endif
#ifdef LOG_AUTHPRIV
	/* AIX doesn't have LOG_AUTHPRIV */
	REGISTER_LONG_CONSTANT("LOG_AUTHPRIV", LOG_AUTHPRIV, CONST_CS | CONST_PERSISTENT);
#endif
#ifndef MSVC5
	REGISTER_LONG_CONSTANT("LOG_LOCAL0", LOG_LOCAL0, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL1", LOG_LOCAL1, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL2", LOG_LOCAL2, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL3", LOG_LOCAL3, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL4", LOG_LOCAL4, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL5", LOG_LOCAL5, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL6", LOG_LOCAL6, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_LOCAL7", LOG_LOCAL7, CONST_CS | CONST_PERSISTENT);
#endif
	/* options */
	REGISTER_LONG_CONSTANT("LOG_PID", LOG_PID, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_CONS", LOG_CONS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_ODELAY", LOG_ODELAY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_NDELAY", LOG_NDELAY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LOG_NOWAIT", LOG_NOWAIT, CONST_CS | CONST_PERSISTENT);
#ifdef LOG_PERROR
	/* AIX doesn't have LOG_PERROR */
	REGISTER_LONG_CONSTANT("LOG_PERROR", LOG_PERROR, CONST_CS | CONST_PERSISTENT); /*log to stderr*/
#endif

	return SUCCESS;
}


int php3_rinit_syslog(INIT_FUNC_ARGS)
{
	if (INI_INT("define_syslog_variables")) {
		start_syslog();
	} else {
		syslog_started=0;
	}
	syslog_device=NULL;
	return SUCCESS;
}


int php3_rshutdown_syslog(SHUTDOWN_FUNC_ARGS)
{
	if (syslog_device) {
		efree(syslog_device);
	}
	return SUCCESS;
}


static void start_syslog(void)
{
	ELS_FETCH();
	
	/* error levels */
	SET_VAR_LONG("LOG_EMERG", LOG_EMERG); /* system unusable */
	SET_VAR_LONG("LOG_ALERT", LOG_ALERT); /* immediate action required */
	SET_VAR_LONG("LOG_CRIT", LOG_CRIT); /* critical conditions */
	SET_VAR_LONG("LOG_ERR", LOG_ERR); 
	SET_VAR_LONG("LOG_WARNING", LOG_WARNING);
	SET_VAR_LONG("LOG_NOTICE", LOG_NOTICE);
	SET_VAR_LONG("LOG_INFO", LOG_INFO);
	SET_VAR_LONG("LOG_DEBUG", LOG_DEBUG);
	/* facility: type of program logging the message */
	SET_VAR_LONG("LOG_KERN", LOG_KERN);
	SET_VAR_LONG("LOG_USER", LOG_USER); /* generic user level */
	SET_VAR_LONG("LOG_MAIL", LOG_MAIL); /* log to email */
	SET_VAR_LONG("LOG_DAEMON", LOG_DAEMON); /* other system daemons */
	SET_VAR_LONG("LOG_AUTH", LOG_AUTH);
	SET_VAR_LONG("LOG_SYSLOG", LOG_SYSLOG);
	SET_VAR_LONG("LOG_LPR", LOG_LPR);
#ifdef LOG_NEWS
	/* No LOG_NEWS on HP-UX */
	SET_VAR_LONG("LOG_NEWS", LOG_NEWS); /* usenet new */
#endif
#ifdef LOG_UUCP
	/* No LOG_UUCP on HP-UX */
	SET_VAR_LONG("LOG_UUCP", LOG_UUCP);
#endif
#ifdef LOG_CRON
	/* apparently some systems don't have this one */
	SET_VAR_LONG("LOG_CRON", LOG_CRON);
#endif
#ifdef LOG_AUTHPRIV
	/* AIX doesn't have LOG_AUTHPRIV */
	SET_VAR_LONG("LOG_AUTHPRIV", LOG_AUTHPRIV);
#endif
#ifndef MSVC5
	SET_VAR_LONG("LOG_LOCAL0", LOG_LOCAL0);
	SET_VAR_LONG("LOG_LOCAL1", LOG_LOCAL1);
	SET_VAR_LONG("LOG_LOCAL2", LOG_LOCAL2);
	SET_VAR_LONG("LOG_LOCAL3", LOG_LOCAL3);
	SET_VAR_LONG("LOG_LOCAL4", LOG_LOCAL4);
	SET_VAR_LONG("LOG_LOCAL5", LOG_LOCAL5);
	SET_VAR_LONG("LOG_LOCAL6", LOG_LOCAL6);
	SET_VAR_LONG("LOG_LOCAL7", LOG_LOCAL7);
#endif
	/* options */
	SET_VAR_LONG("LOG_PID", LOG_PID);
	SET_VAR_LONG("LOG_CONS", LOG_CONS);
	SET_VAR_LONG("LOG_ODELAY", LOG_ODELAY);
	SET_VAR_LONG("LOG_NDELAY", LOG_NDELAY);
	SET_VAR_LONG("LOG_NOWAIT", LOG_NOWAIT);
#ifdef LOG_PERROR
	/* AIX doesn't have LOG_PERROR */
	SET_VAR_LONG("LOG_PERROR", LOG_PERROR); /*log to stderr*/
#endif

	syslog_started=1;
}

/* {{{ proto void define_syslog_variables(void)
   Initializes all syslog-related variables */
PHP_FUNCTION(define_syslog_variables)
{
	if (!syslog_started) {
		start_syslog();
	}
}

/* {{{ proto int openlog(string ident, int option, int facility)
   Open connection to system logger */
/*
   ** OpenLog("nettopp", $LOG_PID, $LOG_LOCAL1);
   ** Syslog($LOG_EMERG, "help me!")
   ** CloseLog();
 */
PHP_FUNCTION(openlog)
{
	pval *ident, *option, *facility;
	if (ARG_COUNT(ht) != 3 || getParameters(ht, 3, &ident, &option, &facility) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(ident);
	convert_to_long(option);
	convert_to_long(facility);
	if (syslog_device) {
		efree(syslog_device);
	}
	syslog_device = estrndup(ident->value.str.val,ident->value.str.len);
	openlog(syslog_device, option->value.lval, facility->value.lval);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int closelog(void)
   Close connection to system logger */
PHP_FUNCTION(closelog)
{
	closelog();
	if (syslog_device) {
		efree(syslog_device);
		syslog_device=NULL;
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int syslog(int priority, string message)
   Generate a system log message */
PHP_FUNCTION(syslog)
{
	pval *priority, *message;

	if (ARG_COUNT(ht) != 2 || getParameters(ht, 2, &priority, &message) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(priority);
	convert_to_string(message);

	/*
	 * CAVEAT: if the message contains patterns such as "%s",
	 * this will cause problems.
	 */

	syslog(priority->value.lval, message->value.str.val);
	RETURN_TRUE;
}
/* }}} */


function_entry syslog_functions[] = {
	PHP_FE(openlog,									NULL)
	PHP_FE(syslog,									NULL)
	PHP_FE(closelog,								NULL)
	PHP_FE(define_syslog_variables,					NULL)
	{NULL, NULL, NULL}
};


php3_module_entry syslog_module_entry = {
	"Syslog", syslog_functions, php3_minit_syslog, NULL, php3_rinit_syslog, php3_rshutdown_syslog, NULL, STANDARD_MODULE_PROPERTIES
};


#if COMPILE_DL
DLEXPORT php3_module_entry *get_module(void) { return &syslog_module_entry; }
#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
