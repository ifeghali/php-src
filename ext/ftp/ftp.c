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
   | Authors: Andrew Skalski <askalski@chek.com>                          |
   |          Stefan Esser <sesser@php.net> (resume functions)            |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "php.h"

#if HAVE_FTP

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <time.h>
#ifdef PHP_WIN32
#include <winsock.h>
#else
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <errno.h>

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include "ftp.h"
#include "ext/standard/fsock.h"

/* sends an ftp command, returns true on success, false on error.
 * it sends the string "cmd args\r\n" if args is non-null, or
 * "cmd\r\n" if args is null
 */
static int		ftp_putcmd(	ftpbuf_t *ftp,
					const char *cmd,
					const char *args);

/* wrapper around send/recv to handle timeouts */
static int		my_send(ftpbuf_t *ftp, int s, void *buf, size_t len);
static int		my_recv(ftpbuf_t *ftp, int s, void *buf, size_t len);
static int		my_accept(ftpbuf_t *ftp, int s, struct sockaddr *addr,
				int *addrlen);

/* reads a line the socket , returns true on success, false on error */
static int		ftp_readline(ftpbuf_t *ftp);

/* reads an ftp response, returns true on success, false on error */
static int		ftp_getresp(ftpbuf_t *ftp);

/* sets the ftp transfer type */
static int		ftp_type(ftpbuf_t *ftp, ftptype_t type);

/* opens up a data stream */
static databuf_t*	ftp_getdata(ftpbuf_t *ftp);

/* accepts the data connection, returns updated data buffer */
static databuf_t*	data_accept(databuf_t *data, ftpbuf_t *ftp);

/* closes the data connection, returns NULL */
static databuf_t*	data_close(databuf_t *data);

/* generic file lister */
static char**		ftp_genlist(ftpbuf_t *ftp,
				const char *cmd, const char *path);

/* IP and port conversion box */
union ipbox {
	unsigned long	l[2];
	unsigned short	s[4];
	unsigned char	c[8];
};

/* {{{ ftp_open
 */
ftpbuf_t*
ftp_open(const char *host, short port, long timeout_sec)
{
	ftpbuf_t		*ftp;
	int			size;
	struct timeval tv;


	/* alloc the ftp structure */
	ftp = calloc(1, sizeof(*ftp));
	if (ftp == NULL) {
		perror("calloc");
		return NULL;
	}

	tv.tv_sec = timeout_sec;
	tv.tv_usec = 0;

	ftp->fd = php_hostconnect(host, (unsigned short) (port ? port : 21), SOCK_STREAM, &tv);
	if (ftp->fd == -1) {
		goto bail;
	}

	/* Default Settings */
	ftp->timeout_sec = timeout_sec;
	ftp->async = 0;

	size = sizeof(ftp->localaddr);
	memset(&ftp->localaddr, 0, size);
	if (getsockname(ftp->fd, (struct sockaddr*) &ftp->localaddr, &size) == -1) {
		perror("getsockname");
		goto bail;
	}

	if (!ftp_getresp(ftp) || ftp->resp != 220) {
		goto bail;
	}

	return ftp;

bail:
	if (ftp->fd != -1)
		closesocket(ftp->fd);
	free(ftp);
	return NULL;
}
/* }}} */

/* {{{ ftp_close
 */
ftpbuf_t*
ftp_close(ftpbuf_t *ftp)
{
	if (ftp == NULL)
		return NULL;
	if (ftp->fd != -1)
		closesocket(ftp->fd);
	ftp_gc(ftp);
	free(ftp);
	return NULL;
}
/* }}} */

/* {{{ ftp_gc
 */
void
ftp_gc(ftpbuf_t *ftp)
{
	if (ftp == NULL)
		return;

	free(ftp->pwd);
	ftp->pwd = NULL;
	free(ftp->syst);
	ftp->syst = NULL;
}
/* }}} */

/* {{{ ftp_quit
 */
int
ftp_quit(ftpbuf_t *ftp)
{
	if (ftp == NULL)
		return 0;

	if (!ftp_putcmd(ftp, "QUIT", NULL))
		return 0;
	if (!ftp_getresp(ftp) || ftp->resp != 221)
		return 0;

	free(ftp->pwd);
	ftp->pwd = NULL;

	return 1;
}
/* }}} */

/* {{{ ftp_login
 */
int
ftp_login(ftpbuf_t *ftp, const char *user, const char *pass)
{
	if (ftp == NULL)
		return 0;

	if (!ftp_putcmd(ftp, "USER", user))
		return 0;
	if (!ftp_getresp(ftp))
		return 0;
	if (ftp->resp == 230)
		return 1;
	if (ftp->resp != 331)
		return 0;
	if (!ftp_putcmd(ftp, "PASS", pass))
		return 0;
	if (!ftp_getresp(ftp))
		return 0;
	return (ftp->resp == 230);
}
/* }}} */

