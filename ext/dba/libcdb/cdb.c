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
   | Author: Marcus Boerger <helly@php.net>                               |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

/* incorporated from D.J.Bernstein's cdb-0.75 (http://cr.yp.to/cdb.html)*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifndef PHP_WIN32
#include <sys/mman.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>
#include "cdb.h"

#ifndef EPROTO
# define EPROTO -15  /* cdb 0.75's default for PROTOless systems */
#endif

/* {{{ cdb_match */
static int cdb_match(struct cdb *c, char *key, unsigned int len, uint32 pos TSRMLS_DC)
{
	char buf[32];
	int n;

	while (len > 0) {
		n = sizeof(buf);
		if (n > len) 
			n = len;
		if (cdb_read(c, buf, n, pos TSRMLS_CC) == -1) 
			return -1;
		if (memcmp(buf, key, n)) 
			return 0;
		pos += n;
		key += n;
		len -= n;
	}
	return 1;
}
/* }}} */

/* {{{ cdb_hashadd */
static uint32 cdb_hashadd(uint32 h, unsigned char c)
{
	h += (h << 5);
	return h ^ c;
}
/* }}} */

/* {{{ cdb_hash */
PHPAPI uint32 cdb_hash(char *buf, unsigned int len)
{
	uint32 h;

	h = CDB_HASHSTART;
	while (len) {
		h = cdb_hashadd(h, *buf++);
		--len;
	}
	return h;
}
/* }}} */

/* {{{ cdb_free */
PHPAPI void cdb_free(struct cdb *c TSRMLS_DC)
{
}
/* }}} */

/* {{{ cdb_findstart */
PHPAPI void cdb_findstart(struct cdb *c TSRMLS_DC)
{
	c->loop = 0;
}
/* }}} */

/* {{{ cdb_init */
PHPAPI void cdb_init(struct cdb *c, php_stream *fp TSRMLS_DC)
{
	cdb_free(c TSRMLS_CC);
	cdb_findstart(c TSRMLS_CC);
	c->fp = fp;
}
/* }}} */

/* {{{ cdb_read */
PHPAPI int cdb_read(struct cdb *c, char *buf, unsigned int len, uint32 pos TSRMLS_DC)
{
	if (php_stream_seek(c->fp, pos, SEEK_SET) == -1) {
		errno = EPROTO;
		return -1;
	}
	while (len > 0) {
		int r;
		do {
			r = php_stream_read(c->fp, buf, len);
		} while ((r == -1) && (errno == EINTR));
		if (r == -1) 
			return -1;
		if (r == 0) {
			errno = EPROTO;
			return -1;
		}
		buf += r;
		len -= r;
	}
	return 0;
}
/* }}} */

/* {{{ cdb_findnext */
PHPAPI int cdb_findnext(struct cdb *c, char *key, unsigned int len TSRMLS_DC)
{
	char buf[8];
	uint32 pos;
	uint32 u;

	if (!c->loop) {
		u = cdb_hash(key, len);
		if (cdb_read(c, buf, 8, (u << 3) & 2047 TSRMLS_CC) == -1) 
			return -1;
		uint32_unpack(buf + 4,&c->hslots);
		if (!c->hslots) 
			return 0;
		uint32_unpack(buf, &c->hpos);
		c->khash = u;
		u >>= 8;
		u %= c->hslots;
		u <<= 3;
		c->kpos = c->hpos + u;
	}

	while (c->loop < c->hslots) {
		if (cdb_read(c, buf, 8, c->kpos TSRMLS_CC) == -1) 
			return -1;
		uint32_unpack(buf + 4, &pos);
		if (!pos) 
			return 0;
		c->loop += 1;
		c->kpos += 8;
		if (c->kpos == c->hpos + (c->hslots << 3)) 
			c->kpos = c->hpos;
		uint32_unpack(buf, &u);
		if (u == c->khash) {
			if (cdb_read(c, buf, 8, pos TSRMLS_CC) == -1) 
				return -1;
			uint32_unpack(buf, &u);
			if (u == len)
			switch(cdb_match(c, key, len, pos + 8 TSRMLS_CC)) {
			case -1:
				return -1;
			case 1:
				uint32_unpack(buf + 4, &c->dlen);
				c->dpos = pos + 8 + len;
				return 1;
			}
		}
	}

	return 0;
}
/* }}} */

/* {{{ cdb_find */
PHPAPI int cdb_find(struct cdb *c, char *key, unsigned int len TSRMLS_DC)
{
	cdb_findstart(c TSRMLS_CC);
	return cdb_findnext(c, key, len TSRMLS_CC);
}
/* }}} */
