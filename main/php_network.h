/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2003 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Stig Venaas <venaas@uninett.no>                              |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifndef _PHP_NETWORK_H
#define _PHP_NETWORK_H

#ifdef PHP_WIN32
# ifndef WINNT
#  define WINNT 1
# endif
# undef FD_SETSIZE
# include "arpa/inet.h"
  /* Apache folks decided that strtoul was evil and redefined
   * it to something that breaks the windows headers */
# undef strtoul
/* defines socklen_t and some IPV6 stuff */
# include <ws2tcpip.h>
# if HAVE_WSPIAPI_H
   /* getaddrinfo */
#  include <wspiapi.h>
# endif
#else
# undef closesocket
# define closesocket close
#endif

#ifndef HAVE_SHUTDOWN
#undef shutdown
#define shutdown(s,n)	/* nothing */
#endif

#ifdef PHP_WIN32
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS	WSAEWOULDBLOCK
#	define fsync _commit
#	define ftruncate(a, b) chsize(a, b)
#endif /* defined(PHP_WIN32) */

#ifdef PHP_WIN32
#define php_socket_errno() WSAGetLastError()
#else
#define php_socket_errno() errno
#endif

/* like strerror, but caller must efree the returned string,
 * unless buf is not NULL.
 * Also works sensibly for win32 */
PHPAPI char *php_socket_strerror(long err, char *buf, size_t bufsize);

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

/* These are here, rather than with the win32 counterparts above,
 * since <sys/socket.h> defines them. */
#ifndef SHUT_RD
# define SHUT_RD 0
# define SHUT_WR 1
# define SHUT_RDWR 2
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#ifdef PHP_WIN32
typedef SOCKET php_socket_t;
#else
typedef int php_socket_t;
#endif

#ifdef PHP_WIN32
# define SOCK_ERR INVALID_SOCKET
# define SOCK_CONN_ERR SOCKET_ERROR
# define SOCK_RECV_ERR SOCKET_ERROR
#else
# define SOCK_ERR -1
# define SOCK_CONN_ERR -1
# define SOCK_RECV_ERR -1
#endif

#define PHP_SOCK_CHUNK_SIZE	8192

#ifdef HAVE_SOCKADDR_STORAGE
typedef struct sockaddr_storage php_sockaddr_storage;
#else
typedef struct {
#ifdef HAVE_SOCKADDR_LEN
		unsigned char ss_len;
		unsigned char ss_family;
#else
        unsigned short ss_family;
#endif
        char info[126];
} php_sockaddr_storage;
#endif

PHPAPI php_socket_t php_network_connect_socket_to_host(const char *host, unsigned short port,
		int socktype, int asynchronous, struct timeval *timeout, char **error_string,
		int *error_code
		TSRMLS_DC);

PHPAPI int php_network_connect_socket(php_socket_t sockfd,
		const struct sockaddr *addr,
		socklen_t addrlen,
		int asynchronous,
		struct timeval *timeout,
		char **error_string,
		int *error_code);

#define php_connect_nonb(sock, addr, addrlen, timeout) \
	php_network_connect_socket((sock), (addr), (addrlen), 0, (timeout), NULL, NULL)

PHPAPI php_socket_t php_network_bind_socket_to_local_addr(const char *host, unsigned port,
		int socktype, char **error_string, int *error_code
		TSRMLS_DC);

PHPAPI php_socket_t php_network_accept_incoming(php_socket_t srvsock,
		char **textaddr, long *textaddrlen,
		struct sockaddr **addr,
		socklen_t *addrlen,
		struct timeval *timeout,
		char **error_string,
		int *error_code
		TSRMLS_DC);

PHPAPI int php_network_get_sock_name(php_socket_t sock, 
		char **textaddr, long *textaddrlen,
		struct sockaddr **addr,
		socklen_t *addrlen
		TSRMLS_DC);
	
PHPAPI int php_network_get_peer_name(php_socket_t sock, 
		char **textaddr, long *textaddrlen,
		struct sockaddr **addr,
		socklen_t *addrlen
		TSRMLS_DC);

PHPAPI void php_any_addr(int family, php_sockaddr_storage *addr, unsigned short port);
PHPAPI int php_sockaddr_size(php_sockaddr_storage *addr);


struct _php_netstream_data_t	{
	php_socket_t socket;
	char is_blocked;
	struct timeval timeout;
	char timeout_event;
	size_t ownsize;
};
typedef struct _php_netstream_data_t php_netstream_data_t;
PHPAPI extern php_stream_ops php_stream_socket_ops;
extern php_stream_ops php_stream_generic_socket_ops;
#define PHP_STREAM_IS_SOCKET	(&php_stream_socket_ops)

PHPAPI php_stream *_php_stream_sock_open_from_socket(php_socket_t socket, const char *persistent_id STREAMS_DC TSRMLS_DC );
/* open a connection to a host using php_hostconnect and return a stream */
PHPAPI php_stream *_php_stream_sock_open_host(const char *host, unsigned short port,
		int socktype, struct timeval *timeout, const char *persistent_id STREAMS_DC TSRMLS_DC);
PHPAPI void php_network_populate_name_from_sockaddr(
		/* input address */
		struct sockaddr *sa, socklen_t sl,
		/* output readable address */
		char **textaddr, long *textaddrlen,
		/* output address */
		struct sockaddr **addr,
		socklen_t *addrlen
		TSRMLS_DC);

PHPAPI int php_network_parse_network_address_with_port(const char *addr,
		long addrlen, struct sockaddr *sa, socklen_t *sl TSRMLS_DC);

#define php_stream_sock_open_from_socket(socket, persistent)	_php_stream_sock_open_from_socket((socket), (persistent) STREAMS_CC TSRMLS_CC)
#define php_stream_sock_open_host(host, port, socktype, timeout, persistent)	_php_stream_sock_open_host((host), (port), (socktype), (timeout), (persistent) STREAMS_CC TSRMLS_CC)

/* {{{ memory debug */
#define php_stream_sock_open_from_socket_rel(socket, persistent)	_php_stream_sock_open_from_socket((socket), (persistent) STREAMS_REL_CC TSRMLS_CC)
#define php_stream_sock_open_host_rel(host, port, socktype, timeout, persistent)	_php_stream_sock_open_host((host), (port), (socktype), (timeout), (persistent) STREAMS_REL_CC TSRMLS_CC)
#define php_stream_sock_open_unix_rel(path, pathlen, persistent, timeval)	_php_stream_sock_open_unix((path), (pathlen), (persistent), (timeval) STREAMS_REL_CC TSRMLS_CC)

/* }}} */

#endif /* _PHP_NETWORK_H */

/*
 * Local variables:
 * tab-width: 8
 * c-basic-offset: 8
 * End:
 */