/* {{{ ftp_reinit
 */
int
ftp_reinit(ftpbuf_t *ftp)
{
	if (ftp == NULL)
		return 0;

	ftp_gc(ftp);

	ftp->async = 0;

	if (!ftp_putcmd(ftp, "REIN", NULL))
		return 0;
	if (!ftp_getresp(ftp) || ftp->resp != 220)
		return 0;

	return 1;
}
/* }}} */

/* {{{ ftp_syst
 */
const char*
ftp_syst(ftpbuf_t *ftp)
{
	char		*syst, *end;

	if (ftp == NULL)
		return NULL;

	/* default to cached value */
	if (ftp->syst)
		return ftp->syst;

	if (!ftp_putcmd(ftp, "SYST", NULL))
		return NULL;
	if (!ftp_getresp(ftp) || ftp->resp != 215)
		return NULL;

	syst = ftp->inbuf;
	if ((end = strchr(syst, ' ')))
		*end = 0;
	ftp->syst = strdup(syst);
	if (end)
		*end = ' ';

	return ftp->syst;
}
/* }}} */

/* {{{ ftp_pwd
 */
const char*
ftp_pwd(ftpbuf_t *ftp)
{
	char		*pwd, *end;

	if (ftp == NULL)
		return NULL;

	/* default to cached value */
	if (ftp->pwd)
		return ftp->pwd;

	if (!ftp_putcmd(ftp, "PWD", NULL))
		return NULL;
	if (!ftp_getresp(ftp) || ftp->resp != 257)
		return NULL;

	/* copy out the pwd from response */
	if ((pwd = strchr(ftp->inbuf, '"')) == NULL)
		return NULL;
	end = strrchr(++pwd, '"');
	*end = 0;
	ftp->pwd = strdup(pwd);
	*end = '"';

	return ftp->pwd;
}
/* }}} */

/* {{{ ftp_exec
 */
int
ftp_exec(ftpbuf_t *ftp, const char *cmd)
{
	if (ftp == NULL)
		return 0;
	if (!ftp_putcmd(ftp, "SITE EXEC", cmd))
		return 0;
	if (!ftp_getresp(ftp) || ftp->resp != 200)
		return 0;

	return 1;
}
/* }}} */

/* {{{ ftp_chdir
 */
int
ftp_chdir(ftpbuf_t *ftp, const char *dir)
{
	if (ftp == NULL)
		return 0;

	free(ftp->pwd);
	ftp->pwd = NULL;

	if (!ftp_putcmd(ftp, "CWD", dir))
		return 0;
	if (!ftp_getresp(ftp) || ftp->resp != 250)
		return 0;

	return 1;
}
/* }}} */

/* {{{ ftp_cdup
 */
int
ftp_cdup(ftpbuf_t *ftp)
{
	if (ftp == NULL)
		return 0;

	free(ftp->pwd);
	ftp->pwd = NULL;

	if (!ftp_putcmd(ftp, "CDUP", NULL))
		return 0;
	if (!ftp_getresp(ftp) || ftp->resp != 250)
		return 0;

	return 1;
}
/* }}} */

/* {{{ ftp_mkdir
 */
char*
ftp_mkdir(ftpbuf_t *ftp, const char *dir)
{
	char		*mkd, *end;

	if (ftp == NULL)
		return NULL;

	if (!ftp_putcmd(ftp, "MKD", dir))
		return NULL;
	if (!ftp_getresp(ftp) || ftp->resp != 257)
		return NULL;

	/* copy out the dir from response */
	if ((mkd = strchr(ftp->inbuf, '"')) == NULL) {
		mkd = strdup(dir);
		return mkd;
	}

	end = strrchr(++mkd, '"');
	*end = 0;
	mkd = strdup(mkd);
	*end = '"';

	return mkd;
}
/* }}} */

/* {{{ ftp_rmdir
 */
int
ftp_rmdir(ftpbuf_t *ftp, const char *dir)
{
	if (ftp == NULL)
		return 0;

	if (!ftp_putcmd(ftp, "RMD", dir))
		return 0;
	if (!ftp_getresp(ftp) || ftp->resp != 250)
		return 0;

	return 1;
}
/* }}} */

/* {{{ ftp_nlist
 */
char**
ftp_nlist(ftpbuf_t *ftp, const char *path)
{
	return ftp_genlist(ftp, "NLST", path);
}
/* }}} */

/* {{{ ftp_list
 */
char**
ftp_list(ftpbuf_t *ftp, const char *path, int recursive)
{
	return ftp_genlist(ftp, ((recursive) ? "LIST -R" : "LIST"), path);
}
/* }}} */

/* {{{ ftp_type
 */
