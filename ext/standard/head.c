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
   | Authors: Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   +----------------------------------------------------------------------+
 */
/* $Id$ */

#include <stdio.h>
#include "php.h"
#include "ext/standard/php3_standard.h"
#include "SAPI.h"
#include "main.h"
#include "head.h"
#include "post.h"
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif

#include "php_globals.h"
#include "safe_mode.h"


/* need to figure out some nice way to get rid of these */
#ifndef THREAD_SAFE
static int php3_HeaderPrinted = 0;
static int php3_PrintHeader = 1;
static CookieList *top = NULL;
static char *cont_type = NULL;
static int header_called = 0;
#endif

void php3_PushCookieList(char *, char *, time_t, char *, char *, int);
CookieList *php3_PopCookieList(void);

int php3_init_head(INIT_FUNC_ARGS)
{
	php3_HeaderPrinted = 0;
	if (header_called == 0)
		php3_PrintHeader = 1;
	top = NULL;
	cont_type = NULL;

	return SUCCESS;
}


#if 0
/* Adds header information */
void php4i_add_header_information(char *header_information, uint header_length)
{
	char *r;
#if APACHE
	char *rr = NULL;
	char *temp = NULL;
	long myuid = 0L;
	char temp2[32];
	request_rec *req;
	SLS_FETCH();
	
	req = ((request_rec *) SG(server_context));
#endif

	if (php3_HeaderPrinted == 1) {
#if DEBUG
		php3_error(E_WARNING, "Cannot add more header information - the header was already sent "
							  "(header information may be added only before any output is generated from the script - "
							  "check for text or whitespace outside PHP tags, or calls to functions that output text)");
#endif
		return;					/* too late, already sent */
	}
#if APACHE
	/*
	 * Not entirely sure this is the right way to support the header
	 * command in the Apache module.  Comments?
	 */
	r = strchr(header_information, ':');
	if (r) {
		*r = '\0';
		if (!strcasecmp(header_information, "Content-type")) {
			if (*(r + 1) == ' ')
				req->content_type = pstrdup(req->pool,r + 2);
			else
				req->content_type = pstrdup(req->pool,r + 1);
			cont_type = (char *) req->content_type;
		} else {
			if (*(r + 1) == ' ') {
				rr = r + 2;
			} else {
				rr = r + 1;
			}
			if (PG(safe_mode) && (!strcasecmp(header_information, "WWW-authenticate"))) {
				myuid = _php3_getuid();
				sprintf(temp2, "realm=\"%ld ", myuid);  /* SAFE */
				temp = _php3_regreplace("realm=\"", temp2, rr, 1, 0);
				if (!strcmp(temp, rr)) {
					sprintf(temp2, "realm=%ld", myuid);  /* SAFE */
					temp = _php3_regreplace("realm=", temp2, rr, 1, 0);
					if (!strcmp(temp, rr)) {
						sprintf(temp2, " realm=%ld", myuid);  /* SAFE */
						temp = _php3_regreplace("$", temp2, rr, 0, 0);
					}
				}
				table_set(req->headers_out, header_information, temp);
			} else
				table_set(req->headers_out, header_information, rr);
		}
		if (!strcasecmp(header_information, "location")) {
			req->status = REDIRECT;
		}
		*r = ':';
		php3_HeaderPrinted = 2;
	}
	if (!strncasecmp(header_information, "http/", 5)) {
		if (strlen(header_information) > 9) {
			req->status = atoi(&((header_information)[9]));
		}
		/* Use a pstrdup here to get the memory straight from Apache's per-request pool to
		 * avoid having our own memory manager complain about this memory not being freed
		 * because it really shouldn't be freed until the end of the request and it isn't
		 * easy for us to figure out when we allocated it vs. when something else might have.
		 */
		req->status_line = pstrdup(req->pool,&((header_information)[9]));
	}
#else
	r = strchr(header_information, ':');
	if (r) {
		*r = '\0';
		if (!strcasecmp(header_information, "Content-type")) {
			if (cont_type) efree(cont_type);
			cont_type = estrdup(r + 1);
#if 0 /*WIN32|WINNT / *M$ does us again*/
			if (!strcmp(cont_type," text/html")){
				*r=':';
				PUTS_H(header_information);
				PUTS_H("\015\012");
			}
#endif
		} else {
			*r = ':';
#if USE_SAPI
			{
				char *tempstr=emalloc(strlen(header_information)+2);
				
				sprintf(tempstr,"%s\015\012",tempstr);
				sapi_rqst->header(sapi_rqst->scid,tempstr);
				efree(tempstr);
			}
#elif FHTTPD
            php3_fhttpd_puts_header(header_information);
            php3_fhttpd_puts_header("\r\n");
#else
			PUTS_H(header_information);
			PUTS_H("\015\012");
#endif
		}
	} else {
#if USE_SAPI
		{
		char *tempstr=emalloc(strlen(header_information)+2);
		sprintf(tempstr,"%s\015\012",tempstr);
		sapi_rqst->header(sapi_rqst->scid,tempstr);
		efree(tempstr);
		}
#elif FHTTPD
        php3_fhttpd_puts_header(header_information);
        php3_fhttpd_puts_header("\r\n");
#else
		PUTS_H(header_information);
		PUTS_H("\015\012");
#endif
	}
#endif
}
#else
void php4i_add_header_information(char *header_information, uint header_length)
{
	sapi_add_header(header_information, header_length);
}
#endif


