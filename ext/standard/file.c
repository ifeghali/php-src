/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_01.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   | PHP 4.0 patches by Thies C. Arntzen (thies@digicol.de)               |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

/* Synced with php 3.0 revision 1.218 1999-06-16 [ssb] */

/* {{{ includes */

#include "php.h"
#include "php_globals.h"
#include "ext/standard/flock_compat.h"
#include "ext/standard/exec.h"

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
#include "win32/winutil.h"
#else
#include <sys/param.h>
#include <sys/socket.h>
#endif
#include "ext/standard/head.h"
#include "safe_mode.h"
#include "php_string.h"
#include "file.h"
#if HAVE_PWD_H
#ifdef PHP_WIN32
#include "win32/pwd.h"
#else
#include <pwd.h>
#endif
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef PHP_WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#include "snprintf.h"
#include "fsock.h"
#include "fopen-wrappers.h"
#include "php_globals.h"

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#if MISSING_FCLOSE_DECL
extern int fclose();
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void *) -1)
#endif

#include "php_realpath.h"

/* }}} */
/* {{{ ZTS-stuff / Globals / Prototypes */

typedef struct {
	int fgetss_state;
	int pclose_ret;
} php_file_globals;

#ifdef ZTS
#define FIL(v) (file_globals->v)
#define FIL_FETCH() php_file_globals *file_globals = ts_resource(file_globals_id)
int file_globals_id;
#else
#define FIL(v) (file_globals.v)
#define FIL_FETCH()
php_file_globals file_globals;
#endif

static void _file_fopen_dtor(FILE *pipe);
static void _file_popen_dtor(FILE *pipe);
static void _file_socket_dtor(int *sock);
static void _file_upload_dtor(char *file);

/* sharing globals is *evil* */
static int le_fopen,le_popen, le_socket, le_uploads; 


/* }}} */
/* {{{ tempnam */

#ifndef HAVE_TEMPNAM
/*
 * Copyright (c) 1988, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef MAXPATHLEN
# ifdef PATH_MAX
#  define MAXPATHLEN PATH_MAX
# else
#  define MAXPATHLEN 255
# endif
#endif

char *tempnam(const char *dir, const char *pfx)
{
	int save_errno;
	char *f, *name;
	static char path_tmp[] = "/tmp";
	
	if (!(name = emalloc(MAXPATHLEN))) {
		return(NULL);
	}
	
	if (!pfx) {
		pfx = "tmp.";
	}
	
	if (f = getenv("TMPDIR")) {
		(void)snprintf(name, MAXPATHLEN, "%s%s%sXXXXXX", f,
					   *(f + strlen(f) - 1) == '/'? "": "/", pfx);
		if (f = mktemp(name))
			return(f);
	}
	
	if (f = (char *)dir) {
		(void)snprintf(name, MAXPATHLEN, "%s%s%sXXXXXX", f,
					   *(f + strlen(f) - 1) == '/'? "": "/", pfx);
		if (f = mktemp(name))
			return(f);
	}

	f = P_tmpdir;
	(void)snprintf(name, MAXPATHLEN, "%s%sXXXXXX", f, pfx);
	if (f = mktemp(name))
		return(f);

	f = path_tmp;
	(void)snprintf(name, MAXPATHLEN, "%s%sXXXXXX", f, pfx);
	if (f = mktemp(name))
		return(f);

	save_errno = errno;
	efree(name);
	errno = save_errno;
	return(NULL);
}

#endif

/* }}} */
/* {{{ Module-Stuff */

static void _file_popen_dtor(FILE *pipe)
{
	FIL_FETCH();
	FIL(pclose_ret) = pclose(pipe);
}


static void _file_socket_dtor(int *sock) 
{
	SOCK_FCLOSE(*sock);
#if HAVE_SHUTDOWN
	shutdown(*sock, 0);
#endif
	efree(sock);
}


static void _file_upload_dtor(char *file)
{
	unlink(file);
}


static void _file_fopen_dtor(FILE *fp) 
{
	fclose(fp);
}


PHPAPI int php_file_le_fopen(void) /* XXX doe we really want this???? */
{
	return le_fopen;
}


PHPAPI int php_file_le_socket(void) /* XXX doe we really want this???? */
{
	return le_socket;
}


PHPAPI int php_file_le_uploads(void) /* XXX doe we really want this???? */
{
	return le_uploads;
}


#ifdef ZTS
static void php_file_init_globals(php_file_globals *file_globals)
{
	FIL(fgetss_state) = 0;
	FIL(pclose_ret) = 0;
}
#endif

PHP_MINIT_FUNCTION(file)
{
	le_fopen = register_list_destructors(_file_fopen_dtor, NULL);
	le_popen = register_list_destructors(_file_popen_dtor, NULL);
	le_socket = register_list_destructors(_file_socket_dtor, NULL);
	le_uploads = register_list_destructors(_file_upload_dtor, NULL);

#ifdef ZTS
	file_globals_id = ts_allocate_id(sizeof(php_file_globals), (ts_allocate_ctor) php_file_init_globals, NULL);
#else
	FIL(fgetss_state) = 0;
	FIL(pclose_ret) = 0;
#endif

	return SUCCESS;
}

/* }}} */
/* {{{ proto bool flock(int fp, int operation)
   Portable file locking */

static int flock_values[] = { LOCK_SH, LOCK_EX, LOCK_UN };

