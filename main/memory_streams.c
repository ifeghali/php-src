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
   | Author:                                                              |
   | Marcus Boerger <helly@php.net>                                       |
   +----------------------------------------------------------------------+
 */

#define _GNU_SOURCE
#include "php.h"

/* Memory streams use a dynamic memory buffer to emulate a stream.
 * You can use php_stream_memory_open to create a readonly stream
 * from an existing memory buffer.
 */

/* Temp streams are streams that uses memory streams as long their
 * size is less than a given memory amount. When a write operation
 * exceeds that limit the content is written to a temporary file.
 */

/* {{{ ------- MEMORY stream implementation -------*/

typedef struct {
	char        *data;
	size_t      fpos;
	size_t      fsize;
	size_t      smax;
	int			mode;
} php_stream_memory_data;

static size_t php_stream_memory_write(php_stream *stream, const char *buf, size_t count) { /* {{{ */
	php_stream_memory_data *ms;

	assert(stream != NULL);
	ms = stream->abstract;
	assert(ms != NULL);

	if (ms->mode & TEMP_STREAM_READONLY) {
		return 0;
	}
	if (ms->fpos + count > ms->fsize) {
		char *tmp;

		if (!ms->data) {
			tmp = emalloc(ms->fpos + count);
		} else {
			tmp = erealloc(ms->data, ms->fpos + count);
		}
		if (!tmp) {
			count = ms->fsize - ms->fpos + 1;
		} else {
			ms->data = tmp;
			ms->fsize = ms->fpos + count;
		}
	}
	if (!ms->data)
		count = 0;
	if (count) {
		assert(buf!= NULL);
		memcpy(ms->data+ms->fpos, (char*)buf, count);
		ms->fpos += count;
	}
	return count;
} /* }}} */

static size_t php_stream_memory_read(php_stream *stream, char *buf, size_t count) { /* {{{ */
	php_stream_memory_data *ms;

	assert(stream != NULL);
	ms = stream->abstract;
	assert(ms != NULL);

	if (buf == NULL && count == 0)	{
		/* check for EOF condition */
		if (ms->fpos >= ms->fsize)	{
			return EOF;
		}
		return 0;
	}

	if (ms->fpos + count > ms->fsize) {
		count = ms->fsize - ms->fpos;
	}
	if (count) {
		assert(ms->data!= NULL);
		assert(buf!= NULL);
		memcpy(buf, ms->data+ms->fpos, count);
		ms->fpos += count;
	}
	return count;
} /* }}} */


static int php_stream_memory_close(php_stream *stream, int close_handle) { /* {{{ */
	php_stream_memory_data *ms;

	assert(stream != NULL);
	ms = stream->abstract;
	assert(ms != NULL);

	if (ms->data) {
		efree(ms->data);
	}
	ms->data = NULL;
	ms->fsize = 0;
	ms->fpos = 0;
	return 0;
} /* }}} */


static int php_stream_memory_flush(php_stream *stream) { /* {{{ */
	/* nothing to do here */
	return 0;
} /* }}} */


static int php_stream_memory_seek(php_stream *stream, off_t offset, int whence) { /* {{{ */
	php_stream_memory_data *ms;

	assert(stream != NULL);
	ms = stream->abstract;
	assert(ms != NULL);

	if (offset == 0 && whence == SEEK_CUR)
		return ms->fpos;
	switch(whence) {
		case SEEK_CUR:
			if (offset < 0) {
				if (ms->fpos < -offset) {
					ms->fpos = 0;
					/*return EINVAL;*/
				} else {
					ms->fpos = ms->fpos + offset;
				}
			} else {
				if (ms->fpos < offset) {
					ms->fpos = ms->fsize;
					/*return EINVAL;*/
				} else {
					ms->fpos = ms->fpos + offset;
				}
			}
			return 0;
		case SEEK_SET:
			if (offset > ms->fsize) {
				ms->fpos = ms->fsize;
				/*return EINVAL;*/
			} else {
				ms->fpos = offset;
			}
			return 0;
		case SEEK_END:
			if (offset > 0) {
				ms->fpos = ms->fsize;
				/*return EINVAL;*/
			} else if (ms->fpos < -offset) {
				ms->fpos = 0;
				/*return EINVAL;*/
			} else {
				ms->fpos = ms->fsize + offset;
			}
			return 0;
		default:
			return 0;
			/*return EINVAL;*/
	}
} /* }}} */

static char *php_stream_memory_gets(php_stream *stream, char *buf, size_t maxlen) { /* {{{ */
	size_t n = 1;
	char *c = buf;

	php_stream_memory_data *ms;

	assert(stream != NULL);
	ms = stream->abstract;
	assert(ms != NULL);
	assert(buf!= NULL);
	assert(ms->data!= NULL);

	while(n < maxlen && ms->fpos<ms->fsize) {
		n++;
		if ((*c = ms->data[ms->fpos++]) == '\n') {
			c++;
			break;
		}
		c++;
	}
	*c = 0;
	return buf;
} /* }}} */

static int php_stream_memory_cast(php_stream *stream, int castas, void **ret) { /* {{{ */
	return FAILURE;
} /* }}} */

php_stream_ops	php_stream_memory_ops = {
	php_stream_memory_write, php_stream_memory_read,
	php_stream_memory_close, php_stream_memory_flush,
	php_stream_memory_seek,
	php_stream_memory_gets,
	php_stream_memory_cast,
	"MEMORY"
};