int
ftp_type(ftpbuf_t *ftp, ftptype_t type)
{
	char		typechar[2] = "?";

	if (ftp == NULL)
		return 0;

	if (type == ftp->type)
		return 1;

	if (type == FTPTYPE_ASCII)
		typechar[0] = 'A';
	else if (type == FTPTYPE_IMAGE)
		typechar[0] = 'I';
	else
		return 0;

	if (!ftp_putcmd(ftp, "TYPE", typechar))
		return 0;
	if (!ftp_getresp(ftp) || ftp->resp != 200)
		return 0;

	ftp->type = type;

	return 1;
}
/* }}} */

/* {{{ ftp_pasv
 */
int
ftp_pasv(ftpbuf_t *ftp, int pasv)
{
	char			*ptr;
	union ipbox		ipbox;
	unsigned long		b[6];
	int			n;
	struct sockaddr *sa;
	struct sockaddr_in *sin;

	if (ftp == NULL)
		return 0;

	if (pasv && ftp->pasv == 2)
		return 1;

	ftp->pasv = 0;
	if (!pasv)
		return 1;

	n = sizeof(ftp->pasvaddr);
	memset(&ftp->pasvaddr, 0, n);
	sa = (struct sockaddr *) &ftp->pasvaddr;

#ifdef HAVE_IPV6
	if (getpeername(ftp->fd, sa, &n) < 0)
		return 0;

	if (sa->sa_family == AF_INET6) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;
		char *endptr, delimiter;

		/* try EPSV first */
		if (!ftp_putcmd(ftp, "EPSV", NULL))
			return 0;
		if (!ftp_getresp(ftp))
			return 0;

		if (ftp->resp == 229) {
			/* parse out the port */
			for (ptr = ftp->inbuf; *ptr && *ptr != '('; ptr++);
			if (!*ptr)
				return 0;
			delimiter = *++ptr;
			for (n = 0; *ptr && n < 3; ptr++) {
				if (*ptr == delimiter)
					n++;
			}

			sin6->sin6_port = htons((unsigned short) strtol(ptr, &endptr, 10));
			if (ptr == endptr || *endptr != delimiter)
				return 0;

			ftp->pasv = 2;
			return 1;
		}
	}

	/* fall back to PASV */
#endif

	if (!ftp_putcmd(ftp, "PASV", NULL))
		return 0;
	if (!ftp_getresp(ftp) || ftp->resp != 227)
		return 0;

	/* parse out the IP and port */
	for (ptr = ftp->inbuf; *ptr && !isdigit(*ptr); ptr++);
	n = sscanf(ptr, "%lu,%lu,%lu,%lu,%lu,%lu",
		&b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
	if (n != 6)
		return 0;

	for (n=0; n<6; n++)
		ipbox.c[n] = (unsigned char) b[n];

	sin = (struct sockaddr_in *) sa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = ipbox.l[0];
	sin->sin_port = ipbox.s[2];

	ftp->pasv = 2;

	return 1;
}
/* }}} */

/* {{{ ftp_get
 */
int
ftp_get(ftpbuf_t *ftp, php_stream *outstream, const char *path, ftptype_t type, int resumepos)
{
	databuf_t		*data = NULL;
	char			*ptr;
	int			lastch;
	int			rcvd;
	char			arg[11];
	TSRMLS_FETCH();

	if (ftp == NULL)
		return 0;

	if (!ftp_type(ftp, type)) {
		goto bail;
	}

	if ((data = ftp_getdata(ftp)) == NULL) {
		goto bail;
	}

	if (resumepos>0) {
		sprintf(arg, "%u", resumepos);
		if (!ftp_putcmd(ftp, "REST", arg)) {
			goto bail;
		}
		if (!ftp_getresp(ftp) || (ftp->resp != 350)) {
			goto bail;
		}
	}

	if (!ftp_putcmd(ftp, "RETR", path)) {
		goto bail;
	}
	if (!ftp_getresp(ftp) || (ftp->resp != 150 && ftp->resp != 125)) {
		goto bail;
	}

	if ((data = data_accept(data, ftp)) == NULL) {
		goto bail;
	}

	lastch = 0;
	while ((rcvd = my_recv(ftp, data->fd, data->buf, FTP_BUFSIZE))) {
		if (rcvd == -1) {
			goto bail;
		}

		if (type == FTPTYPE_ASCII) {
			for (ptr = data->buf; rcvd; rcvd--, ptr++) {
				if (lastch == '\r' && *ptr != '\n')
					php_stream_putc(outstream, '\r');
				if (*ptr != '\r')
					php_stream_putc(outstream, *ptr);
				lastch = *ptr;
			}
		}
		else {
			php_stream_write(outstream, data->buf, rcvd);
		}
	}

	if (type == FTPTYPE_ASCII && lastch == '\r')
		php_stream_putc(outstream, '\r');

	data = data_close(data);

	if (php_stream_error(outstream)) {
		goto bail;
	}

	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250)) {
		goto bail;
	}

	return 1;
