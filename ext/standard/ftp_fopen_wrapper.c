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
   | Authors: Rasmus Lerdorf <rasmus@php.net>                             |
   |          Jim Winstead <jimw@php.net>                                 |
   |          Hartmut Holzgraefe <hholzgra@php.net>                       |
   +----------------------------------------------------------------------+
 */
/* $Id$ */

#include "php.h"
#include "php_globals.h"
#include "php_network.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef PHP_WIN32
#include <windows.h>
#include <winsock.h>
#define O_RDONLY _O_RDONLY
#include "win32/param.h"
#else
#include <sys/param.h>
#endif

#include "php_standard.h"

#include <sys/types.h>
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef PHP_WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#endif

#if defined(PHP_WIN32) || defined(__riscos__)
#undef AF_UNIX
#endif

#if defined(AF_UNIX)
#include <sys/un.h>
#endif

#include "php_fopen_wrappers.h"


static inline int get_ftp_result(php_stream *stream, char *buffer, size_t buffer_size TSRMLS_DC)
{
	while (php_stream_gets(stream, buffer, buffer_size-1) &&
		   !(isdigit((int) buffer[0]) && isdigit((int) buffer[1]) &&
			 isdigit((int) buffer[2]) && buffer[3] == ' '));

	return strtol(buffer, NULL, 10);
}
#define GET_FTP_RESULT(stream)	get_ftp_result((stream), tmp_line, sizeof(tmp_line) TSRMLS_CC)

static int php_stream_ftp_stream_stat(php_stream_wrapper *wrapper,
		php_stream *stream,
		php_stream_statbuf *ssb
		TSRMLS_DC)
{
	/* For now, we return with a failure code to prevent the underlying
	 * file's details from being used instead. */
	return -1;
}


static php_stream_wrapper_ops ftp_stream_wops = {
	php_stream_url_wrap_ftp,
	NULL,
	php_stream_ftp_stream_stat,
	NULL
};

php_stream_wrapper php_stream_ftp_wrapper =	{
	&ftp_stream_wops,
	NULL,
	1 /* is_url */
};


/* {{{ php_fopen_url_wrap_ftp
 */