PHPAPI php_stream *_php_stream_memory_create(int mode STREAMS_DC) { /* {{{ */
	php_stream_memory_data *self;

	self = emalloc(sizeof(*self));
	assert(self != NULL);
	self->data = NULL;
	self->fpos = 0;
	self->fsize = 0;
	self->smax = -1;
	self->mode = mode;
	return php_stream_alloc(&php_stream_memory_ops, self, 0, "rwb");
} /* }}} */

PHPAPI php_stream *_php_stream_memory_open(int mode, char *buf, size_t length STREAMS_DC) { /* {{{ */
	php_stream *stream;
	php_stream_memory_data *ms;

	if ((stream = php_stream_memory_create_rel(TEMP_STREAM_DEFAULT)) != NULL) {
		if (length) {
			assert(buf != NULL);
			php_stream_write(stream, buf, length);
		}
		ms->mode = mode;
	}
	return stream;
} /* }}} */

PHPAPI char *_php_stream_memory_get_buffer(php_stream *stream, size_t *length STREAMS_DC) { /* {{{ */
	php_stream_memory_data *ms;

	assert(stream != NULL);
	ms = stream->abstract;
	assert(ms != NULL);
	assert(length != 0);

	*length = ms->fsize;
	return ms->data;
} /* }}} */

/* }}} */

/* {{{ ------- TEMP stream implementation -------*/

typedef struct {
	php_stream  *innerstream;
	size_t      smax;
	int			mode;
} php_stream_temp_data;

static size_t php_stream_temp_write(php_stream *stream, const char *buf, size_t count) { /* {{{ */
	php_stream_temp_data *ts;
	TSRMLS_FETCH();

	assert(stream != NULL);
	ts = stream->abstract;
	assert(ts != NULL);

	if (php_stream_is(ts->innerstream, PHP_STREAM_IS_MEMORY)) {
		size_t memsize;
		char *membuf = php_stream_memory_get_buffer(ts->innerstream, &memsize);

		if (memsize + count >= ts->smax) {
			php_stream *file = php_stream_fopen_tmpfile();
			php_stream_write(file, membuf, memsize);
			php_stream_close(ts->innerstream);
			ts->innerstream = file;
		}
	}
	return php_stream_write(ts->innerstream, buf, count);
} /* }}} */

static size_t php_stream_temp_read(php_stream *stream, char *buf, size_t count) { /* {{{ */
	php_stream_temp_data *ts;

	assert(stream != NULL);
	ts = stream->abstract;
	assert(ts != NULL);

	return php_stream_read(ts->innerstream, buf, count);
} /* }}} */


static int php_stream_temp_close(php_stream *stream, int close_handle) { /* {{{ */
	php_stream_temp_data *ts;

	assert(stream != NULL);
	ts = stream->abstract;
	assert(ts != NULL);

	return php_stream_close(ts->innerstream);
} /* }}} */


static int php_stream_temp_flush(php_stream *stream) { /* {{{ */
	php_stream_temp_data *ts;

	assert(stream != NULL);
	ts = stream->abstract;
	assert(ts != NULL);

	return php_stream_flush(ts->innerstream);
} /* }}} */


static int php_stream_temp_seek(php_stream *stream, off_t offset, int whence) { /* {{{ */
	php_stream_temp_data *ts;

	assert(stream != NULL);
	ts = stream->abstract;
	assert(ts != NULL);

	return php_stream_seek(ts->innerstream, offset, whence);
} /* }}} */

char *php_stream_temp_gets(php_stream *stream, char *buf, size_t maxlen) { /* {{{ */
	php_stream_temp_data *ts;

	assert(stream != NULL);
	ts = stream->abstract;
	assert(ts != NULL);

	return php_stream_gets(ts->innerstream, buf, maxlen);
} /* }}} */

static int php_stream_temp_cast(php_stream *stream, int castas, void **ret) { /* {{{ */
	php_stream_temp_data *ts;

	assert(stream != NULL);
	ts = stream->abstract;
	assert(ts != NULL);

	return php_stream_cast(ts->innerstream, castas, ret, 0);
} /* }}} */

php_stream_ops	php_stream_temp_ops = {
	php_stream_temp_write, php_stream_temp_read,
	php_stream_temp_close, php_stream_temp_flush,
	php_stream_temp_seek,
	php_stream_temp_gets,
	php_stream_temp_cast,
	"TEMP"
};

PHPAPI php_stream *_php_stream_temp_create(int mode, size_t max_memory_usage STREAMS_DC) { /* {{{ */
	php_stream_temp_data *self;
	php_stream *stream;

	self = emalloc(sizeof(*self));
	assert(self != NULL);
	self->smax = max_memory_usage;
	self->mode = mode;
	stream = php_stream_alloc(&php_stream_temp_ops, self, 0, "rwb");
	self->innerstream = php_stream_memory_create(mode);
	php_stream_temp_write(stream, NULL, 0);
	return stream;
} /* }}} */

PHPAPI php_stream *_php_stream_temp_open(int mode, size_t max_memory_usage, char *buf, size_t length STREAMS_DC) { /* {{{ */
	php_stream *stream;
	php_stream_temp_data *ms;

	if ((stream = php_stream_temp_create_rel(mode & ~TEMP_STREAM_READONLY, max_memory_usage)) != NULL) {
		if (length) {
			assert(buf != NULL);
			php_stream_temp_write(stream, buf, length);
		}
		ms = stream->abstract;
		assert(ms != NULL);
		ms->mode = mode;
	}
	return stream;
} /* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