PHP_FUNCTION(flock)
{
    pval **arg1, **arg2;
    int type, fd, act;
	void *what;
	
    if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
        WRONG_PARAM_COUNT;
    }
	
	what = zend_fetch_resource(arg1,-1,"File-Handle",&type,3,le_fopen,le_popen,le_socket);
	ZEND_VERIFY_RESOURCE(what);
	
	if (type == le_socket) {
		fd = *(int *) what;
	} else {
		fd = fileno((FILE*) what);
	}
	
    convert_to_long_ex(arg2);

    act = (*arg2)->value.lval & 3;
    if (act < 1 || act > 3) {
		php_error(E_WARNING, "illegal value for second argument");
		RETURN_FALSE;
    }

    /* flock_values contains all possible actions
       if (arg2 & 4) we won't block on the lock */
    act = flock_values[act - 1] | ((*arg2)->value.lval & 4 ? LOCK_NB : 0);
    if (flock(fd, act) == -1) {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

/* }}} */
/* {{{ proto array get_meta_tags(string filename [, int use_include_path])
   Extracts all meta tag content attributes from a file and returns an array */

PHP_FUNCTION(get_meta_tags)
{
	pval **filename, **arg2;
	FILE *fp;
	char buf[8192];
	char buf_lcase[8192];
	int use_include_path = 0;
	int issock=0, socketd=0;
	int len, var_namelen;
	char var_name[50],*val=NULL,*tmp,*end,*slashed;
	PLS_FETCH();
	
	/* check args */
	switch (ARG_COUNT(ht)) {
	case 1:
		if (zend_get_parameters_ex(1,&filename) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 2:
		if (zend_get_parameters_ex(2,&filename,&arg2) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long_ex(arg2);
		use_include_path = (*arg2)->value.lval;
		break;
	default:
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(filename);
	
	fp = php_fopen_wrapper((*filename)->value.str.val,"r", use_include_path|ENFORCE_SAFE_MODE, &issock, &socketd, NULL);
	if (!fp && !socketd) {
		if (issock != BAD_URL) {
			char *tmp = estrdup((*filename)->value.str.val);
			php_strip_url_passwd(tmp);
			php_error(E_WARNING,"get_meta_tags(\"%s\") - %s", tmp, strerror(errno));
			efree(tmp);
		}
		RETURN_FALSE;
	}
	
	if (array_init(return_value)==FAILURE) {
		if (issock) {
			SOCK_FCLOSE(socketd);
		} else {
			fclose(fp);
		}
		RETURN_FALSE;
	}
	/* Now loop through the file and do the magic quotes thing if needed */
	memset(buf, 0, 8191);
	while((FP_FGETS(buf,8191,socketd,fp,issock) != NULL)) {
	   	memcpy(buf_lcase, buf, 8191);
		php_strtolower(buf_lcase, 8191);
		if (php_memnstr(buf_lcase, "</head>", sizeof("</head>")-1, buf_lcase + 8191))
			break;

		if(php_memnstr(buf_lcase, "<meta", sizeof("<meta")-1, buf_lcase + 8191)) {

			memset(var_name,0,50);
			/* get the variable name from the name attribute of the meta tag */
			tmp = php_memnstr(buf_lcase, "name=\"", sizeof("name=\"")-1, buf_lcase + 8191);
			if(tmp) {
				tmp = &buf[tmp - buf_lcase];
				tmp+=6;
				end=strstr(tmp,"\"");
				if(end) {
					unsigned char *c;
					*end='\0';
					snprintf(var_name,50,"%s",tmp);
					*end='"';

					c = (unsigned char*)var_name;
					while (*c) {
						switch(*c) {
							case '.':
							case '\\':
							case '+':
							case '*':
							case '?':
							case '[':
							case '^':
							case ']':
							case '$':
							case '(':
							case ')':
							case ' ':
								*c++ ='_';
								break;
							default:
								*c++ = tolower((unsigned char)*c);
						}
					}
					var_namelen=strlen(var_name);
				}

				/* get the variable value from the content attribute of the meta tag */
				tmp = php_memnstr(buf_lcase, "content=\"", sizeof("content=\"")-1, buf_lcase + 8191);
				if(tmp) {
					tmp = &buf[tmp - buf_lcase];
					tmp+=9;
					end=strstr(tmp,"\"");
					if(end) {
						*end='\0';
						val=estrdup(tmp);
						*end='"';
					}
				}
			}
			if(*var_name && val) {
				if (PG(magic_quotes_runtime)) {
					slashed = php_addslashes(val,0,&len,0);
				} else {
					slashed = estrndup(val,strlen(val));
				}
				add_assoc_string(return_value, var_name, slashed, 0);
				efree(val);
			}
		}
	}
	if (issock) {
		SOCK_FCLOSE(socketd);
	} else {
		fclose(fp);
	}
}

/* }}} */
/* {{{ proto array file(string filename)
   Read entire file into an array */

PHP_FUNCTION(file)
{
	pval **filename, **arg2;
	FILE *fp;
	char *slashed, buf[8192];
	register int i=0;
	int use_include_path = 0;
	int issock=0, socketd=0;
	PLS_FETCH();
	
	/* check args */
	switch (ARG_COUNT(ht)) {
	case 1:
		if (zend_get_parameters_ex(1,&filename) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 2:
		if (zend_get_parameters_ex(2,&filename,&arg2) == FAILURE) {
				WRONG_PARAM_COUNT;
		}
		convert_to_long_ex(arg2);
		use_include_path = (*arg2)->value.lval;
		break;
	default:
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(filename);
	
	fp = php_fopen_wrapper((*filename)->value.str.val,"r", use_include_path|ENFORCE_SAFE_MODE, &issock, &socketd, NULL);
	if (!fp && !socketd) {
		if (issock != BAD_URL) {
			char *tmp = estrdup((*filename)->value.str.val);
			php_strip_url_passwd(tmp);
			php_error(E_WARNING,"file(\"%s\") - %s", tmp, strerror(errno));
			efree(tmp);
		}
		RETURN_FALSE;
	}

	/* Initialize return array */
	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}	
	
	/* Now loop through the file and do the magic quotes thing if needed */
	memset(buf,0,8191);
	while (FP_FGETS(buf,8191,socketd,fp,issock) != NULL) {
		if (PG(magic_quotes_runtime)) {
			int len;
			
			slashed = php_addslashes(buf,0,&len,0); /* 0 = don't free source string */
            add_index_stringl(return_value, i++, slashed, len, 0);
		} else {
			add_index_string(return_value, i++, buf, 1);
		}
	}
	if (issock) {
		SOCK_FCLOSE(socketd);
	} else {
		fclose(fp);
	}
}


/* }}} */
/* {{{ proto string tempnam(string dir, string prefix)
   Create a unique filename in a directory */

PHP_FUNCTION(tempnam)
{
	pval **arg1, **arg2;
	char *d;
	char *t;
	char p[64];
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg1);
	convert_to_string_ex(arg2);
	d = estrndup((*arg1)->value.str.val,(*arg1)->value.str.len);
	strlcpy(p,(*arg2)->value.str.val,sizeof(p));

	t = tempnam(d,p);
	efree(d);
	if(!t) {
		RETURN_FALSE;
	}
	RETURN_STRING(t,1);
}

/* }}} */
/* {{{ proto int tmpfile(void)
   Create a temporary file that will be deleted automatically after use */
PHP_FUNCTION(tmpfile)
{
	FILE *fp;
	if (ARG_COUNT(ht) != 0) {
		WRONG_PARAM_COUNT;
	}
	fp = tmpfile();
	if (fp == NULL) {
		php_error(E_WARNING, "tmpfile: %s", strerror(errno));
		RETURN_FALSE;
	}
	ZEND_REGISTER_RESOURCE(return_value, fp, le_fopen);
}
/* }}} */

/* {{{ proto int fopen(string filename, string mode [, int use_include_path])
   Open a file or a URL and return a file pointer */

PHP_FUNCTION(fopen)
{
	pval **arg1, **arg2, **arg3;
	FILE *fp;
	char *p;
	int *sock;
	int use_include_path = 0;
	int issock=0, socketd=0;
	FIL_FETCH();
	
	switch(ARG_COUNT(ht)) {
	case 2:
		if (zend_get_parameters_ex(2,&arg1,&arg2) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 3:
		if (zend_get_parameters_ex(3,&arg1,&arg2,&arg3) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long_ex(arg3);
		use_include_path = (*arg3)->value.lval;
		break;
	default:
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg1);
	convert_to_string_ex(arg2);
	p = estrndup((*arg2)->value.str.val,(*arg2)->value.str.len);

	/*
	 * We need a better way of returning error messages from
	 * php_fopen_wrapper().
	 */
	fp = php_fopen_wrapper((*arg1)->value.str.val, p, use_include_path|ENFORCE_SAFE_MODE, &issock, &socketd, NULL);
	if (!fp && !socketd) {
		if (issock != BAD_URL) {
			char *tmp = estrdup((*arg1)->value.str.val);
			php_strip_url_passwd(tmp);
			php_error(E_WARNING,"fopen(\"%s\",\"%s\") - %s", tmp, p, strerror(errno));
			efree(tmp);
		}
		efree(p);
		RETURN_FALSE;
	}

	efree(p);
	FIL(fgetss_state)=0;

	if (issock) {
		sock=emalloc(sizeof(int));
		*sock=socketd;
		ZEND_REGISTER_RESOURCE(return_value,sock,le_socket);
	} else {
		ZEND_REGISTER_RESOURCE(return_value,fp,le_fopen);
	}
}

/* }}} */	
/* {{{ proto int fclose(int fp)
   Close an open file pointer */

PHP_FUNCTION(fclose)
{
	pval **arg1;
	int type;
	void *what;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	what = zend_fetch_resource(arg1,-1,"File-Handle",&type,2,le_fopen,le_socket);
	ZEND_VERIFY_RESOURCE(what);
	
	zend_list_delete((*arg1)->value.lval);
	RETURN_TRUE;
}

/* }}} */
/* {{{ proto int popen(string command, string mode)
   Execute a command and open either a read or a write pipe to it */

PHP_FUNCTION(popen)
{
	pval **arg1, **arg2;
	FILE *fp;
	char *p,*tmp = NULL;
	char *b, buf[1024];
	PLS_FETCH();
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg1);
	convert_to_string_ex(arg2);
	p = estrndup((*arg2)->value.str.val,(*arg2)->value.str.len);
	if (PG(safe_mode)){
		b = strchr((*arg1)->value.str.val,' ');
		if (!b) {
			b = strrchr((*arg1)->value.str.val,'/');
		} else {
			char *c;
			c = (*arg1)->value.str.val;
			while((*b!='/')&&(b!=c)) {
				b--;
			}
			if (b==c) {
				b=NULL;
			}
		}
		if (b) {
			snprintf(buf,sizeof(buf),"%s%s",PG(safe_mode_exec_dir),b);
		} else {
			snprintf(buf,sizeof(buf),"%s/%s",PG(safe_mode_exec_dir),(*arg1)->value.str.val);
		}

		tmp = php_escape_shell_cmd(buf);
		fp = popen(tmp,p);
		efree(tmp);

		if (!fp) {
			php_error(E_WARNING,"popen(\"%s\",\"%s\") - %s",buf,p,strerror(errno));
			RETURN_FALSE;
		}
	} else {
		fp = popen((*arg1)->value.str.val,p);
		if (!fp) {
			php_error(E_WARNING,"popen(\"%s\",\"%s\") - %s",(*arg1)->value.str.val,p,strerror(errno));
			efree(p);
			RETURN_FALSE;
		}
	}
	efree(p);

	ZEND_REGISTER_RESOURCE(return_value,fp,le_popen);
}

/* }}} */
/* {{{ proto int pclose(int fp)
   Close a file pointer opened by popen() */

PHP_FUNCTION(pclose)
{
	pval **arg1;
	void *what;
	FIL_FETCH();
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	what = zend_fetch_resource(arg1,-1,"File-Handle",NULL,1,le_popen);
	ZEND_VERIFY_RESOURCE(what);
	
	zend_list_delete((*arg1)->value.lval);
	RETURN_LONG(FIL(pclose_ret));
}

/* }}} */
/* {{{ proto int feof(int fp)
   Test for end-of-file on a file pointer */

PHP_FUNCTION(feof)
{
	pval **arg1;
	int type;
	int issock=0;
	int socketd=0;
	void *what;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	what = zend_fetch_resource(arg1,-1,"File-Handle",&type,3,le_fopen,le_popen,le_socket);
	ZEND_VERIFY_RESOURCE(what);

	if (type == le_socket) {
		issock=1;
		socketd=*(int *) what;
	} 

	if (FP_FEOF(socketd, (FILE*)what, issock)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{ proto int socket_set_blocking(int socket descriptor, int mode)
   Set blocking/non-blocking mode on a socket */
PHPAPI int php_set_sock_blocking(int socketd, int block)
{
      int ret = SUCCESS;
      int flags;
      int myflag = 0;
      
#ifdef PHP_WIN32
      /* with ioctlsocket, a non-zero sets nonblocking, a zero sets blocking */
	  flags = block;
	  if (ioctlsocket(socketd,FIONBIO,&flags)==SOCKET_ERROR){
		  php_error(E_WARNING,"%s",WSAGetLastError());
		  ret = FALSE;
      }
#else
      flags = fcntl(socketd, F_GETFL);
#ifdef O_NONBLOCK
      myflag = O_NONBLOCK; /* POSIX version */
#elif defined(O_NDELAY)
      myflag = O_NDELAY;   /* old non-POSIX version */
#endif
      if (!block) {
              flags |= myflag;
      } else {
              flags &= ~myflag;
      }
      fcntl(socketd, F_SETFL, flags);
#endif
      return ret;
}

PHP_FUNCTION(socket_set_blocking)
{
	pval **arg1, **arg2;
	int block;
	int socketd = 0;
	void *what;
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	what = zend_fetch_resource(arg1,-1,"File-Handle",NULL,1,le_socket);
	ZEND_VERIFY_RESOURCE(what);

	convert_to_long_ex(arg2);
	block = (*arg2)->value.lval;
	
	socketd = *(int*)what;

	if (php_set_sock_blocking(socketd, block) == FAILURE)
		RETURN_FALSE;

	php_sockset_blocking(socketd, block == 0 ? 0 : 1);
	
	RETURN_TRUE;
}

/* }}} */

PHP_FUNCTION(set_socket_blocking)
{
	php_error(E_NOTICE, "set_socket_blocking() is deprecated, use socket_set_blocking() instead");
	PHP_FN(socket_set_blocking)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

/* {{{ proto bool socket_set_timeout(int socket descriptor, int seconds, int microseconds)
   Set timeout on socket read to seconds + microseonds */
#if HAVE_SYS_TIME_H
PHP_FUNCTION(socket_set_timeout)
{
	zval **socket, **seconds, **microseconds;
	int type;
	void *what;
	int socketd = 0;
	struct timeval t;

	if (ZEND_NUM_ARGS() < 2 || ZEND_NUM_ARGS() > 3 ||
		zend_get_parameters_ex(ZEND_NUM_ARGS(), &socket, &seconds, &microseconds)==FAILURE) {
		WRONG_PARAM_COUNT;
	}

	what = zend_fetch_resource(socket, -1, "File-Handle", &type, 1, le_socket);
	ZEND_VERIFY_RESOURCE(what);
	socketd = *(int *)what;
	
	convert_to_long_ex(seconds);
	t.tv_sec = (*seconds)->value.lval;

	if (ZEND_NUM_ARGS() == 3) {
		convert_to_long_ex(microseconds);
		t.tv_usec = (*microseconds)->value.lval % 1000000;
		t.tv_sec += (*microseconds)->value.lval / 1000000;
	}
	else
		t.tv_usec = 0;

	php_sockset_timeout(socketd, &t);
	RETURN_TRUE;
}
#endif /* HAVE_SYS_TIME_H */

/* }}} */


/* {{{ proto array socket_get_status(resource socket_descriptor)
   Return an array describing socket status */
PHP_FUNCTION(socket_get_status)
{
	zval **socket;
	int type;
	void *what;
	int socketd = 0;
	struct php_sockbuf *sock;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(ZEND_NUM_ARGS(), &socket) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	what = zend_fetch_resource(socket, -1, "File-Handle", &type, 1, le_socket);
	ZEND_VERIFY_RESOURCE(what);
	socketd = *(int *)what;
	sock = php_get_socket(socketd);

	array_init(return_value);

	add_assoc_bool(return_value, "timed_out", sock->timeout_event);
	add_assoc_bool(return_value, "blocked", sock->is_blocked);
	add_assoc_bool(return_value, "eof", sock->eof);
	add_assoc_long(return_value, "unread_bytes", sock->writepos - sock->readpos);
}
/* }}} */


/* {{{ proto string fgets(int fp, int length)
   Get a line from file pointer */

PHP_FUNCTION(fgets)
{
	pval **arg1, **arg2;
	int len, type;
	char *buf;
	int issock=0;
	int socketd=0;
	void *what;
	PLS_FETCH();
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	what = zend_fetch_resource(arg1,-1,"File-Handle",&type,3,le_fopen,le_popen,le_socket);
	ZEND_VERIFY_RESOURCE(what);

	convert_to_long_ex(arg2);
	len = (*arg2)->value.lval;

	if (type == le_socket) {
		issock=1;
		socketd=*(int*)what;
	}
	buf = emalloc(sizeof(char) * (len + 1));
	/* needed because recv doesnt put a null at the end*/
	memset(buf,0,len+1);
	if (FP_FGETS(buf, len, socketd, (FILE*)what, issock) == NULL) {
		efree(buf);
		RETVAL_FALSE;
	} else {
		if (PG(magic_quotes_runtime)) {
			return_value->value.str.val = php_addslashes(buf,0,&return_value->value.str.len,1);
		} else {
			return_value->value.str.val = buf;
			return_value->value.str.len = strlen(return_value->value.str.val);
		}
		return_value->type = IS_STRING;
	}
}

/* }}} */
/* {{{ proto string fgetc(int fp)
   Get a character from file pointer */

PHP_FUNCTION(fgetc) {
	pval **arg1;
	int type;
	char *buf;
	int issock=0;
	int socketd=0;
	void *what;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	what = zend_fetch_resource(arg1,-1,"File-Handle",&type,3,le_fopen,le_popen,le_socket);
	ZEND_VERIFY_RESOURCE(what);

	if (type == le_socket) {
		issock=1;
		socketd=*(int*)what;
	}

	buf = emalloc(sizeof(char) * 2);
	if (!(*buf = FP_FGETC(socketd, (FILE*)what, issock))) {
		efree(buf);
		RETVAL_FALSE;
	} else {
		buf[1]='\0';
		return_value->value.str.val = buf; 
		return_value->value.str.len = 1; 
		return_value->type = IS_STRING;
	}
}

/* }}} */
/* {{{ proto string fgetss(int fp, int length [, string allowable_tags])
   Get a line from file pointer and strip HTML tags */

PHP_FUNCTION(fgetss)
{
	pval **fd, **bytes, **allow=NULL;
	int len, type;
	char *buf;
	int issock=0;
	int socketd=0;
	void *what;
	char *allowed_tags=NULL;
	int allowed_tags_len=0;
	FIL_FETCH();

	switch(ARG_COUNT(ht)) {
	case 2:
		if (zend_get_parameters_ex(2, &fd, &bytes) == FAILURE) {
			RETURN_FALSE;
		}
		break;
	case 3:
		if (zend_get_parameters_ex(3, &fd, &bytes, &allow) == FAILURE) {
			RETURN_FALSE;
		}
		convert_to_string_ex(allow);
		allowed_tags = (*allow)->value.str.val;
		allowed_tags_len = (*allow)->value.str.len;
		break;
	default:
		WRONG_PARAM_COUNT;
		/* NOTREACHED */
		break;
	}

	what = zend_fetch_resource(fd,-1,"File-Handle",&type,3,le_fopen,le_popen,le_socket);
	ZEND_VERIFY_RESOURCE(what);

	if (type == le_socket) {
		issock=1;
		socketd=*(int*)what;
	}

	convert_to_long_ex(bytes);
	len = (*bytes)->value.lval;

	buf = emalloc(sizeof(char) * (len + 1));
	/*needed because recv doesnt set null char at end*/
	memset(buf, 0, len + 1);
	if (FP_FGETS(buf, len, socketd, (FILE*)what, issock) == NULL) {
		efree(buf);
		RETURN_FALSE;
	}

	/* strlen() can be used here since we are doing it on the return of an fgets() anyway */
	php_strip_tags(buf, strlen(buf), FIL(fgetss_state), allowed_tags, allowed_tags_len);

	RETURN_STRING(buf, 0);
}

/* }}} */
/* {{{ proto int fwrite(int fp, string str [, int length])
   Binary-safe file write */

PHP_FUNCTION(fwrite)
{
	pval **arg1, **arg2, **arg3=NULL;
	int ret,type;
	int num_bytes;
	int issock=0;
	int socketd=0;
	void *what;
	PLS_FETCH();

	switch (ARG_COUNT(ht)) {
	case 2:
		if (zend_get_parameters_ex(2, &arg1, &arg2)==FAILURE) {
			RETURN_FALSE;
		}
		convert_to_string_ex(arg2);
		num_bytes = (*arg2)->value.str.len;
		break;
	case 3:
		if (zend_get_parameters_ex(3, &arg1, &arg2, &arg3)==FAILURE) {
			RETURN_FALSE;
		}
		convert_to_string_ex(arg2);
		convert_to_long_ex(arg3);
		num_bytes = MIN((*arg3)->value.lval, (*arg2)->value.str.len);
		break;
	default:
		WRONG_PARAM_COUNT;
		/* NOTREACHED */
		break;
	}				

	what = zend_fetch_resource(arg1,-1,"File-Handle",&type,3,le_fopen,le_popen,le_socket);
	ZEND_VERIFY_RESOURCE(what);

	if (type == le_socket) {
		issock=1;
		socketd=*(int*)what;
	}

	if (!arg3 && PG(magic_quotes_runtime)) {
		zval_copy_ctor(*arg2);
		php_stripslashes((*arg2)->value.str.val,&num_bytes);
	}

	if (issock){
		ret = SOCK_WRITEL((*arg2)->value.str.val,num_bytes,socketd);
	} else {
		ret = fwrite((*arg2)->value.str.val,1,num_bytes,(FILE*)what);
	}
	RETURN_LONG(ret);
}

/* }}} */	
/* {{{ proto int set_file_buffer(int fp, int buffer)
   Set file write buffer */

PHP_FUNCTION(set_file_buffer)
{
	pval **arg1, **arg2;
	int ret,type,buff;
	void *what;
      
	switch (ARG_COUNT(ht)) {
	case 2:
		if (zend_get_parameters_ex(2, &arg1, &arg2)==FAILURE) {
			RETURN_FALSE;
		} 
		break;
	default:
		WRONG_PARAM_COUNT;
		/* NOTREACHED */
		break;
	}                               

	what = zend_fetch_resource(arg1,-1,"File-Handle",&type,2,le_fopen,le_popen);
	ZEND_VERIFY_RESOURCE(what);

	convert_to_long_ex(arg2);
	buff = (*arg2)->value.lval;    

	/* if buff is 0 then set to non-buffered */
	if (buff == 0){
		ret = setvbuf((FILE*)what, NULL, _IONBF, 0);
	} else {
		ret = setvbuf((FILE*)what, NULL, _IOFBF, buff);
	}

	RETURN_LONG(ret);
}

/* }}} */     
/* {{{ proto int rewind(int fp)
   Rewind the position of a file pointer */

PHP_FUNCTION(rewind)
{
	pval **arg1;
	void *what;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	what = zend_fetch_resource(arg1,-1,"File-Handle",NULL,2,le_fopen,le_popen);
	ZEND_VERIFY_RESOURCE(what);
	
	rewind((FILE*) what);
	RETURN_TRUE;
}

/* }}} */
/* {{{ proto int ftell(int fp)
   Get file pointer's read/write position */

PHP_FUNCTION(ftell)
{
	pval **arg1;
	void *what;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	what = zend_fetch_resource(arg1,-1,"File-Handle",NULL,2,le_fopen,le_popen);
	ZEND_VERIFY_RESOURCE(what);
	
	RETURN_LONG(ftell((FILE*) what));
}

/* }}} */
/* {{{ proto int fseek(int fp, int offset)
   Seek on a file pointer */

PHP_FUNCTION(fseek)
{
	pval **arg1, **arg2;
	void *what;
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	what = zend_fetch_resource(arg1,-1,"File-Handle",NULL,2,le_fopen,le_popen);
	ZEND_VERIFY_RESOURCE(what);

	convert_to_long_ex(arg2);
	
	RETURN_LONG(fseek((FILE*)what,(*arg2)->value.lval,SEEK_SET));
}

/* }}} */
/* {{{ proto int mkdir(string pathname, int mode)
   Create a directory */

PHP_FUNCTION(mkdir)
{
	pval **arg1, **arg2;
	int ret,mode;
	PLS_FETCH();
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg1);
	convert_to_long_ex(arg2);
	mode = (*arg2)->value.lval;
	if (PG(safe_mode) &&(!php_checkuid((*arg1)->value.str.val,3))) {
		RETURN_FALSE;
	}
	ret = mkdir((*arg1)->value.str.val,mode);
	if (ret < 0) {
		php_error(E_WARNING,"MkDir failed (%s)", strerror(errno));
		RETURN_FALSE;
	}
	RETURN_TRUE;
}

/* }}} */	
/* {{{ proto int rmdir(string dirname)
   Remove a directory */

PHP_FUNCTION(rmdir)
{
	pval **arg1;
	int ret;
	PLS_FETCH();
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg1);
	if (PG(safe_mode) &&(!php_checkuid((*arg1)->value.str.val,1))) {
		RETURN_FALSE;
	}
	ret = rmdir((*arg1)->value.str.val);
	if (ret < 0) {
		php_error(E_WARNING,"RmDir failed (%s)", strerror(errno));
		RETURN_FALSE;
	}
	RETURN_TRUE;
}

/* }}} */	
/* {{{ php_passthru_fd */

static size_t php_passthru_fd(int socketd, FILE *fp, int issock)
{
	size_t bcount = 0;
	int ready = 0;
	char buf[8192];
	
#ifdef HAVE_MMAP 
	if(!issock) {
		int fd;
		struct stat sbuf;
		off_t off;
		void *p;
		size_t len;

		fd = fileno(fp);
		fstat(fd, &sbuf);
	
		if(sbuf.st_size > sizeof(buf)) {
			off = ftell(fp);
			len = sbuf.st_size - off;
			p = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, off);
			if(p!=MAP_FAILED) {
				PHPWRITE(p, len);
				munmap(p, len);
				bcount += len;
				ready = 1;
			}
		}
	}
#endif

	if(!ready) {
		int b;

		while ((b = FP_FREAD(buf, sizeof(buf), socketd, fp, issock)) > 0) {
			PHPWRITE(buf, b);
			bcount += b;
		}
	}
		
	return bcount;
}

/* }}} */
/* {{{ proto int readfile(string filename [, int use_include_path])
   Output a file or a URL */

PHP_FUNCTION(readfile)
{
	pval **arg1, **arg2;
	FILE *fp;
	int size=0;
	int use_include_path=0;
	int issock=0, socketd=0;
	
	/* check args */
	switch (ARG_COUNT(ht)) {
	case 1:
		if (zend_get_parameters_ex(1,&arg1) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 2:
		if (zend_get_parameters_ex(2,&arg1,&arg2) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long_ex(arg2);
		use_include_path = (*arg2)->value.lval;
		break;
	default:
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg1);

	/*
	 * We need a better way of returning error messages from
	 * php_fopen_wrapper().
	 */
	fp = php_fopen_wrapper((*arg1)->value.str.val,"r", use_include_path|ENFORCE_SAFE_MODE, &issock, &socketd, NULL);
	if (!fp && !socketd){
		if (issock != BAD_URL) {
			char *tmp = estrdup((*arg1)->value.str.val);
			php_strip_url_passwd(tmp);
			php_error(E_WARNING,"readfile(\"%s\") - %s", tmp, strerror(errno));
			efree(tmp);
		}
		RETURN_FALSE;
	}
	if (php_header()) {
		size = php_passthru_fd(socketd, fp, issock);
	}
	if (issock) {
		SOCK_FCLOSE(socketd);
	} else {
		fclose(fp);
	}
	RETURN_LONG(size);
}

/* }}} */
/* {{{ proto int umask([int mask])
   Return or change the umask */

PHP_FUNCTION(umask)
{
	pval **arg1;
	int oldumask;
	int arg_count = ARG_COUNT(ht);
	
	oldumask = umask(077);

	if (arg_count == 0) {
		umask(oldumask);
	} else {
		if (arg_count > 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long_ex(arg1);
		umask((*arg1)->value.lval);
	}

	/* XXX we should maybe reset the umask after each request! */

	RETURN_LONG(oldumask);
}

/* }}} */
/* {{{ proto int fpassthru(int fp)
   Output all remaining data from a file pointer */

PHP_FUNCTION(fpassthru)
{
	pval **arg1;
	int size, type;
	int issock=0;
	int socketd=0;
	void *what;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	what = zend_fetch_resource(arg1,-1,"File-Handle",&type,3,le_fopen,le_popen,le_socket);
	ZEND_VERIFY_RESOURCE(what);

	if (type == le_socket) {
		issock=1;
		socketd=*(int*)what;
	}

	size = 0;
	if (php_header()) { /* force headers if not already sent */
		size = php_passthru_fd(socketd, (FILE*) what, issock);
	}

	zend_list_delete((*arg1)->value.lval);
	RETURN_LONG(size);
}

/* }}} */
/* {{{ proto int rename(string old_name, string new_name)
   Rename a file */

PHP_FUNCTION(rename)
{
	pval **old_arg, **new_arg;
	char *old_name, *new_name;
	int ret;
	PLS_FETCH();
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &old_arg, &new_arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(old_arg);
	convert_to_string_ex(new_arg);

	old_name = (*old_arg)->value.str.val;
	new_name = (*new_arg)->value.str.val;

	if (PG(safe_mode) &&(!php_checkuid(old_name, 2))) {
		RETURN_FALSE;
	}
	ret = rename(old_name, new_name);

	if (ret == -1) {
		php_error(E_WARNING,"Rename failed (%s)", strerror(errno));
		RETURN_FALSE;
	}

	RETVAL_TRUE;
}

/* }}} */
/* {{{ proto int ftruncate (int fp, int size)
   Truncate file to 'size' length */
PHP_FUNCTION(ftruncate)
{
	zval **fp , **size;
	short int ret;
	int type;
	void *what;

	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &fp, &size) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	what = zend_fetch_resource(fp,-1,"File-Handle",&type,3,le_fopen,le_popen,le_socket);
	ZEND_VERIFY_RESOURCE(what);

	if (type == le_socket)
	{
		php_error(E_WARNING, "can't truncate sockets!");
		RETURN_FALSE;
	}

	convert_to_long_ex(size);

	ret = ftruncate(fileno((FILE *)what), (*size)->value.lval);
	RETURN_LONG(ret);
}
/* }}} */

/* {{{ proto int fstat (int fp)
   Stat() on a filehandle */
PHP_FUNCTION(fstat)
{
	zval **fp;
	int type;
	void *what;
	struct stat stat_sb;

	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &fp) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	what = zend_fetch_resource(fp,-1,"File-Handle",&type,3,le_fopen,le_popen,le_socket);
	ZEND_VERIFY_RESOURCE(what);

	if (fstat(fileno((FILE *) what ), &stat_sb)) {
		RETURN_FALSE;
	}

	array_init(return_value);

	add_assoc_long ( return_value , "dev" , stat_sb.st_dev );
	add_assoc_long ( return_value , "ino" , stat_sb.st_ino );
	add_assoc_long ( return_value , "mode" , stat_sb.st_mode );
	add_assoc_long ( return_value , "nlink" , stat_sb.st_nlink );
	add_assoc_long ( return_value , "uid" , stat_sb.st_uid );
	add_assoc_long ( return_value , "gid" , stat_sb.st_gid );

#ifdef HAVE_ST_BLKSIZE
	add_assoc_long ( return_value, "rdev" , stat_sb.st_rdev );
	add_assoc_long ( return_value , "blksize" , stat_sb.st_blksize );
#endif
	
	add_assoc_long ( return_value , "size" , stat_sb.st_size );
	add_assoc_long ( return_value , "atime" , stat_sb.st_atime );
	add_assoc_long ( return_value , "mtime" , stat_sb.st_mtime );
	add_assoc_long ( return_value , "ctime" , stat_sb.st_ctime );

#ifdef HAVE_ST_BLOCKS
	add_assoc_long ( return_value , "blocks" , stat_sb.st_blocks );
#endif
}
/* }}} */


/* {{{ proto int copy(string source_file, string destination_file)
   Copy a file */

PHP_FUNCTION(copy)
{
	pval **source, **target;
	char buffer[8192];
	int fd_s,fd_t,read_bytes;
	PLS_FETCH();
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &source, &target) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(source);
	convert_to_string_ex(target);

	if (PG(safe_mode) &&(!php_checkuid((*source)->value.str.val,2))) {
		RETURN_FALSE;
	}
	
#ifdef PHP_WIN32
	if ((fd_s=open((*source)->value.str.val,O_RDONLY|_O_BINARY))==-1) {
#else
	if ((fd_s=open((*source)->value.str.val,O_RDONLY))==-1) {
#endif
		php_error(E_WARNING,"Unable to open '%s' for reading:  %s",(*source)->value.str.val,strerror(errno));
		RETURN_FALSE;
	}
#ifdef PHP_WIN32
	if ((fd_t=open((*target)->value.str.val,_O_WRONLY|_O_CREAT|_O_TRUNC|_O_BINARY,_S_IREAD|_S_IWRITE))==-1){
#else
	if ((fd_t=creat((*target)->value.str.val,0777))==-1) {
#endif
		php_error(E_WARNING,"Unable to create '%s':  %s", (*target)->value.str.val,strerror(errno));
		close(fd_s);
		RETURN_FALSE;
	}

	while ((read_bytes=read(fd_s,buffer,8192))!=-1 && read_bytes!=0) {
		if (write(fd_t,buffer,read_bytes)==-1) {
			php_error(E_WARNING,"Unable to write to '%s':  %s",(*target)->value.str.val,strerror(errno));
			close(fd_s);
			close(fd_t);
			RETURN_FALSE;
		}
	}
	
	close(fd_s);
	close(fd_t);

	RETVAL_TRUE;
}

/* }}} */
/* {{{ proto int fread(int fp, int length)
   Binary-safe file read */

PHP_FUNCTION(fread)
{
	pval **arg1, **arg2;
	int len, type;
	int issock=0;
	int socketd=0;
	void *what;
	PLS_FETCH();
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	what = zend_fetch_resource(arg1,-1,"File-Handle",&type,3,le_fopen,le_popen,le_socket);
	ZEND_VERIFY_RESOURCE(what);

	if (type == le_socket) {
		issock=1;
		socketd=*(int*)what;
	}

	convert_to_long_ex(arg2);
	len = (*arg2)->value.lval;

	return_value->value.str.val = emalloc(sizeof(char) * (len + 1));
	/* needed because recv doesnt put a null at the end*/
	
	if (!issock) {
		return_value->value.str.len = fread(return_value->value.str.val, 1, len, (FILE*)what);
		return_value->value.str.val[return_value->value.str.len] = 0;
	} else {
		return_value->value.str.len = SOCK_FREAD(return_value->value.str.val, len, socketd);
	}
	if (PG(magic_quotes_runtime)) {
		return_value->value.str.val = php_addslashes(return_value->value.str.val,return_value->value.str.len,&return_value->value.str.len,1);
	}
	return_value->type = IS_STRING;
}

/* }}} */
/* {{{ proto array fgetcsv(int fp, int length)
   Get line from file pointer and parse for CSV fields */
 
PHP_FUNCTION(fgetcsv) {
	char *temp, *tptr, *bptr;
	char delimiter = ',';	/* allow this to be set as parameter */

	/* first section exactly as php_fgetss */

	pval **fd, **bytes, **p_delim;
	int len, type;
	char *buf;
	int issock=0;
	int socketd=0;
	void *what;
	
	switch(ARG_COUNT(ht)) {
	case 2:
		if (zend_get_parameters_ex(2, &fd, &bytes) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
		
	case 3:
		if (zend_get_parameters_ex(3, &fd, &bytes, &p_delim) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_string_ex(p_delim);
		/* Make sure that there is at least one character in string */
		if ((*p_delim)->value.str.len < 1) {
			WRONG_PARAM_COUNT;
		}
			/* use first character from string */
		delimiter = (*p_delim)->value.str.val[0];
		break;
		
	default:
		WRONG_PARAM_COUNT;
		/* NOTREACHED */
		break;
	}

	what = zend_fetch_resource(fd,-1,"File-Handle",&type,3,le_fopen,le_popen,le_socket);
	ZEND_VERIFY_RESOURCE(what);

	if (type == le_socket) {
		issock=1;
		socketd=*(int*)what;
	}

	convert_to_long_ex(bytes);
	len = (*bytes)->value.lval;

	buf = emalloc(sizeof(char) * (len + 1));
	/*needed because recv doesnt set null char at end*/
	memset(buf, 0, len + 1);
	if (FP_FGETS(buf, len, socketd, (FILE*)what, issock) == NULL) {
		efree(buf);
		RETURN_FALSE;
	}

	/* Now into new section that parses buf for comma/quote delimited fields */

	/* Strip trailing space from buf */

	bptr = buf;
	tptr = buf + strlen(buf) -1;
	while ( isspace((int)*tptr) && (tptr > bptr) ) *tptr--=0;

	/* add single space - makes it easier to parse trailing null field */
	*++tptr = ' ';
	*++tptr = 0;

	/* reserve workspace for building each individual field */

	temp = emalloc(sizeof(char) * len);	/*	unlikely but possible! */
	tptr = temp;

	/* Initialize return array */
	if (array_init(return_value) == FAILURE) {
		efree(temp);
		efree(buf);
		RETURN_FALSE;
	}

	/* Main loop to read CSV fields */
	/* NB this routine will return a single null entry for a blank line */

	do	{
		/* 1. Strip any leading space */		
		while(isspace((int)*bptr)) bptr++;	
		/* 2. Read field, leaving bptr pointing at start of next field */
		if (*bptr == '"') {
			/* 2A. handle quote delimited field */
			bptr++;		/* move on to first character in field */
			while (*bptr) {
				if (*bptr == '"') {
					/* handle the double-quote */
					if ( *(bptr+1) == '"') {
					/* embedded double quotes */
						*tptr++ = *bptr; bptr +=2;
					} else {
					/* must be end of string - skip to start of next field or end */
						while ( (*bptr != delimiter) && *bptr ) bptr++;
						if (*bptr == delimiter) bptr++;
						*tptr=0;	/* terminate temporary string */
						break;	/* .. from handling this field - resumes at 3. */
					}
				} else {
				/* normal character */
					*tptr++ = *bptr++;
				}
			}
		} else {
			/* 2B. Handle non-quoted field */
			while ( (*bptr != delimiter) && *bptr ) *tptr++ = *bptr++;
			*tptr=0;	/* terminate temporary string */
			if (strlen(temp)) {
				tptr--;
				while (isspace((int)*tptr)) *tptr-- = 0;	/* strip any trailing spaces */
			}
			if (*bptr == delimiter) bptr++;
		}
		/* 3. Now pass our field back to php */
		add_next_index_string(return_value, temp, 1);
		tptr=temp;
	} while (*bptr);
	efree(temp);
	efree(buf);
}

/* }}} */

/* {{{ proto string realpath(string path)
   Return the resolved path */
PHP_FUNCTION(realpath)
{
	zval **path;
	char resolved_path[MAXPATHLEN];

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(ZEND_NUM_ARGS(), &path) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(path);
	if (php_realpath((*path)->value.str.val, resolved_path)) {
		RETURN_STRING(resolved_path, 1);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

#if 0

static fd_set readfd;
static int max_fd;

PHP_FUNCTION(fd_set)
{
        pval **arg;
        void *what;
        int type, fd;

        if(ARG_COUNT(ht) <= 0) {
                php_error(E_WARNING, "fd_set: Must be passed at least one value" );
                var_uninit(return_value);
                return;
        }
        else if(ARG_COUNT(ht) == 1) {
                if(zend_get_parameters_ex(1, &arg) == FAILURE) {
                        WRONG_PARAM_COUNT;
                }
        what = zend_fetch_resource(arg,-1,"select",&type,3,le_fopen,le_socket,le_popen);
        ZEND_VERIFY_RESOURCE(what);
        if(type == le_socket) {
                fd = *(int *)what;
        } else {
                fd = fileno((FILE *)what);
        }
        max_fd = fd;
        FD_ZERO(&readfd);
        FD_SET(max_fd, &readfd);
        }
        else {
                pval ***args = (pval ***) emalloc(sizeof(pval **) * ARG_COUNT(ht));
                int i;
                if(zend_get_parameters_array_ex(ARG_COUNT(ht), args) == FAILURE) {
                        efree(args);
                        WRONG_PARAM_COUNT;
                }
                FD_ZERO(&readfd);
                for(i = 0; i < ARG_COUNT(ht); i++) {
                        what = zend_fetch_resource(*args,-1,"select",&type,3,le_fopen,le_socket,le_popen);
                        ZEND_VERIFY_RESOURCE(what);
                        if(type == le_socket) {
                                fd = *(int *)what;
                        } else {
                                fd = fileno((FILE *)what);
                        }
                FD_SET(fd, &readfd);
                if(fd > max_fd) max_fd = fd;
		}
	efree(args);
	}
	RETURN_LONG(1);
}

PHP_FUNCTION(select)
{
	pval **timeout;
	struct timeval tv;

	if(ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &timeout) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(timeout);

	tv.tv_sec = (*timeout)->value.lval / 1000000;
	tv.tv_usec = (*timeout)->value.lval % 1000000;

	RETURN_LONG(select(max_fd + 1,&readfd,NULL,NULL,((*timeout)->value.lval <= 0) ? NULL : &tv));
}

PHP_FUNCTION(fd_isset)
{
	pval **fdarg;
	void *what;
	int type, fd;

	if(ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &fdarg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	what = zend_fetch_resource(fdarg,-1,"select",&type,3,le_fopen,le_socket,le_popen);
	ZEND_VERIFY_RESOURCE(what);

	if(type == le_socket) {
		fd = *(int *)what;
	} else {
		fd = fileno((FILE *)what);
	}

	if(FD_ISSET(fd,&readfd)) {
		RETURN_TRUE;
	}	
	RETURN_FALSE;
}	

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