bail:
	data_close(data);
	return 0;
}
/* }}} */

/* {{{ ftp_put
 */
int
ftp_put(ftpbuf_t *ftp, const char *path, php_stream *instream, ftptype_t type, int startpos)
{
	databuf_t		*data = NULL;
	int			size;
	char			*ptr;
	int			ch;
	char			arg[11];
	TSRMLS_FETCH();

	if (ftp == NULL)
		return 0;

	if (!ftp_type(ftp, type))
		goto bail;

	if ((data = ftp_getdata(ftp)) == NULL)
		goto bail;

	if (startpos>0) {
		sprintf(arg, "%u", startpos);
		if (!ftp_putcmd(ftp, "REST", arg)) {
			goto bail;
		}
		if (!ftp_getresp(ftp) || (ftp->resp != 350)) {
			goto bail;
		}
	}

	if (!ftp_putcmd(ftp, "STOR", path))
		goto bail;
	if (!ftp_getresp(ftp) || (ftp->resp != 150 && ftp->resp != 125))
		goto bail;

	if ((data = data_accept(data, ftp)) == NULL)
		goto bail;

	size = 0;
	ptr = data->buf;
	while ((ch = php_stream_getc(instream))!=EOF && !php_stream_eof(instream)) {
		/* flush if necessary */
		if (FTP_BUFSIZE - size < 2) {
			if (my_send(ftp, data->fd, data->buf, size) != size)
				goto bail;
			ptr = data->buf;
			size = 0;
		}

		if (ch == '\n' && type == FTPTYPE_ASCII) {
			*ptr++ = '\r';
			size++;
		}

		*ptr++ = ch;
		size++;
	}

	if (size && my_send(ftp, data->fd, data->buf, size) != size)
		goto bail;

	if (php_stream_error(instream))
		goto bail;

	data = data_close(data);

	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250))
		goto bail;

	return 1;
bail:
	data_close(data);
	return 0;
}
/* }}} */

/* {{{ ftp_size
 */
int
ftp_size(ftpbuf_t *ftp, const char *path)
{
	if (ftp == NULL)
		return -1;

	if (!ftp_type(ftp, FTPTYPE_IMAGE))
		return -1;
	if (!ftp_putcmd(ftp, "SIZE", path))
		return -1;
	if (!ftp_getresp(ftp) || ftp->resp != 213)
		return -1;

	return atoi(ftp->inbuf);
}
/* }}} */

/* {{{ ftp_mdtm
 */
time_t
ftp_mdtm(ftpbuf_t *ftp, const char *path)
{
	time_t		stamp;
	struct tm	*gmt, tmbuf;
	struct tm	tm;
	char		*ptr;
	int		n;

	if (ftp == NULL)
		return -1;

	if (!ftp_putcmd(ftp, "MDTM", path))
		return -1;
	if (!ftp_getresp(ftp) || ftp->resp != 213)
		return -1;

	/* parse out the timestamp */
	for (ptr = ftp->inbuf; *ptr && !isdigit(*ptr); ptr++);
	n = sscanf(ptr, "%4u%2u%2u%2u%2u%2u",
		&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
		&tm.tm_hour, &tm.tm_min, &tm.tm_sec);
	if (n != 6)
		return -1;
	tm.tm_year -= 1900;
	tm.tm_mon--;
	tm.tm_isdst = -1;

	/* figure out the GMT offset */
	stamp = time(NULL);
	gmt = php_gmtime_r(&stamp, &tmbuf);
	gmt->tm_isdst = -1;

	/* apply the GMT offset */
	tm.tm_sec += stamp - mktime(gmt);
	tm.tm_isdst = gmt->tm_isdst;

	stamp = mktime(&tm);

	return stamp;
}
/* }}} */

/* {{{ ftp_delete
 */
int
ftp_delete(ftpbuf_t *ftp, const char *path)
{
	if (ftp == NULL)
		return 0;

	if (!ftp_putcmd(ftp, "DELE", path))
		return 0;
	if (!ftp_getresp(ftp) || ftp->resp != 250)
		return 0;

	return 1;
}
/* }}} */

/* {{{ ftp_rename
 */
int
ftp_rename(ftpbuf_t *ftp, const char *src, const char *dest)
{
	if (ftp == NULL)
		return 0;

	if (!ftp_putcmd(ftp, "RNFR", src))
		return 0;
	if (!ftp_getresp(ftp) || ftp->resp != 350)
		return 0;

	if (!ftp_putcmd(ftp, "RNTO", dest))
		return 0;
	if (!ftp_getresp(ftp) || ftp->resp != 250)
		return 0;

	return 1;
}
/* }}} */

/* {{{ ftp_site
 */