/* Implementation of the language Header() function */
PHP_FUNCTION(Header)
{
	pval *arg1;

	if (getParameters(ht, 1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string(arg1);
	php4i_add_header_information(arg1->value.str.val, arg1->value.str.len);
	arg1->type = IS_LONG; /* change arg1's type so that it doesn't get freed */
	arg1->value.lval = 0;
}





#if 0
/*
 * php3_header() flushes the header info built up using calls to
 * the Header() function.  If type is 1, a redirect to str is done.
 * Otherwise type should be 0 and str NULL.
 *
 * The function returns non-zero if output is allowed after the
 * call, and zero otherwise.  Any call to php3_header() must check
 * the return status and if false, no output must be sent.  This
 * is in order to correctly handle HEAD requests.
 */
PHPAPI int php3_header(void)
{
#if APACHE
	CookieList *cookie;
	int len = 0;
	time_t t;
	char *dt, *cookievalue = NULL;
#endif
#if APACHE || defined(USE_SAPI) || FHTTPD
	char *tempstr;
#endif
	PLS_FETCH();
	SLS_FETCH();

	if (PG(header_is_being_sent)) {
		return 0;
	} else {
		PG(header_is_being_sent) = 1;
	}

#if APACHE
	if (!((request_rec *) SG(server_context))) {  /* we're not in a request, allow output */
		PG(header_is_being_sent) = 0;
		return 1;
	}
	if ((php3_PrintHeader && !php3_HeaderPrinted) || (php3_PrintHeader && php3_HeaderPrinted == 2)) {
		cookie = php3_PopCookieList();
		while (cookie) {
			if (cookie->name)
				len += strlen(cookie->name);
                      if (cookie->value) {
                              cookievalue = _php3_urlencode(cookie->value, strlen (cookie->value));
                              len += strlen(cookievalue);
                      }
			if (cookie->path)
				len += strlen(cookie->path);
			if (cookie->domain)
				len += strlen(cookie->domain);
			tempstr = emalloc(len + 100);
			if (!cookie->value || (cookie->value && !*cookie->value)) {
				/* 
				 * MSIE doesn't delete a cookie when you set it to a null value
				 * so in order to force cookies to be deleted, even on MSIE, we
				 * pick an expiry date 1 year and 1 second in the past
				 */
				sprintf(tempstr, "%s=deleted", cookie->name);
				t = time(NULL) - 31536001;
				strcat(tempstr, "; expires=");
				dt = php3_std_date(t);
				strcat(tempstr, dt);
				efree(dt);
			} else {
				/* FIXME: XXX: this is not binary data safe */
                              sprintf(tempstr, "%s=%s", cookie->name, cookie->value ? cookievalue : "");
				if (cookie->name) efree(cookie->name);
				if (cookie->value) efree(cookie->value);
                              if (cookievalue) efree(cookievalue);
				cookie->name=NULL;
				cookie->value=NULL;
                              cookievalue=NULL;
				if (cookie->expires > 0) {
					strcat(tempstr, "; expires=");
					dt = php3_std_date(cookie->expires);
					strcat(tempstr, dt);
					efree(dt);
				}
			}
			if (cookie->path && strlen(cookie->path)) {
				strcat(tempstr, "; path=");
				strcat(tempstr, cookie->path);
				efree(cookie->path);
				cookie->path=NULL;
			}
			if (cookie->domain && strlen(cookie->domain)) {
				strcat(tempstr, "; domain=");
				strcat(tempstr, cookie->domain);
				efree(cookie->domain);
				cookie->domain=NULL;
			}
			if (cookie->secure) {
				strcat(tempstr, "; secure");
			}
			table_add(((request_rec *) SG(server_context))->headers_out, "Set-Cookie", tempstr);
			if (cookie->domain) efree(cookie->domain);
			if (cookie->path) efree(cookie->path);
			if (cookie->name) efree(cookie->name);
			if (cookie->value) efree(cookie->value);
                      if (cookievalue) efree(cookievalue);
			efree(cookie);
			cookie = php3_PopCookieList();
			efree(tempstr);
		}
		php3_HeaderPrinted = 1;
		header_called = 1;
		send_http_header(((request_rec *) SG(server_context)));
		if (((request_rec *) SG(server_context))->header_only) {
			set_header_request(1);
			PG(header_is_being_sent) = 0;
			return(0);
		}
	}
#else
	if (php3_PrintHeader && !php3_HeaderPrinted) {
		if (!cont_type) {
#if USE_SAPI
			sapi_rqst->header(sapi_rqst->scid,"Content-type: text/html\015\012\015\012");
#elif FHTTPD
			php3_fhttpd_puts_header("Content-type: text/html\r\n");
#else
			PUTS_H("Content-type: text/html\015\012\015\012");
#endif
		} else {
#if 0 /*WIN32|WINNT / *M$ does us again*/
			if (!strcmp(cont_type,"text/html")){
#endif
#if USE_SAPI
			tempstr=emalloc(strlen(cont_type)+18);
			sprintf(tempstr,"Content-type: %s\015\012\015\012",cont_type);
			sapi_rqst->header(sapi_rqst->scid,tempstr);
			efree(tempstr);
#elif FHTTPD
			tempstr = emalloc(strlen(cont_type)
							  + sizeof("Content-type:") + 2);
			if(tempstr) {
				strcpy(tempstr, "Content-type:");
				strcpy(tempstr + sizeof("Content-type:") - 1,
					   cont_type);
				strcat(tempstr, "\r\n");
				php3_fhttpd_puts_header(tempstr);
				efree(tempstr);
			}
#else
			PUTS_H("Content-type:");
			PUTS_H(cont_type);
			PUTS_H("\015\012\015\012");
#endif
			efree(cont_type);
#if 0 /*WIN32|WINNT / *M$ does us again*/
			} else {
				PUTS_H("\015\012");
			}/*end excluding output of text/html*/
#endif
		}
#if USE_SAPI
		sapi_rqst->flush(sapi_rqst->scid);
#else
		fflush(stdout);
#endif
		php3_HeaderPrinted = 1;
		header_called = 1;
	}
#endif
	PG(header_is_being_sent) = 0;
	return(1);
}
#else
PHPAPI int php3_header()
{
	SLS_FETCH();

	if (sapi_send_headers()==FAILURE || SG(request_info).headers_only) {
		return 0; /* don't allow output */
	} else {
		return 1; /* allow output */
	}
}
#endif



void php3_PushCookieList(char *name, char *value, time_t expires, char *path, char *domain, int secure)
{
	CookieList *new;

	new = emalloc(sizeof(CookieList));
	new->next = top;
	new->name = name;
	new->value = value;
	new->expires = expires;
	new->path = path;
	new->domain = domain;
	new->secure = secure;
	top = new;
}

CookieList *php3_PopCookieList(void)
{
	CookieList *ret;

	ret = top;
	if (top)
		top = top->next;
	return (ret);
}

/* php3_SetCookie(name,value,expires,path,domain,secure) */
PHP_FUNCTION(SetCookie)
{
	char *cookie;
	int len=sizeof("Set-Cookie: ");
	time_t t;
	char *r, *dt;
	char *name = NULL, *value = NULL, *path = NULL, *domain = NULL;
	time_t expires = 0;
	int secure = 0;
	pval *arg[6];
	int arg_count;

	arg_count = ARG_COUNT(ht);
	if (arg_count < 1 || arg_count > 6 || getParametersArray(ht, arg_count, arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	if (php3_HeaderPrinted == 1) {
		php3_error(E_WARNING, "Oops, php3_SetCookie called after header has been sent\n");
		return;
	}
	switch (arg_count) {
		case 6:
			convert_to_boolean(arg[5]);
			secure = arg[5]->value.lval;
			/* break missing intentionally */
		case 5:
			convert_to_string(arg[4]);
			domain = estrndup(arg[4]->value.str.val,arg[4]->value.str.len);
			/* break missing intentionally */
		case 4:
			convert_to_string(arg[3]);
			path = estrndup(arg[3]->value.str.val,arg[3]->value.str.len);
			/* break missing intentionally */
		case 3:
			convert_to_long(arg[2]);
			expires = arg[2]->value.lval;
			/* break missing intentionally */
		case 2:
			convert_to_string(arg[1]);
			value = estrndup(arg[1]->value.str.val,arg[1]->value.str.len);
			/* break missing intentionally */
		case 1:
			convert_to_string(arg[0]);
			name = estrndup(arg[0]->value.str.val,arg[0]->value.str.len);
			break;
	}
#if 0
	php3_PushCookieList(name, value, expires, path, domain, secure);
#else
	if (name) {
		len += strlen(name);
	}
	if (value) {
		len += strlen(value);
	}
	if (path) {
		len += strlen(path);
	}
	if (domain) {
		len += strlen(domain);
	}
	cookie = emalloc(len + 100);
	if (!value || (value && !*value)) {
		/* 
		 * MSIE doesn't delete a cookie when you set it to a null value
		 * so in order to force cookies to be deleted, even on MSIE, we
		 * pick an expiry date 1 year and 1 second in the past
		 */
		sprintf(cookie, "Set-Cookie: %s=deleted", name);
		strcat(cookie, "; expires=");
		t = time(NULL) - 31536001;
		dt = php3_std_date(t);
		strcat(cookie, dt);
		efree(dt);
	} else {
		/* FIXME: XXX: this is not binary data safe */
		r = _php3_urlencode(value, strlen (value));
		sprintf(cookie, "Set-Cookie: %s=%s", name, value ? r : "");
		if (r) efree(r);
		if (value) efree(value);
		value=NULL;
		if (name) efree(name);
		name=NULL;
		if (expires > 0) {
			strcat(cookie, "; expires=");
			dt = php3_std_date(expires);
			strcat(cookie, dt);
			efree(dt);
		}
	}
	if (path && strlen(path)) {
		strcat(cookie, "; path=");
		strcat(cookie, path);
		efree(path);
		path=NULL;
	}
	if (domain && strlen(domain)) {
		strcat(cookie, "; domain=");
		strcat(cookie, domain);
		efree(domain);
		domain=NULL;
	}
	if (secure) {
		strcat(cookie, "; secure");
	}


	if (sapi_add_header(cookie, strlen(cookie))==SUCCESS) {
		RETVAL_TRUE;
	} else {
		RETVAL_FALSE;
	}

	if (domain) {
		efree(domain);
	}
	if (path) {
		efree(path);
	}
	if (name) {
		efree(name);
	}
	if (value) {
		efree(value);
	}
#endif
}


int php3_headers_unsent(void)
{
	if (php3_HeaderPrinted!=1 || !php3_PrintHeader) {
		return 1;
	} else {
		return 0;
	}
}


function_entry php3_header_functions[] = {
	{NULL, NULL, NULL}
};


php3_module_entry php3_header_module_entry = {
	"PHP_head", php3_header_functions, NULL, NULL, php3_init_head, NULL, NULL, STANDARD_MODULE_PROPERTIES
};

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
