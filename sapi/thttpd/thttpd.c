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
   | Author: Sascha Schumann <sascha@schumann.cx>                         |
   +----------------------------------------------------------------------+
*/


#include "php.h"
#include "SAPI.h"
#include "main.h"
#include "php_thttpd.h"
#include "version.h"

typedef struct {
	httpd_conn *hc;
	int post_off;
} php_thttpd_globals;

static php_thttpd_globals thttpd_globals;

#define TLS_D
#define TLS_DC
#define TLS_C
#define TLS_CC
#define TG(v) (thttpd_globals.v)
#define TLS_FETCH()

static int sapi_thttpd_ub_write(const char *str, uint str_length)
{
	TLS_FETCH();
	
	return send(TG(hc)->conn_fd, str, str_length, 0);
}

static int sapi_thttpd_send_headers(sapi_headers_struct *sapi_headers SLS_DC)
{
	char buf[1024];

	if (!SG(sapi_headers).http_status_line) {
		snprintf(buf, 1023, "HTTP/1.0 %d Something\r\n", SG(sapi_headers).http_response_code);
		send(TG(hc)->conn_fd, buf, strlen(buf), 0);
	}
	
	return SAPI_HEADER_DO_SEND;
}

static void sapi_thttpd_send_header(sapi_header_struct *sapi_header, void *server_context)
{
	TLS_FETCH();

	if (sapi_header)
		send(TG(hc)->conn_fd, sapi_header->header, sapi_header->header_len, 0);
	send(TG(hc)->conn_fd, "\r\n", sizeof("\r\n") - 1, 0);
}

static int sapi_thttpd_read_post(char *buffer, uint count_bytes SLS_DC)
{
	size_t read_bytes = 0, tmp;
	int c;
	TLS_FETCH();

	/* to understand this, read cgi_interpose_input() in libhttpd.c */
	c = TG(hc)->read_idx - TG(hc)->checked_idx;
	if (c > 0) {
		read_bytes = MIN(c, count_bytes);
		memcpy(buffer, TG(hc)->read_buf + TG(hc)->checked_idx, read_bytes);
		TG(hc)->checked_idx += read_bytes;
		count_bytes -= read_bytes;
	}
	
	count_bytes = MIN(count_bytes, 
			SG(request_info).content_length - SG(read_post_bytes) - TG(post_off));

	while (read_bytes < count_bytes) {
		tmp = recv(TG(hc)->conn_fd, buffer + read_bytes, 
				count_bytes - read_bytes, 0);
		if (tmp <= 0) 
			break;
		read_bytes += tmp;
	}
	
	return read_bytes;
}

static char *sapi_thttpd_read_cookies(SLS_D)
{
	TLS_FETCH();
	
	return TG(hc)->cookie;
}

static sapi_module_struct sapi_module = {
	"thttpd",
	
	php_module_startup,
	php_module_shutdown_wrapper,
	
	NULL,									/* activate */
	NULL,									/* deactivate */

	sapi_thttpd_ub_write,
	NULL,
	NULL,									/* get uid */
	NULL,									/* getenv */

	php_error,
	
	NULL,
	sapi_thttpd_send_headers,
	sapi_thttpd_send_header,
	sapi_thttpd_read_post,
	sapi_thttpd_read_cookies,

	NULL,									/* register server variables */
	NULL,									/* Log message */

	NULL,									/* Block interruptions */
	NULL,									/* Unblock interruptions */

	STANDARD_SAPI_MODULE_PROPERTIES
};

#define BUF_SIZE 512
#define ADD_STRING(name)										\
	MAKE_STD_ZVAL(pval);										\
	pval->type = IS_STRING;										\
	pval->value.str.len = strlen(buf);							\
	pval->value.str.val = estrndup(buf, pval->value.str.len);	\
	zend_hash_update(&EG(symbol_table), name, sizeof(name), 	\
			&pval, sizeof(zval *), NULL)