int
ftp_site(ftpbuf_t *ftp, const char *cmd)
{
	if (ftp == NULL)
		return 0;

	if (!ftp_putcmd(ftp, "SITE", cmd))
		return 0;
	if (!ftp_getresp(ftp) || ftp->resp < 200 || ftp->resp >= 300)
		return 0;

	return 1;
}
/* }}} */

/* static functions */

/* {{{ ftp_putcmd
 */
int
ftp_putcmd(ftpbuf_t *ftp, const char *cmd, const char *args)
{
	int		size;
	char		*data;

	/* build the output buffer */
	if (args && args[0]) {
		/* "cmd args\r\n\0" */
		if (strlen(cmd) + strlen(args) + 4 > FTP_BUFSIZE)
			return 0;
		size = sprintf(ftp->outbuf, "%s %s\r\n", cmd, args);
	}
	else {
		/* "cmd\r\n\0" */
		if (strlen(cmd) + 3 > FTP_BUFSIZE)
			return 0;
		size = sprintf(ftp->outbuf, "%s\r\n", cmd);
	}

	data = ftp->outbuf;
	if (my_send(ftp, ftp->fd, data, size) != size)
		return 0;

	return 1;
}
/* }}} */

/* {{{ ftp_readline
 */
int
ftp_readline(ftpbuf_t *ftp)
{
	int		size, rcvd;
	char		*data, *eol;

	/* shift the extra to the front */
	size = FTP_BUFSIZE;
	rcvd = 0;
	if (ftp->extra) {
		memmove(ftp->inbuf, ftp->extra, ftp->extralen);
		rcvd = ftp->extralen;
	}

	data = ftp->inbuf;

	do {
		size -= rcvd;
		for (eol = data; rcvd; rcvd--, eol++) {
			if (*eol == '\r') {
				*eol = 0;
				ftp->extra = eol + 1;
				if (rcvd > 1 && *(eol + 1) == '\n') {
					ftp->extra++;
					rcvd--;
				}
				if ((ftp->extralen = --rcvd) == 0)
					ftp->extra = NULL;
				return 1;
			}
			else if (*eol == '\n') {
				*eol = 0;
				ftp->extra = eol + 1;
				if ((ftp->extralen = --rcvd) == 0)
					ftp->extra = NULL;
				return 1;
			}
		}

		data = eol;
		if ((rcvd = my_recv(ftp, ftp->fd, data, size)) < 1) {
			return 0;
		}
	} while (size);

	return 0;
}
/* }}} */

/* {{{ ftp_getresp
 */
int
ftp_getresp(ftpbuf_t *ftp)
{
	char *buf;

	if (ftp == NULL) return 0;
	buf = ftp->inbuf;
	ftp->resp = 0;

	while (1) {

		if (!ftp_readline(ftp)) {
			return 0;
		}

		/* Break out when the end-tag is found */
		if (isdigit(ftp->inbuf[0]) &&
			isdigit(ftp->inbuf[1]) &&
			isdigit(ftp->inbuf[2]) &&
			ftp->inbuf[3] == ' ') {
			break;
		}
	}

	/* translate the tag */
	if (!isdigit(ftp->inbuf[0]) ||
		!isdigit(ftp->inbuf[1]) ||
		!isdigit(ftp->inbuf[2]))
	{
		return 0;
	}

	ftp->resp =	100 * (ftp->inbuf[0] - '0') +
			10 * (ftp->inbuf[1] - '0') +
			(ftp->inbuf[2] - '0');

	memmove(ftp->inbuf, ftp->inbuf + 4, FTP_BUFSIZE - 4);

	if (ftp->extra)
		ftp->extra -= 4;

	return 1;
}
/* }}} */

/* {{{ my_send
 */
int
my_send(ftpbuf_t *ftp, int s, void *buf, size_t len)
{
	fd_set		write_set;
	struct timeval	tv;
	int		n, size, sent;

	size = len;
	while (size) {
		tv.tv_sec = ftp->timeout_sec;
		tv.tv_usec = 0;

		FD_ZERO(&write_set);
		FD_SET(s, &write_set);
		n = select(s + 1, NULL, &write_set, NULL, &tv);
		if (n < 1) {
#ifndef PHP_WIN32
			if (n == 0)
				errno = ETIMEDOUT;
#endif
			return -1;
		}

		sent = send(s, buf, size, 0);
		if (sent == -1)
			return -1;

		buf = (char*) buf + sent;
		size -= sent;
	}

	return len;
}
/* }}} */

/* {{{ my_recv
 */
int
my_recv(ftpbuf_t *ftp, int s, void *buf, size_t len)
{
	fd_set		read_set;
	struct timeval	tv;
	int		n;

	tv.tv_sec = ftp->timeout_sec;
	tv.tv_usec = 0;

	FD_ZERO(&read_set);
	FD_SET(s, &read_set);
	n = select(s + 1, &read_set, NULL, NULL, &tv);
	if (n < 1) {
#ifndef PHP_WIN32
		if (n == 0)
			errno = ETIMEDOUT;
#endif
		return -1;
	}

	return recv(s, buf, len, 0);
}
/* }}} */

