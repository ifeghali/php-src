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
   | Authors: Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
 */

#include "php.h"
#include "php_ini.h"
#include "php_globals.h"
#include "ext/standard/head.h"
#include "info.h"
#include "SAPI.h"
#include <time.h>
#if !defined(PHP_WIN32)
#include "build-defs.h"
#endif
#include "zend_globals.h"		/* needs ELS */
#include "zend_highlight.h"


#define PHP_CONF_LONG(directive,value1,value2) \
	php_printf("<TR VALIGN=\"baseline\" BGCOLOR=\"" PHP_CONTENTS_COLOR "\"><TD BGCOLOR=\"" PHP_ENTRY_NAME_COLOR "\">%s<BR></TD><TD>%ld<BR></TD><TD>%ld<BR></TD></TR>\n",directive,value1,value2);

#define SECTION(name)  PUTS("<H2>" name "</H2>\n")

#define CREDIT_LINE(module, authors) php_info_print_table_row(2, module, authors)


static int _display_module_info(zend_module_entry *module, void *arg)
{
	int show_info_func = *((int *) arg);

	if (show_info_func && module->info_func) {
		php_printf("<A NAME=\"module_%s\"><H2>%s</H2>\n", module->name, module->name);
		module->info_func(module);
	} else if (!show_info_func && !module->info_func) {
		php_printf("<TR VALIGN=\"baseline\" BGCOLOR=\"" PHP_CONTENTS_COLOR "\">");
		php_printf("<TD>");
		php_printf(module->name);
		php_printf("</TD></TR>\n");
	}
	return 0;
}


static void php_print_gpcse_array(char *name, uint name_length ELS_DC)
{
	zval **data, **tmp;
	char *string_key;
	ulong num_key;

	if (zend_hash_find(&EG(symbol_table), name, name_length+1, (void **) &data)!=FAILURE
		&& ((*data)->type==IS_ARRAY)) {
		zend_hash_internal_pointer_reset((*data)->value.ht);
		while (zend_hash_get_current_data((*data)->value.ht, (void **) &tmp) == SUCCESS) {
			zval tmp2, *value_ptr;

			if ((*tmp)->type != IS_STRING) {
				tmp2 = **tmp;
				zval_copy_ctor(&tmp2);
				convert_to_string(&tmp2);
				value_ptr = &tmp2;
			} else {
				value_ptr = *tmp;
			}
			PUTS("<TR VALIGN=\"baseline\" BGCOLOR=\"" PHP_CONTENTS_COLOR "\">");
			PUTS("<TD BGCOLOR=\"" PHP_ENTRY_NAME_COLOR "\"><B>");
			PUTS(name);
			PUTS("[\"");
			switch (zend_hash_get_current_key((*data)->value.ht, &string_key, &num_key)) {
				case HASH_KEY_IS_STRING:
					PUTS(string_key);
					efree(string_key);
					break;
				case HASH_KEY_IS_LONG:
					php_printf("%ld",num_key);
					break;
			}
			PUTS("\"]</B></TD><TD>");
			if ((*tmp)->type == IS_ARRAY) {
				PUTS("<PRE>");
				zend_print_zval_r(*tmp, 0);
				PUTS("</PRE>");
			} else {
				PUTS(value_ptr->value.str.val);
			}
			PUTS("</TD></TR>\n");
			zend_hash_move_forward((*data)->value.ht);
			if (value_ptr==&tmp2) {
				zval_dtor(value_ptr);
			}
		}
	}
}

void php_info_print_style()
{
	php_printf("<STYLE TYPE=\"text/css\"><!--\n");
	php_printf("A { text-decoration: none; }\n");
	php_printf("A:hover { text-decoration: underline; }\n");
	php_printf("H1 { font-family: arial,helvetica,sans-serif; font-size: 18pt; font-weight: bold;}\n");
	php_printf("H2 { font-family: arial,helvetica,sans-serif; font-size: 14pt; font-weight: bold;}\n");
	php_printf("BODY,TD { font-family: arial,helvetica,sans-serif; font-size: 10pt; }\n");
	php_printf("TH { font-family: arial,helvetica,sans-serif; font-size: 11pt; font-weight: bold; }\n");
	php_printf("//--></STYLE>\n");
}