static void thttpd_hash_environment(void)
{
	char buf[BUF_SIZE + 1];
	zval *pval;

	buf[BUF_SIZE] = '\0';
	
	strncpy(buf, SERVER_SOFTWARE, BUF_SIZE);
	ADD_STRING("SERVER_SOFTWARE");

	strncpy(buf, "CGI/1.1", BUF_SIZE);
	ADD_STRING("GATEWAY_INTERFACE");

	snprintf(buf, BUF_SIZE, "%d", TG(hc)->hs->port);
	ADD_STRING("SERVER_PORT");

	strncpy(buf, SG(request_info).request_method, BUF_SIZE);
	ADD_STRING("REQUEST_METHOD");

	strncpy(buf, SG(request_info).request_uri, BUF_SIZE);
	ADD_STRING("REQUEST_URI");

	snprintf(buf, BUF_SIZE, "/%s", TG(hc)->pathinfo);
	ADD_STRING("PATH_INFO");
	
	strncpy(buf, SG(request_info).path_translated, BUF_SIZE);
	ADD_STRING("PATH_TRANSLATED");

	snprintf(buf, BUF_SIZE, "/%s", TG(hc)->origfilename);
	ADD_STRING("SCRIPT_NAME");

	strncpy(buf, inet_ntoa(TG(hc)->client_addr.sa_in.sin_addr), BUF_SIZE);
	ADD_STRING("REMOTE_ADDR");
	ADD_STRING("REMOTE_HOST");

#define CONDADD(name, field) 							\
	if (TG(hc)->field[0]) {								\
		strncpy(buf, TG(hc)->field, BUF_SIZE);			\
		ADD_STRING(#name);								\
	}

	CONDADD(HTTP_REFERER, referer);
	CONDADD(HTTP_USER_AGENT, useragent);
	CONDADD(HTTP_ACCEPT, accept);
	CONDADD(HTTP_ACCEPT_ENCODING, accepte);
	CONDADD(HTTP_COOKIE, cookie);
	CONDADD(CONTENT_TYPE, contenttype);
	CONDADD(REMOTE_USER, remoteuser);
	CONDADD(SERVER_PROTOCOL, protocol);

	if (TG(hc)->contentlength != -1) {
		sprintf(buf, "%ld", (long) TG(hc)->contentlength);
		ADD_STRING("CONTENT_LENGTH");
	}

	if (TG(hc)->authorization[0]) {
		strcpy(buf, "Basic");
		ADD_STRING("AUTH_TYPE");
	}
}

static void thttpd_module_main(TLS_D SLS_DC)
{
	zend_file_handle file_handle;
	CLS_FETCH();
	ELS_FETCH();
	PLS_FETCH();

	file_handle.type = ZEND_HANDLE_FILENAME;
	file_handle.filename = TG(hc)->expnfilename;
	file_handle.free_filename = 0;

	if (php_request_startup(CLS_C ELS_CC PLS_CC SLS_CC) == FAILURE) {
		return;
	}
	
	thttpd_hash_environment();
	php_execute_script(&file_handle CLS_CC ELS_CC PLS_CC);
	php_request_shutdown(NULL);
}

static void thttpd_request_ctor(TLS_D SLS_DC)
{
	char *cp2;
	int l;
	char buf[1024];
	int offset;
	size_t pathinfo_len;
	size_t cwd_len;
	
	pathinfo_len = strlen(TG(hc)->pathinfo);
	cwd_len = strlen(TG(hc)->hs->cwd);

	SG(request_info).query_string = TG(hc)->query;
	
	l = cwd_len + pathinfo_len + 1;
	cp2 = (char *) malloc(l);
	sprintf(cp2, "%s%s", TG(hc)->hs->cwd, TG(hc)->pathinfo);
	SG(request_info).path_translated = cp2;
	
	snprintf(buf, 1023, "/%s", TG(hc)->origfilename);
	SG(request_info).request_uri = strdup(buf);
	SG(request_info).request_method = httpd_method_str(TG(hc)->method);

	SG(request_info).content_type = TG(hc)->contenttype;
	SG(request_info).content_length = TG(hc)->contentlength;
	
	SG(request_info).auth_user = NULL;
	SG(request_info).auth_password = NULL;

	TG(post_off) = TG(hc)->read_idx - TG(hc)->checked_idx;

	/* avoid feeding \r\n from POST data to SAPI */
	offset = TG(post_off) - SG(request_info).content_length;

	if (offset > 0) {
		TG(post_off) -= offset;
		TG(hc)->read_idx -= offset;
	}
}

static void thttpd_request_dtor(TLS_D SLS_DC)
{
	free(SG(request_info).request_uri);
	free(SG(request_info).path_translated);
}

off_t thttpd_php_request(httpd_conn *hc)
{
	SLS_FETCH();
	TLS_FETCH();

	TG(hc) = hc;
	
	thttpd_request_ctor(TLS_C SLS_CC);

	thttpd_module_main(TLS_C SLS_CC);

	thttpd_request_dtor(TLS_C SLS_CC);

	return 0;
}

void thttpd_php_init(void)
{
	sapi_startup(&sapi_module);
	sapi_module.startup(&sapi_module);
	SG(server_context) = (void *) 1;
}

void thttpd_php_shutdown(void)
{
	sapi_module.shutdown(&sapi_module);
	sapi_shutdown();
}