/* {{{ data_available
 */
int
data_available(ftpbuf_t *ftp, int s)
{
	fd_set		read_set;
	struct timeval	tv;
	int		n;

	tv.tv_sec = 0;
	tv.tv_usec = 1;

	FD_ZERO(&read_set);
	FD_SET(s, &read_set);
	n = select(s + 1, &read_set, NULL, NULL, &tv);
	if (n < 1) {
#ifndef PHP_WIN32
		if (n == 0)
			errno = ETIMEDOUT;
#endif
		return 0;
	}

	return 1;
}
/* }}} */
/* {{{ data_writeable
 */
int
data_writeable(ftpbuf_t *ftp, int s)
{
	fd_set		write_set;
	struct timeval	tv;
	int		n;

	tv.tv_sec = 0;
	tv.tv_usec = 1;

	FD_ZERO(&write_set);
	FD_SET(s, &write_set);
	n = select(s + 1, NULL, &write_set, NULL, &tv);
	if (n < 1) {
#ifndef PHP_WIN32
		if (n == 0)
			errno = ETIMEDOUT;
#endif
		return 0;
	}

	return 1;
}
/* }}} */

/* {{{ my_accept
 */
int
my_accept(ftpbuf_t *ftp, int s, struct sockaddr *addr, int *addrlen)
{
	fd_set		accept_set;
	struct timeval	tv;
	int		n;

	tv.tv_sec = ftp->timeout_sec;
	tv.tv_usec = 0;
	FD_ZERO(&accept_set);
	FD_SET(s, &accept_set);

	n = select(s + 1, &accept_set, NULL, NULL, &tv);
	if (n < 1) {
#ifndef PHP_WIN32
		if (n == 0)
			errno = ETIMEDOUT;
#endif
		return -1;
	}

	return accept(s, addr, addrlen);
}
/* }}} */

/* {{{ ftp_getdata
 */