PHPAPI void php_print_info(int flag)
{
	char **env,*tmp1,*tmp2;
	char *php_uname;
	int expose_php = INI_INT("expose_php");
	time_t the_time;
	struct tm *ta, tmbuf;
#ifdef PHP_WIN32
	char php_windows_uname[256];
	DWORD dwBuild=0;
	DWORD dwVersion = GetVersion();
	DWORD dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
	DWORD dwWindowsMinorVersion =  (DWORD)(HIBYTE(LOWORD(dwVersion)));
#endif
	ELS_FETCH();
	SLS_FETCH();

	the_time = time(NULL);
	ta = php_localtime_r(&the_time, &tmbuf);
	
	PUTS("<CENTER>");

	if (flag & PHP_INFO_GENERAL) {
		char *zend_version = get_zend_version();

#ifdef PHP_WIN32
		/* Get build numbers for Windows NT or Win95 */
		if (dwVersion < 0x80000000){
			dwBuild = (DWORD)(HIWORD(dwVersion));
			snprintf(php_windows_uname,255,"%s %d.%d build %d","Windows NT",dwWindowsMajorVersion,dwWindowsMinorVersion,dwBuild);
		} else {
			snprintf(php_windows_uname,255,"%s %d.%d","Windows 95/98",dwWindowsMajorVersion,dwWindowsMinorVersion);
		}
		php_uname = php_windows_uname;
#else
		php_uname=PHP_UNAME;
#endif

		php_info_print_style();

		php_info_print_box_start(1);
		if (expose_php) {
			PUTS("<a href=\"http://www.php.net/\"><img src=\"");
			if (SG(request_info).request_uri) {
				PUTS(SG(request_info).request_uri);
			}
			if ((ta->tm_mon==3) && (ta->tm_mday==1)) {
				PUTS("?=PHPE9568F36-D428-11d2-A769-00AA001ACF42\" border=0 align=\"right\"></a>");
			} else {
				PUTS("?=PHPE9568F34-D428-11d2-A769-00AA001ACF42\" border=0 align=\"right\"></a>");
			}
		}
		php_printf("<H1>PHP Version %s</H1>\n", PHP_VERSION);
		php_info_print_box_end();
		php_info_print_table_start();
		php_info_print_table_row(2, "System", php_uname );
		php_info_print_table_row(2, "Build Date", __DATE__ );
#ifdef CONFIGURE_COMMAND
		php_info_print_table_row(2, "Configure Command", CONFIGURE_COMMAND );
#endif
		if (sapi_module.pretty_name) {
			php_info_print_table_row(2, "Server API", sapi_module.pretty_name );
		}

#ifdef VIRTUAL_DIR
		php_info_print_table_row(2, "Virtual Directory Support", "enabled" );
#else
		php_info_print_table_row(2, "Virtual Directory Support", "disabled" );
#endif

		php_info_print_table_row(2, "Configuration File (php.ini) Path", CONFIGURATION_FILE_PATH );

#if ZEND_DEBUG
		php_info_print_table_row(2, "ZEND_DEBUG", "enabled" );
#else
		php_info_print_table_row(2, "ZEND_DEBUG", "disabled" );
#endif

#ifdef ZTS
		php_info_print_table_row(2, "Thread Safety", "enabled" );
#else
		php_info_print_table_row(2, "Thread Safety", "disabled" );
#endif
		php_info_print_table_end();

		/* Zend Engine */
		php_info_print_box_start(0);
		if (expose_php) {
			PUTS("<a href=\"http://www.zend.com/\"><img src=\"");
			if (SG(request_info).request_uri) {
				PUTS(SG(request_info).request_uri);
			}
			PUTS("?=PHPE9568F35-D428-11d2-A769-00AA001ACF42\" border=\"0\" align=\"right\"></a>\n");
		}
		php_printf("This program makes use of the Zend scripting language engine:<BR>");
		zend_html_puts(zend_version, strlen(zend_version));
		php_printf("</BR>\n");
		php_info_print_box_end();
	}

	if ((flag & PHP_INFO_CREDITS) && expose_php) {	
		php_info_print_hr();
		PUTS("<a href=\"");
		if (SG(request_info).request_uri) {
			PUTS(SG(request_info).request_uri);
		}
		PUTS("?=PHPB8B5F2A0-3C92-11d3-A3A9-4C7B08C10000\">");
		PUTS("<h1>PHP 4.0 Credits</h1>");
		PUTS("</a>\n");
	}

	php_ini_sort_entries();

	if (flag & PHP_INFO_CONFIGURATION) {
		php_info_print_hr();
		PUTS("<h1>Configuration</h1>\n");
		SECTION("PHP Core\n");
		display_ini_entries(NULL);
	}

	if (flag & PHP_INFO_MODULES) {
		int show_info_func;

		show_info_func = 1;
		zend_hash_apply_with_argument(&module_registry, (int (*)(void *, void *)) _display_module_info, &show_info_func);

		SECTION("Additional Modules");
		php_info_print_table_start();
		show_info_func = 0;
		zend_hash_apply_with_argument(&module_registry, (int (*)(void *, void *)) _display_module_info, &show_info_func);
		php_info_print_table_end();
	}

	if (flag & PHP_INFO_ENVIRONMENT) {
		SECTION("Environment");
		php_info_print_table_start();
		php_info_print_table_header(2, "Variable", "Value");
		for (env=environ; env!=NULL && *env !=NULL; env++) {
			tmp1 = estrdup(*env);
			if (!(tmp2=strchr(tmp1,'='))) { /* malformed entry? */
				efree(tmp1);
				continue;
			}
			*tmp2 = 0;
			tmp2++;
			php_info_print_table_row(2, tmp1, tmp2);
			efree(tmp1);
		}
		php_info_print_table_end();
	}

	if (flag & PHP_INFO_VARIABLES) {
		pval **data;

		SECTION("PHP Variables");

		php_info_print_table_start();
		php_info_print_table_header(2, "Variable", "Value");
		if (zend_hash_find(&EG(symbol_table), "PHP_SELF", sizeof("PHP_SELF"), (void **) &data) != FAILURE) {
			php_info_print_table_row(2, "PHP_SELF", (*data)->value.str.val);
		}
		if (zend_hash_find(&EG(symbol_table), "PHP_AUTH_TYPE", sizeof("PHP_AUTH_TYPE"), (void **) &data) != FAILURE) {
			php_info_print_table_row(2, "PHP_AUTH_TYPE", (*data)->value.str.val);
		}
		if (zend_hash_find(&EG(symbol_table), "PHP_AUTH_USER", sizeof("PHP_AUTH_USER"), (void **) &data) != FAILURE) {
			php_info_print_table_row(2, "PHP_AUTH_USER", (*data)->value.str.val);
		}
		if (zend_hash_find(&EG(symbol_table), "PHP_AUTH_PW", sizeof("PHP_AUTH_PW"), (void **) &data) != FAILURE) {
			php_info_print_table_row(2, "PHP_AUTH_PW", (*data)->value.str.val);
		}
		php_print_gpcse_array("HTTP_GET_VARS", sizeof("HTTP_GET_VARS")-1 ELS_CC);
		php_print_gpcse_array("HTTP_POST_VARS", sizeof("HTTP_POST_VARS")-1 ELS_CC);
		php_print_gpcse_array("HTTP_POST_FILES", sizeof("HTTP_POST_FILES")-1 ELS_CC);
		php_print_gpcse_array("HTTP_COOKIE_VARS", sizeof("HTTP_COOKIE_VARS")-1 ELS_CC);
		php_print_gpcse_array("HTTP_SERVER_VARS", sizeof("HTTP_SERVER_VARS")-1 ELS_CC);
		php_print_gpcse_array("HTTP_ENV_VARS", sizeof("HTTP_ENV_VARS")-1 ELS_CC);
		php_info_print_table_end();
	}

	if (flag & PHP_INFO_LICENSE) {
		SECTION("PHP License");
		php_info_print_box_start(0);
		PUTS("<P>\n");
		PUTS("This program is free software; you can redistribute it and/or modify ");
		PUTS("it under the terms of the PHP License as published by the PHP Group ");
		PUTS("and included in the distribution in the file:  LICENSE\n");
		PUTS("</P>\n");
		PUTS("<P>");
		PUTS("This program is distributed in the hope that it will be useful, ");
		PUTS("but WITHOUT ANY WARRANTY; without even the implied warranty of ");
		PUTS("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		PUTS("</P>\n");
		PUTS("<P>");
		PUTS("If you did not receive a copy of the PHP license, or have any questions about ");
		PUTS("PHP licensing, please contact license@php.net.\n");
		PUTS("</P>\n");
		php_info_print_box_end();
	}

	PUTS("</center>");
}


void php_print_credits(int flag)
{
	if (flag & PHP_CREDITS_FULLPAGE) {
		PUTS("<html><head><title>PHP Credits</title></head><body>\n");
	}

	php_info_print_style();

	PUTS("<center>");
	PUTS("<h1>PHP 4.0 Credits</h1>\n");

	if (flag & PHP_CREDITS_GROUP) {
		/* Group */

		php_info_print_table_start();
		php_info_print_table_header(1, "PHP Group");
		php_info_print_table_row(1, "Thies C. Arntzen, Stig Bakken, Andi Gutmans, Rasmus Lerdorf, Sam Ruby,\
					Sascha Schumann, Zeev Suraski, Jim Winstead, Andrei Zmievski");
		php_info_print_table_end();
	}

	if (flag & PHP_CREDITS_GENERAL) {
		/* Design & Concept */
		php_info_print_table_start();
		php_info_print_table_header(1, "Language Design & Concept");
		php_info_print_table_row(1, "Andi Gutmans, Rasmus Lerdorf, Zeev Suraski");
		php_info_print_table_end();

		/* PHP 4.0 Language */
		php_info_print_table_start();
		php_info_print_table_colspan_header(2, "PHP 4.0 Authors");
		php_info_print_table_header(2, "Contribution", "Authors");
		CREDIT_LINE("Zend Scripting Language Engine", "Andi Gutmans, Zeev Suraski");
		CREDIT_LINE("Extension Module API", "Andi Gutmans, Zeev Suraski");
		CREDIT_LINE("UNIX Build and Modularization", "Stig Bakken, Sascha Schumann");
		CREDIT_LINE("Win32 Port", "Shane Caraveo, Zeev Suraski");
		CREDIT_LINE("Server API (SAPI) Abstraction Layer", "Andi Gutmans, Shane Caraveo, Zeev Suraski");
		php_info_print_table_end();
	}

	if (flag & PHP_CREDITS_GENERAL) {
		/* SAPI Modules */

		php_info_print_table_start();
		php_info_print_table_colspan_header(2, "SAPI Module");
		php_info_print_table_header(2, "Contribution", "Authors");
		CREDIT_LINE("Apache", "Rasmus Lerdorf, Zeev Suraski");
		CREDIT_LINE("ISAPI", "Andi Gutmans, Zeev Suraski");
		CREDIT_LINE("CGI", "Rasmus Lerdorf, Stig Bakken");
		CREDIT_LINE("AOLserver", "Sascha Schumann");
		CREDIT_LINE("Java Servlet", "Sam Ruby");
		CREDIT_LINE("Roxen", "David Hedbor");
		CREDIT_LINE("thttpd", "Sascha Schumann");
		php_info_print_table_end();
	}

	if (flag & PHP_CREDITS_MODULES) {
		/* Modules */

		php_info_print_table_start();
		php_info_print_table_colspan_header(2, "Module Authors");
		php_info_print_table_header(2, "Module", "Authors");
		CREDIT_LINE("Apache", "Rasmus Lerdorf, Stig Bakken, David Sklar");
		CREDIT_LINE("Assert", "Thies C. Arntzen");
		CREDIT_LINE("BC Math", "Andi Gutmans");
		CREDIT_LINE("CyberCash", "Evan Klinger");
		CREDIT_LINE("Win32 COM", "Zeev Suraski");
		CREDIT_LINE("DAV", "Stig Bakken");
		CREDIT_LINE("DBA", "Sascha Schumann");
		CREDIT_LINE("DBM", "Rasmus Lerdorf, Jim Winstead");
		CREDIT_LINE("dBase", "Jim Winstead");
		CREDIT_LINE("FDF", "Uwe Steinmann");
		CREDIT_LINE("FilePro", "Chad Robinson");
		CREDIT_LINE("FTP", "Andrew Skalski");
		CREDIT_LINE("GD imaging", "Rasmus Lerdorf, Stig Bakken, Jim Winstead, Jouni Ahto");
		CREDIT_LINE("GetText", "Alex Plotnick");
		CREDIT_LINE("HyperWave", "Uwe Steinmann");
		CREDIT_LINE("IMAP", "Rex Logan, Mark Musone, Brian Wang, Kaj-Michael Lang, Antoni Pamies Olive, Rasmus Lerdorf, Andrew Skalski, Chuck Hagenbuch");
		CREDIT_LINE("Informix", "Danny Heijl, Christian Cartus");
		CREDIT_LINE("Java", "Sam Ruby");
		CREDIT_LINE("InterBase", "Jouni Ahto, Andrew Avdeev");
		CREDIT_LINE("LDAP", "Amitay Isaacs, Eric Warnke, Rasmus Lerdorf, Gerrit Thomson");
		CREDIT_LINE("MCAL", "Mark Musone, Chuck Hagenbuch");
		CREDIT_LINE("mcrypt", "Sascha Schumann");
		CREDIT_LINE("mhash", "Sascha Schumann");
		CREDIT_LINE("MS SQL", "Frank M. Kromann");
		CREDIT_LINE("mSQL", "Zeev Suraski");
		CREDIT_LINE("MySQL", "Zeev Suraski");
		CREDIT_LINE("OCI8", "Stig Bakken, Thies C. Arntzen");
		CREDIT_LINE("ODBC", "Stig Bakken, Andreas Karajannis, Frank M. Kromann");
		CREDIT_LINE("Oracle", "Stig Bakken, Mitch Golden, Rasmus Lerdorf, Andreas Karajannis, Thies C. Arntzen");
		CREDIT_LINE("Perl Compatible Regexps", "Andrei Zmievski");
		CREDIT_LINE("PDF", "Uwe Steinmann");
		CREDIT_LINE("Posix", "Kristian K�hntopp");
		CREDIT_LINE("PostgreSQL", "Jouni Ahto, Zeev Suraski");
		CREDIT_LINE("Readline", "Thies C. Arntzen");
		CREDIT_LINE("Recode", "Kristian K�hntopp");
		CREDIT_LINE("Sessions", "Sascha Schumann, Andrei Zmievski");
		CREDIT_LINE("SNMP", "Rasmus Lerdorf");
		CREDIT_LINE("SWF", "Sterling Hughes");
		CREDIT_LINE("Sybase", "Zeev Suraski");
		CREDIT_LINE("System V Shared Memory", "Christian Cartus");
		CREDIT_LINE("System V Semaphores", "Tom May");
		CREDIT_LINE("WDDX", "Andrei Zmievski");
		CREDIT_LINE("XML", "Stig Bakken, Thies C. Arntzen");
		CREDIT_LINE("Yellow Pages", "Stephanie Wehner");
		CREDIT_LINE("Zlib", "Rasmus Lerdorf, Stefan Roehrich");
		php_info_print_table_end();
	}

	if (flag & PHP_CREDITS_DOCS) {
		php_info_print_table_start();
		php_info_print_table_header(1, "PHP Documentation Team");
		php_info_print_table_row(1, "Alexander Aulbach, Stig Bakken, Rasmus Lerdorf, Egon Schmid, Zeev Suraski, Lars Torben Wilson, Jim Winstead");
		php_info_print_table_row(1, "Edited by:  Stig Bakken and Egon Schmid");
		php_info_print_table_end();
	}

	PUTS("</center>");

	if (flag & PHP_CREDITS_FULLPAGE) {
		PUTS("</body></html>\n");
	}
}

PHPAPI void php_info_print_table_start()
{
	php_printf("<TABLE BORDER=0 CELLPADDING=3 CELLSPACING=1 WIDTH=600 BGCOLOR=\"#000000\">\n");
}

PHPAPI void php_info_print_table_end()
{
	php_printf("</TABLE><BR>\n");

}

PHPAPI void php_info_print_box_start(int flag)
{
	php_info_print_table_start();
	if (flag) {
		php_printf("<TR VALIGN=\"middle\" BGCOLOR=\"" PHP_HEADER_COLOR "\"><TD ALIGN=\"left\">\n");
	} else {
		php_printf("<TR VALIGN=\"top\" BGCOLOR=\"" PHP_CONTENTS_COLOR "\"><TD ALIGN=\"left\">\n");
	}
}

PHPAPI void php_info_print_box_end()
{
	php_printf("</TD></TR>\n");
	php_info_print_table_end();
}

PHPAPI void php_info_print_hr()
{
	php_printf("<HR NOSHADE SIZE=1 WIDTH=600>\n");
}

PHPAPI void php_info_print_table_colspan_header(int num_cols, char *header)
{
	php_printf("<TR BGCOLOR=\"" PHP_HEADER_COLOR "\"><TH COLSPAN=%d>%s</TH></TR>\n", num_cols, header );
}


PHPAPI void php_info_print_table_header(int num_cols, ...)
{
	int i;
	va_list row_elements;
	char *row_element;

	va_start(row_elements, num_cols);

	php_printf("<TR VALIGN=\"bottom\" bgcolor=\"" PHP_HEADER_COLOR "\">");
	for (i=0; i<num_cols; i++) {
		row_element = va_arg(row_elements, char *);
		if (!row_element || !*row_element) {
			row_element = "&nbsp;";
		}
		php_printf("<TH>%s</TH>", row_element);
	}
	php_printf("</TR>\n");

	va_end(row_elements);
}


PHPAPI void php_info_print_table_row(int num_cols, ...)
{
	int i;
	va_list row_elements;
	char *row_element;

	va_start(row_elements, num_cols);

	php_printf("<TR VALIGN=\"baseline\" BGCOLOR=\"" PHP_CONTENTS_COLOR "\">");
	for (i=0; i<num_cols; i++) {
		row_element = va_arg(row_elements, char *);
		if (!row_element || !*row_element) {
			row_element = "&nbsp;";
		}
		php_printf("<TD %s>%s%s%s</td>", 
			(i==0?"BGCOLOR=\"" PHP_ENTRY_NAME_COLOR "\" ":"ALIGN=\"left\""),
			(i==0?"<B>":""), 
			row_element,
			(i==0?"</B>":""));
	}
	php_printf("</TR>\n");

	va_end(row_elements);
}



void register_phpinfo_constants(INIT_FUNC_ARGS)
{
	REGISTER_LONG_CONSTANT("INFO_GENERAL", PHP_INFO_GENERAL, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_CREDITS", PHP_INFO_CREDITS, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_CONFIGURATION", PHP_INFO_CONFIGURATION, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_MODULES", PHP_INFO_MODULES, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_ENVIRONMENT", PHP_INFO_ENVIRONMENT, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_VARIABLES", PHP_INFO_VARIABLES, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_LICENSE", PHP_INFO_LICENSE, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("INFO_ALL", PHP_INFO_ALL, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_GROUP",	PHP_CREDITS_GROUP, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_GENERAL",	PHP_CREDITS_GENERAL, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_SAPI",	PHP_CREDITS_SAPI, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_MODULES",	PHP_CREDITS_MODULES, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_DOCS",	PHP_CREDITS_DOCS, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_FULLPAGE",	PHP_CREDITS_FULLPAGE, CONST_PERSISTENT|CONST_CS);
	REGISTER_LONG_CONSTANT("CREDITS_ALL",	PHP_CREDITS_ALL, CONST_PERSISTENT|CONST_CS);
}


/* {{{ proto void phpinfo([int what])
   Output a page of useful information about PHP and the current request */
PHP_FUNCTION(phpinfo)
{
	int flag;
	zval **flag_arg;


	switch (ZEND_NUM_ARGS()) {
		case 0:
			flag = 0xFFFFFFFF;
			break;
		case 1:
			if (zend_get_parameters_ex(1, &flag_arg)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(flag_arg);
			flag = (*flag_arg)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	php_print_info(flag);
	RETURN_TRUE;
}

/* }}} */

/* {{{ proto string phpversion(void)
   Return the current PHP version */
PHP_FUNCTION(phpversion)
{
    RETURN_STRING(PHP_VERSION,1);
}
/* }}} */


/* {{{ proto void phpcredits(int)
   Prints the list of people who've contributed to the PHP project */
PHP_FUNCTION(phpcredits)
{
	int flag;
	zval **flag_arg;


	switch (ZEND_NUM_ARGS()) {
		case 0:
			flag = 0xFFFFFFFF;
			break;
		case 1:
			if (zend_get_parameters_ex(1, &flag_arg)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(flag_arg);
			flag = (*flag_arg)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	php_print_credits(flag);
	RETURN_TRUE;
}

/* }}} */


PHP_FUNCTION(php_logo_guid)
{
	RETURN_STRINGL(PHP_LOGO_GUID, sizeof(PHP_LOGO_GUID)-1, 1);
}

PHP_FUNCTION(php_egg_logo_guid)
{
	RETURN_STRINGL(PHP_EGG_LOGO_GUID, sizeof(PHP_EGG_LOGO_GUID)-1, 1);
}


PHP_FUNCTION(zend_logo_guid)
{
	RETURN_STRINGL(ZEND_LOGO_GUID, sizeof(ZEND_LOGO_GUID)-1, 1);
}

/* {{{ proto string sapi_module_name(void)
   Return the current SAPI module name */
PHP_FUNCTION(php_sapi_name)
{
	if (sapi_module.name) {
		RETURN_STRING(sapi_module.name,1);
	} else {
		RETURN_FALSE;
	}
}

/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