php_stream * php_stream_url_wrap_ftp(php_stream_wrapper *wrapper, char *path, char *mode, int options, char **opened_path, php_stream_context *context STREAMS_DC TSRMLS_DC)
{
	php_stream *stream=NULL;
	php_url *resource=NULL;
	char tmp_line[512];
	unsigned short portno;
	char *scratch;
	int result;
	int i;
	char *tpath, *ttpath;
	size_t file_size = 0;
	
	resource = php_url_parse((char *) path);
	if (resource == NULL || resource->path == NULL)
		return NULL;

	/* use port 21 if one wasn't specified */
	if (resource->port == 0)
		resource->port = 21;
	
	stream = php_stream_sock_open_host(resource->host, resource->port, SOCK_STREAM, 0, 0);
	if (stream == NULL)
		goto errexit;

	php_stream_context_set(stream, context);
	php_stream_notify_info(context, PHP_STREAM_NOTIFY_CONNECT, NULL, 0);

	/* Start talking to ftp server */
	result = GET_FTP_RESULT(stream);
	if (result > 299 || result < 200) {
		php_stream_notify_error(context, PHP_STREAM_NOTIFY_FAILURE, tmp_line, result);
		goto errexit;
	}

	/* send the user name */
	php_stream_write_string(stream, "USER ");
	if (resource->user != NULL) {
		php_raw_url_decode(resource->user, strlen(resource->user));
		php_stream_write_string(stream, resource->user);
	} else {
		php_stream_write_string(stream, "anonymous");
	}
	php_stream_write_string(stream, "\r\n");
	
	/* get the response */
	result = GET_FTP_RESULT(stream);
	
	/* if a password is required, send it */
	if (result >= 300 && result <= 399) {
		php_stream_notify_info(context, PHP_STREAM_NOTIFY_AUTH_REQUIRED, tmp_line, 0);

		php_stream_write_string(stream, "PASS ");
		if (resource->pass != NULL) {
			php_raw_url_decode(resource->pass, strlen(resource->pass));
			php_stream_write_string(stream, resource->pass);
		} else {
			/* if the user has configured who they are,
			   send that as the password */
			if (cfg_get_string("from", &scratch) == SUCCESS) {
				php_stream_write_string(stream, scratch);
			} else {
				php_stream_write_string(stream, "anonymous");
			}
		}
		php_stream_write_string(stream, "\r\n");
		
		/* read the response */
		result = GET_FTP_RESULT(stream);

		if (result > 299 || result < 200) {
			php_stream_notify_error(context, PHP_STREAM_NOTIFY_AUTH_RESULT, tmp_line, result);
		} else {
			php_stream_notify_info(context, PHP_STREAM_NOTIFY_AUTH_RESULT, tmp_line, result);
		}
	}
	if (result > 299 || result < 200)
		goto errexit;
	
	/* set the connection to be binary */
	php_stream_write_string(stream, "TYPE I\r\n");
	result = GET_FTP_RESULT(stream);
	if (result > 299 || result < 200)
		goto errexit;
	
	/* find out the size of the file (verifying it exists) */
	php_stream_write_string(stream, "SIZE ");
	php_stream_write_string(stream, resource->path);
	php_stream_write_string(stream, "\r\n");
	
	/* read the response */
	result = GET_FTP_RESULT(stream);
	if (mode[0] == 'r') {
		char *sizestr;
		
		/* when reading file, it must exist */
		if (result > 299 || result < 200) {
			errno = ENOENT;
			goto errexit;
		}
		
		sizestr = strchr(tmp_line, ' ');
		if (sizestr) {
			sizestr++;
			file_size = atoi(sizestr);
			php_stream_notify_file_size(context, file_size, tmp_line, result);
		}	
	} else {
		/* when writing file, it must NOT exist */
		if (result <= 299 && result >= 200) {
			errno = EEXIST;
			goto errexit;
		}
	}

	/* set up the passive connection */

    /* We try EPSV first, needed for IPv6 and works on some IPv4 servers */
	php_stream_write_string(stream, "EPSV\r\n");
	result = GET_FTP_RESULT(stream);

	/* check if we got a 229 response */
	if (result != 229) {
		/* EPSV failed, let's try PASV */
		php_stream_write_string(stream, "PASV\r\n");
		result = GET_FTP_RESULT(stream);
		
		/* make sure we got a 227 response */
		if (result != 227)
			goto errexit;

		/* parse pasv command (129, 80, 95, 25, 13, 221) */
		tpath = tmp_line;
		/* skip over the "227 Some message " part */
		for (tpath += 4; *tpath && !isdigit((int) *tpath); tpath++);
		if (!*tpath)
			goto errexit;
		/* skip over the host ip, we just assume it's the same */
		for (i = 0; i < 4; i++) {
			for (; isdigit((int) *tpath); tpath++);
			if (*tpath != ',')
				goto errexit;
			tpath++;
		}
		/* pull out the MSB of the port */
		portno = (unsigned short) strtol(tpath, &ttpath, 10) * 256;
		if (ttpath == NULL) {
			/* didn't get correct response from PASV */
			goto errexit;
		}
		tpath = ttpath;
		if (*tpath != ',')
			goto errexit;
		tpath++;
		/* pull out the LSB of the port */
		portno += (unsigned short) strtol(tpath, &ttpath, 10);
	} else {
		/* parse epsv command (|||6446|) */
		for (i = 0, tpath = tmp_line + 4; *tpath; tpath++) {
			if (*tpath == '|') {
				i++;
				if (i == 3)
					break;
			}
		}
		if (i < 3)
			goto errexit;
		/* pull out the port */
		portno = (unsigned short) strtol(tpath + 1, &ttpath, 10);
	}
	
	if (ttpath == NULL) {
		/* didn't get correct response from EPSV/PASV */
		goto errexit;
	}
	
	if (mode[0] == 'r') {
		/* retrieve file */
		php_stream_write_string(stream, "RETR ");
	} else {
		/* store file */
		php_stream_write_string(stream, "STOR ");
	} 
	if (resource->path != NULL) {
		php_stream_write_string(stream, resource->path);
	} else {
		php_stream_write_string(stream, "/");
	}
	
	/* close control connection */
	php_stream_write_string(stream, "\r\nQUIT\r\n");
	php_stream_close(stream);

	/* open the data channel */
	stream = php_stream_sock_open_host(resource->host, portno, SOCK_STREAM, 0, 0);
	if (stream == NULL)
		goto errexit;

	php_stream_context_set(stream, context);
	php_stream_notify_progress_init(context, 0, file_size);

	php_url_free(resource);
	return stream;

 errexit:
	php_url_free(resource);
	if (stream) {
		php_stream_notify_error(context, PHP_STREAM_NOTIFY_FAILURE, tmp_line, result);
		php_stream_close(stream);
	}
	if (tmp_line[0] != '\0')
		zend_error(E_WARNING, "FTP server reports %s", tmp_line);
	return NULL;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