databuf_t*
ftp_getdata(ftpbuf_t *ftp)
{
	int			fd = -1;
	databuf_t		*data;
	php_sockaddr_storage addr;
	struct sockaddr *sa;
	int			size;
	union ipbox		ipbox;
	char			arg[sizeof("255, 255, 255, 255, 255, 255")];
	struct timeval	tv;


	/* ask for a passive connection if we need one */
	if (ftp->pasv && !ftp_pasv(ftp, 1))
		return NULL;

	/* alloc the data structure */
	data = calloc(1, sizeof(*data));
	if (data == NULL) {
		perror("calloc");
		return NULL;
	}
	data->listener = -1;
	data->fd = -1;
	data->type = ftp->type;

	sa = (struct sockaddr *) &ftp->localaddr;
	/* bind/listen */
	if ((fd = socket(sa->sa_family, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		goto bail;
	}

	/* passive connection handler */
	if (ftp->pasv) {
		/* clear the ready status */
		ftp->pasv = 1;

		/* connect */
		/* Win 95/98 seems not to like size > sizeof(sockaddr_in) */
		size = php_sockaddr_size(&ftp->pasvaddr);
		tv.tv_sec = ftp->timeout_sec;
		tv.tv_usec = 0;
		if (php_connect_nonb(fd, (struct sockaddr*) &ftp->pasvaddr, size, &tv) == -1) {
			perror("connect");
			goto bail;
		}

		data->fd = fd;

		return data;
	}


	/* active (normal) connection */

	/* bind to a local address */
	php_any_addr(sa->sa_family, &addr, 0);
	size = php_sockaddr_size(&addr);

	if (bind(fd, (struct sockaddr*) &addr, size) == -1) {
		perror("bind");
		goto bail;
	}

	if (getsockname(fd, (struct sockaddr*) &addr, &size) == -1) {
		perror("getsockname");
		goto bail;
	}

	if (listen(fd, 5) == -1) {
		perror("listen");
		goto bail;
	}

	data->listener = fd;

#ifdef HAVE_IPV6
	if (sa->sa_family == AF_INET6) {
		/* need to use EPRT */
		char eprtarg[INET6_ADDRSTRLEN + sizeof("|x||xxxxx|")];
		char out[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &((struct sockaddr_in6*) sa)->sin6_addr, out, sizeof(out));
		sprintf(eprtarg, "|2|%s|%hu|", out, ntohs(((struct sockaddr_in6 *) &addr)->sin6_port));

		if (!ftp_putcmd(ftp, "EPRT", eprtarg))
			goto bail;

		if (!ftp_getresp(ftp) || ftp->resp != 200)
			goto bail;

		return data;
	}
#endif

	/* send the PORT */
	ipbox.l[0] = ((struct sockaddr_in*) sa)->sin_addr.s_addr;
	ipbox.s[2] = ((struct sockaddr_in*) &addr)->sin_port;
	sprintf(arg, "%u,%u,%u,%u,%u,%u",
		ipbox.c[0], ipbox.c[1], ipbox.c[2], ipbox.c[3],
		ipbox.c[4], ipbox.c[5]);

	if (!ftp_putcmd(ftp, "PORT", arg))
		goto bail;
	if (!ftp_getresp(ftp) || ftp->resp != 200)
		goto bail;

	return data;

bail:
	if (fd != -1)
		closesocket(fd);
	free(data);
	return NULL;
}
/* }}} */

/* {{{ data_accept
 */
databuf_t*
data_accept(databuf_t *data, ftpbuf_t *ftp)
{
	php_sockaddr_storage addr;
	int			size;

	if (data->fd != -1)
		return data;

	size = sizeof(addr);
	data->fd = my_accept(ftp, data->listener, (struct sockaddr*) &addr, &size);
	closesocket(data->listener);
	data->listener = -1;

	if (data->fd == -1) {
		free(data);
		return NULL;
	}

	return data;
}
/* }}} */

/* {{{ data_close
 */
databuf_t*
data_close(databuf_t *data)
{
	if (data == NULL)
		return NULL;
	if (data->listener != -1)
		closesocket(data->listener);
	if (data->fd != -1)
		closesocket(data->fd);
	free(data);
	return NULL;
}
/* }}} */

/* {{{ ftp_genlist
 */
char**
ftp_genlist(ftpbuf_t *ftp, const char *cmd, const char *path)
{
	FILE		*tmpfp = NULL;
	databuf_t	*data = NULL;
	char		*ptr;
	int		ch, lastch;
	int		size, rcvd;
	int		lines;
	char		**ret = NULL;
	char		**entry;
	char		*text;


	if ((tmpfp = tmpfile()) == NULL)
		return NULL;

	if (!ftp_type(ftp, FTPTYPE_ASCII))
		goto bail;

	if ((data = ftp_getdata(ftp)) == NULL)
		goto bail;

	if (!ftp_putcmd(ftp, cmd, path))
		goto bail;
	if (!ftp_getresp(ftp) || (ftp->resp != 150 && ftp->resp != 125))
		goto bail;

	/* pull data buffer into tmpfile */
	if ((data = data_accept(data, ftp)) == NULL)
		goto bail;

	size = 0;
	lines = 0;
	lastch = 0;
	while ((rcvd = my_recv(ftp, data->fd, data->buf, FTP_BUFSIZE))) {
		if (rcvd == -1)
			goto bail;

		fwrite(data->buf, rcvd, 1, tmpfp);

		size += rcvd;
		for (ptr = data->buf; rcvd; rcvd--, ptr++) {
			if (*ptr == '\n' && lastch == '\r')
				lines++;
			else
				size++;
			lastch = *ptr;
		}
	}

	data = data_close(data);

	if (ferror(tmpfp))
		goto bail;



	rewind(tmpfp);

	ret = malloc((lines + 1) * sizeof(char**) + size * sizeof(char*));
	if (ret == NULL) {
		perror("malloc");
		goto bail;
	}

	entry = ret;
	text = (char*) (ret + lines + 1);
	*entry = text;
	lastch = 0;
	while ((ch = getc(tmpfp)) != EOF) {
		if (ch == '\n' && lastch == '\r') {
			*(text - 1) = 0;
			*++entry = text;
		}
		else {
			*text++ = ch;
		}
		lastch = ch;
	}
	*entry = NULL;

	if (ferror(tmpfp))
		goto bail;

	fclose(tmpfp);

	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250)) {
		free(ret);
		return NULL;
	}

	return ret;
bail:
	data_close(data);
	fclose(tmpfp);
	free(ret);
	return NULL;
}
/* }}} */

/* {{{ ftp_async_get
 */
int
ftp_async_get(ftpbuf_t *ftp, php_stream *outstream, const char *path, ftptype_t type, int resumepos)
{
	databuf_t		*data = NULL;
	char			*ptr;
	int			lastch;
	int			rcvd;
	char			arg[11];
	TSRMLS_FETCH();

	if (ftp == NULL)
		goto bail;

	if (!ftp_type(ftp, type)) {
		goto bail;
	}

	if ((data = ftp_getdata(ftp)) == NULL) {
		goto bail;
	}

	if (resumepos>0) {
		sprintf(arg, "%u", resumepos);
		if (!ftp_putcmd(ftp, "REST", arg)) {
			goto bail;
		}
		if (!ftp_getresp(ftp) || (ftp->resp != 350)) {
			goto bail;
		}
	}

	if (!ftp_putcmd(ftp, "RETR", path)) {
		goto bail;
	}
	if (!ftp_getresp(ftp) || (ftp->resp != 150 && ftp->resp != 125)) {
		goto bail;
	}

	if ((data = data_accept(data, ftp)) == NULL) {
		goto bail;
	}

	ftp->data = data;
	ftp->stream = outstream;
	ftp->lastch = 0;
	ftp->async = 1;

	return (ftp_async_continue_read(ftp));

bail:
	data_close(data);
	return PHP_FTP_FAILED;
}
/* }}} */

