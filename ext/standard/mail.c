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
   | Authors:                                                             |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "php.h"
#ifndef MSVC5
#include "build-defs.h"
#endif
#include "php3_mail.h"
#include "php_ini.h"

#if APACHE
#  ifndef DEBUG
#  undef palloc
#  endif
#endif

#if HAVE_SENDMAIL
#if MSVC5
#include "win32/sendmail.h"
#endif

function_entry mail_functions[] = {
	PHP_FE(mail,									NULL)
	{NULL, NULL, NULL}
};

php3_module_entry mail_module_entry = {
	"Sendmail", mail_functions, NULL, NULL, NULL, NULL, php3_info_mail, STANDARD_MODULE_PROPERTIES
};


#if COMPILE_DL
DLEXPORT php3_module_entry *get_module(void) { return &odbc_module_entry; }
#endif

/* {{{ proto int mail(string to, string subject, string message [, string additional_headers])
   Send an email message */
PHP_FUNCTION(mail)
{
	pval *argv[4];
	char *to=NULL, *message=NULL, *headers=NULL, *subject=NULL;
	int argc;
	
	argc = ARG_COUNT(ht);
	if (argc < 3 || argc > 4 || getParametersArray(ht, argc, argv) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	/* To: */
	convert_to_string(argv[0]);
	if (argv[0]->value.str.val) {
		to = argv[0]->value.str.val;
	} else {
		php3_error(E_WARNING, "No to field in mail command");
		RETURN_FALSE;
	}

	/* Subject: */
	convert_to_string(argv[1]);
	if (argv[1]->value.str.val) {
		subject = argv[1]->value.str.val;
	} else {
		php3_error(E_WARNING, "No subject field in mail command");
		RETURN_FALSE;
	}

	/* message body */
	convert_to_string(argv[2]);
	if (argv[2]->value.str.val) {
		message = argv[2]->value.str.val;
	} else {
		/* this is not really an error, so it is allowed. */
		php3_error(E_WARNING, "No message string in mail command");
		message = NULL;
	}

	if (argc == 4) {			/* other headers */
		convert_to_string(argv[3]);
		headers = argv[3]->value.str.val;
	}
	
	if (_php3_mail(to, subject, message, headers)){
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

int _php3_mail(char *to, char *subject, char *message, char *headers)
{
#if MSVC5
	int tsm_err;
#else
	FILE *sendmail;
	int ret;
	char *sendmail_path = INI_STR("sendmail_path");
#endif

#if MSVC5
	if (TSendMail(INI_STR("SMTP"), &tsm_err, headers, subject, to, message) != SUCCESS){
		php3_error(E_WARNING, GetSMErrorText(tsm_err));
		return 0;
	}
#else
	if (!sendmail_path) {
		return 0;
	}
	sendmail = popen(sendmail_path, "w");

	if (sendmail) {
		fprintf(sendmail, "To: %s\n", to);
		fprintf(sendmail, "Subject: %s\n", subject);
		if (headers != NULL) {
			fprintf(sendmail, "%s\n", headers);
		}
		fprintf(sendmail, "\n%s\n", message);
		ret = pclose(sendmail);
		if (ret == -1) {
			return 0;
		} else {
			return 1;
		}
	} else {
		php3_error(E_WARNING, "Could not execute mail delivery program");
		return 0;
	}
#endif
	return 1;
}

void php3_info_mail(ZEND_MODULE_INFO_FUNC_ARGS)
{
#if MSVC5
	PUTS("Internal Sendmail support for Windows 4");
#else
	php3_printf("Path to sendmail: <tt>%s</tt>", INI_STR("sendmail_path"));
#endif
}

#else

PHP_FUNCTION(mail) {}
void php3_info_mail(ZEND_MODULE_INFO_FUNC_ARGS) {}

#endif


/*
 * Local variables:
 * tab-width: 4
 * End:
 */