/* {{{ ftp_aget
 */
int
ftp_async_continue_read(ftpbuf_t *ftp)
{
	databuf_t	*data = NULL;
	char		*ptr;
	int			lastch;
	int			rcvd;
	ftptype_t	type;
	TSRMLS_FETCH();

	data = ftp->data;

	/* check if there is already more data */
	if (!data_available(ftp, data->fd)) {
		return PHP_FTP_MOREDATA;
	}

	type = ftp->type;

	lastch = ftp->lastch;
	if (rcvd = my_recv(ftp, data->fd, data->buf, FTP_BUFSIZE)) {
		if (rcvd == -1) {
			goto bail;
		}

		if (type == FTPTYPE_ASCII) {
			for (ptr = data->buf; rcvd; rcvd--, ptr++) {
				if (lastch == '\r' && *ptr != '\n')
					php_stream_putc(ftp->stream, '\r');
				if (*ptr != '\r')
					php_stream_putc(ftp->stream, *ptr);
				lastch = *ptr;
			}
		}
		else {
			php_stream_write(ftp->stream, data->buf, rcvd);
		}

		ftp->lastch = lastch;
		return PHP_FTP_MOREDATA;
	}

	if (type == FTPTYPE_ASCII && lastch == '\r')
		php_stream_putc(ftp->stream, '\r');

	data = data_close(data);

	if (php_stream_error(ftp->stream)) {
		goto bail;
	}

	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250)) {
		goto bail;
	}

	ftp->async = 0;
	return PHP_FTP_FINISHED;
bail:
	ftp->async = 0;
	data_close(data);
	return PHP_FTP_FAILED;
}
/* }}} */

/* {{{ ftp_async_put
 */
int
ftp_async_put(ftpbuf_t *ftp, const char *path, php_stream *instream, ftptype_t type, int startpos)
{
	databuf_t		*data = NULL;
	int			size;
	char			*ptr;
	int			ch;
	char			arg[11];
	TSRMLS_FETCH();

	if (ftp == NULL)
		return 0;

	if (!ftp_type(ftp, type))
		goto bail;

	if ((data = ftp_getdata(ftp)) == NULL)
		goto bail;

	if (startpos>0) {
		sprintf(arg, "%u", startpos);
		if (!ftp_putcmd(ftp, "REST", arg)) {
			goto bail;
		}
		if (!ftp_getresp(ftp) || (ftp->resp != 350)) {
			goto bail;
		}
	}

	if (!ftp_putcmd(ftp, "STOR", path))
		goto bail;
	if (!ftp_getresp(ftp) || (ftp->resp != 150 && ftp->resp != 125))
		goto bail;

	if ((data = data_accept(data, ftp)) == NULL)
		goto bail;

	ftp->data = data;
	ftp->stream = instream;
	ftp->lastch = 0;
	ftp->async = 1;

	return (ftp_async_continue_write(ftp));

bail:
	data_close(data);
	return PHP_FTP_FAILED;

}
/* }}} */


/* {{{ ftp_async_continue_write
 */
int
ftp_async_continue_write(ftpbuf_t *ftp)
{
	int			size;
	char			*ptr;
	int 			ch;
	TSRMLS_FETCH();


	/* check if we can write more data */
	if (!data_writeable(ftp, ftp->data->fd)) {
		return PHP_FTP_MOREDATA;
	}

	size = 0;
	ptr = ftp->data->buf;
	while ((ch = php_stream_getc(ftp->stream))!=EOF && !php_stream_eof(ftp->stream)) {

		if (ch == '\n' && ftp->type == FTPTYPE_ASCII) {
			*ptr++ = '\r';
			size++;
		}

		*ptr++ = ch;
		size++;

		/* flush if necessary */
		if (FTP_BUFSIZE - size < 2) {
			if (my_send(ftp, ftp->data->fd, ftp->data->buf, size) != size)
				goto bail;
			return PHP_FTP_MOREDATA;
		}

	}

	if (size && my_send(ftp, ftp->data->fd, ftp->data->buf, size) != size)
		goto bail;

	if (php_stream_error(ftp->stream))
		goto bail;

	ftp->data = data_close(ftp->data);

	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250))
		goto bail;

	ftp->async = 0;
	return PHP_FTP_FINISHED;
bail:
	data_close(ftp->data);
	ftp->async = 0;
	return PHP_FTP_FAILED;
}
/* }}} */


#endif /* HAVE_FTP */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
