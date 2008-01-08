/*
  +----------------------------------------------------------------------+
  | phar:// stream wrapper support                                       |
  +----------------------------------------------------------------------+
  | Copyright (c) 2005-2008 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Gregory Beaver <cellog@php.net>                             |
  |          Marcus Boerger <helly@php.net>                              |
  +----------------------------------------------------------------------+
*/

#define PHAR_STREAM 1
#include "phar_internal.h"
#include "php_stream_unlink.h"
#include "stream.h"
#include "dirstream.h"

php_stream_ops phar_ops = {
	phar_stream_write, /* write */
	phar_stream_read,  /* read */
	phar_stream_close, /* close */
	phar_stream_flush, /* flush */
	"phar stream",
	phar_stream_seek,  /* seek */
	NULL,              /* cast */
	phar_stream_stat,  /* stat */
	NULL, /* set option */
};

php_stream_wrapper_ops phar_stream_wops = {
    phar_wrapper_open_url,
    NULL,                  /* phar_wrapper_close */
    NULL,                  /* phar_wrapper_stat, */
    phar_wrapper_stat,     /* stat_url */
    phar_wrapper_open_dir, /* opendir */
    "phar",
    phar_wrapper_unlink,   /* unlink */
    phar_wrapper_rename,   /* rename */
    phar_wrapper_mkdir,    /* create directory */
    phar_wrapper_rmdir,    /* remove directory */
};

php_stream_wrapper php_stream_phar_wrapper =  {
    &phar_stream_wops,
    NULL,
    0 /* is_url */
};

/**
 * Open a phar file for streams API
 */
php_url* phar_open_url(php_stream_wrapper *wrapper, char *filename, char *mode, int options TSRMLS_DC) /* {{{ */
{
	php_url *resource;
	char *arch = NULL, *entry = NULL, *error;
	int arch_len, entry_len;

	if (!strncasecmp(filename, "phar://", 7)) {
		if (mode[0] == 'a') {
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: open mode append not supported");
			return NULL;			
		}		
		if (phar_split_fname(filename, strlen(filename), &arch, &arch_len, &entry, &entry_len TSRMLS_CC) == FAILURE) {
			if (!(options & PHP_STREAM_URL_STAT_QUIET)) {
				if (arch && !entry) {
					php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: no directory in \"%s\", must have at least phar://%s/ for root directory (always use full path to a new phar)", filename, arch);
					arch = NULL;
				} else {
					php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: invalid url \"%s\" (cannot contain .phar.php and .phar.gz/.phar.bz2)", filename);
				}
			}
			if (arch) {
				efree(arch);
			}
			if (entry) {
				efree(entry);
			}
			return NULL;
		}
		resource = ecalloc(1, sizeof(php_url));
		resource->scheme = estrndup("phar", 4);
		resource->host = arch;

		resource->path = entry;
#if MBO_0
		if (resource) {
			fprintf(stderr, "Alias:     %s\n", alias);
			fprintf(stderr, "Scheme:    %s\n", resource->scheme);
/*			fprintf(stderr, "User:      %s\n", resource->user);*/
/*			fprintf(stderr, "Pass:      %s\n", resource->pass ? "***" : NULL);*/
			fprintf(stderr, "Host:      %s\n", resource->host);
/*			fprintf(stderr, "Port:      %d\n", resource->port);*/
			fprintf(stderr, "Path:      %s\n", resource->path);
/*			fprintf(stderr, "Query:     %s\n", resource->query);*/
/*			fprintf(stderr, "Fragment:  %s\n", resource->fragment);*/
		}
#endif
		if (PHAR_GLOBALS->request_init && PHAR_GLOBALS->phar_plain_map.arBuckets && zend_hash_exists(&(PHAR_GLOBALS->phar_plain_map), arch, arch_len+1)) {
			return resource;
		}
		if (mode[0] == 'w' || (mode[0] == 'r' && mode[1] == '+')) {
			if (PHAR_G(readonly)) {
				php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: write operations disabled by INI setting");
				php_url_free(resource);
				return NULL;
			}
			if (phar_open_or_create_filename(resource->host, arch_len, NULL, 0, options, NULL, &error TSRMLS_CC) == FAILURE)
			{
				if (error) {
					php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, error);
					efree(error);
				}
				php_url_free(resource);
				return NULL;
			}
		} else {
			if (phar_open_filename(resource->host, arch_len, NULL, 0, options, NULL, &error TSRMLS_CC) == FAILURE)
			{
				if (error) {
					php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, error);
					efree(error);
				}
				php_url_free(resource);
				return NULL;
			}
		}	
		return resource;
	}

	return NULL;
}
/* }}} */

/**
 * used for fopen('phar://...') and company
 */
static php_stream * phar_wrapper_open_url(php_stream_wrapper *wrapper, char *path, char *mode, int options, char **opened_path, php_stream_context *context STREAMS_DC TSRMLS_DC) /* {{{ */
{
	phar_entry_data *idata;
	char *internal_file;
	char *error;
	char *plain_map;
	HashTable *pharcontext;
	php_url *resource = NULL;
	php_stream *fp, *fpf;
	zval **pzoption, *metadata;
	uint host_len;

	if ((resource = phar_open_url(wrapper, path, mode, options TSRMLS_CC)) == NULL) {
		return NULL;
	}

	/* we must have at the very least phar://alias.phar/internalfile.php */
	if (!resource->scheme || !resource->host || !resource->path) {
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: invalid url \"%s\"", path);
		return NULL;
	}

	if (strcasecmp("phar", resource->scheme)) {
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: not a phar stream url \"%s\"", path);
		return NULL;
	}

	host_len = strlen(resource->host);
	phar_request_initialize(TSRMLS_C);
	if (zend_hash_find(&(PHAR_GLOBALS->phar_plain_map), resource->host, host_len+1, (void **)&plain_map) == SUCCESS) {
		spprintf(&internal_file, 0, "%s%s", plain_map, resource->path);
		fp = php_stream_open_wrapper_ex(internal_file, mode, options, opened_path, context);
		efree(internal_file);
		if (!fp) {
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: file \"%s\" extracted from \"%s\" could not be opened", resource->path+1, resource->host);
		}
		php_url_free(resource);
		return fp;
	}

	/* strip leading "/" */
	internal_file = estrdup(resource->path + 1);
	if (mode[0] == 'w' || (mode[0] == 'r' && mode[1] == '+')) {
		if (NULL == (idata = phar_get_or_create_entry_data(resource->host, host_len, internal_file, strlen(internal_file), mode, &error TSRMLS_CC))) {
			if (error) {
				php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, error);
				efree(error);
			} else {
				php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: file \"%s\" could not be created in phar \"%s\"", internal_file, resource->host);
			}
			efree(internal_file);
			php_url_free(resource);
			return NULL;
		}
		if (error) {
			efree(error);
		}
		fpf = php_stream_alloc(&phar_ops, idata, NULL, mode);
		php_url_free(resource);
		efree(internal_file);
		if (context && context->options && zend_hash_find(HASH_OF(context->options), "phar", sizeof("phar"), (void**)&pzoption) == SUCCESS) {
			pharcontext = HASH_OF(*pzoption);
			if (idata->internal_file->uncompressed_filesize == 0
				&& idata->internal_file->compressed_filesize == 0
				&& zend_hash_find(pharcontext, "compress", sizeof("compress"), (void**)&pzoption) == SUCCESS
				&& Z_TYPE_PP(pzoption) == IS_LONG
				&& (Z_LVAL_PP(pzoption) & ~PHAR_ENT_COMPRESSION_MASK) == 0
			) {
				idata->internal_file->flags &= ~PHAR_ENT_COMPRESSION_MASK;
				idata->internal_file->flags |= Z_LVAL_PP(pzoption);
			}
			if (zend_hash_find(pharcontext, "metadata", sizeof("metadata"), (void**)&pzoption) == SUCCESS) {
				if (idata->internal_file->metadata) {
					zval_ptr_dtor(&idata->internal_file->metadata);
					idata->internal_file->metadata = NULL;
				}

				MAKE_STD_ZVAL(idata->internal_file->metadata);
				metadata = *pzoption;
				ZVAL_ZVAL(idata->internal_file->metadata, metadata, 1, 0);
				idata->phar->is_modified = 1;
			}
		}
		return fpf;
	} else {
		if ((FAILURE == phar_get_entry_data(&idata, resource->host, host_len, internal_file, strlen(internal_file), "r", &error TSRMLS_CC)) || !idata) {
			if (error) {
				php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, error);
				efree(error);
			}
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: \"%s\" is not a file in phar \"%s\"", internal_file, resource->host);
			efree(internal_file);
			php_url_free(resource);
			return NULL;
		}
	}
	php_url_free(resource);
	
#if MBO_0
		fprintf(stderr, "Pharname:   %s\n", idata->phar->filename);
		fprintf(stderr, "Filename:   %s\n", internal_file);
		fprintf(stderr, "Entry:      %s\n", idata->internal_file->filename);
		fprintf(stderr, "Size:       %u\n", idata->internal_file->uncompressed_filesize);
		fprintf(stderr, "Compressed: %u\n", idata->internal_file->flags);
		fprintf(stderr, "Offset:     %u\n", idata->internal_file->offset_within_phar);
		fprintf(stderr, "Cached:     %s\n", idata->internal_file->filedata ? "yes" : "no");
#endif

	/* do we have the data already? */
	if (idata->fp) {
		fpf = php_stream_alloc(&phar_ops, idata, NULL, mode);
		efree(internal_file);
		return fpf;
	}

#if PHP_MAJOR_VERSION < 6
	if (PG(safe_mode) && (!php_checkuid(idata->phar->fname, NULL, CHECKUID_ALLOW_ONLY_FILE))) {
		phar_entry_delref(idata TSRMLS_CC);
		efree(internal_file);
		return NULL;
	}
#endif
	
	if (php_check_open_basedir(idata->phar->fname TSRMLS_CC)) {
		phar_entry_delref(idata TSRMLS_CC);
		efree(internal_file);
		return NULL;
	}

	fp = idata->phar->fp;

	if (!idata->phar->is_zip && !fp) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot open phar \"%s\"", idata->phar->fname);
		phar_entry_delref(idata TSRMLS_CC);
		efree(internal_file);
		return NULL;
	}

	if (!phar_open_jit(idata->phar, idata->internal_file, fp, &error, idata->for_write TSRMLS_CC)) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, error);
		efree(error);
		phar_entry_delref(idata TSRMLS_CC);
		efree(internal_file);
		return NULL;
	}
	idata->fp = idata->internal_file->fp;
	if (idata->fp == idata->phar->fp) {
		idata->zero = idata->internal_file->offset_within_phar + idata->phar->internal_file_start;
	}

	/* check length, crc32 */
	if (!idata->internal_file->is_crc_checked && phar_postprocess_file(wrapper, options, idata, idata->internal_file->crc32 TSRMLS_CC) != SUCCESS) {
		/* already issued the error */
		phar_entry_delref(idata TSRMLS_CC);
		efree(internal_file);
		return NULL;
	}
	idata->internal_file->is_crc_checked = 1;

	fpf = php_stream_alloc(&phar_ops, idata, NULL, mode);
	efree(internal_file);
	return fpf;
}
/* }}} */

/**
 * Used for fclose($fp) where $fp is a phar archive
 */
static int phar_stream_close(php_stream *stream, int close_handle TSRMLS_DC) /* {{{ */
{
	phar_entry_delref((phar_entry_data *)stream->abstract TSRMLS_CC);

	return 0;
}
/* }}} */

/**
 * used for fread($fp) and company on a fopen()ed phar file handle
 */
static size_t phar_stream_read(php_stream *stream, char *buf, size_t count TSRMLS_DC) /* {{{ */
{
	phar_entry_data *data = (phar_entry_data *)stream->abstract;
	size_t got;
	
	if (data->internal_file->is_deleted) {
		stream->eof = 1;
		return 0;
	}

	/* use our proxy position */
	php_stream_seek(data->fp, data->position + data->zero, SEEK_SET);

	if (!data->zero) {
		got = php_stream_read(data->fp, buf, count);
		if (data->fp->eof) {
			stream->eof = 1;
		}
		/* note the position, and restore the stream for the next fp */
		data->position = php_stream_tell(data->fp);
	} else {
		got = php_stream_read(data->fp, buf, MIN(count, data->internal_file->uncompressed_filesize - data->position));
		data->position = php_stream_tell(data->fp) - data->zero;
		stream->eof = (data->position == (off_t) data->internal_file->uncompressed_filesize);
	}


	return got;
}
/* }}} */

/**
 * Used for fseek($fp) on a phar file handle
 */
static int phar_stream_seek(php_stream *stream, off_t offset, int whence, off_t *newoffset TSRMLS_DC) /* {{{ */
{
	phar_entry_data *data = (phar_entry_data *)stream->abstract;

	int res;
	if (data->zero) {
		off_t temp;
		switch (whence) {
			case SEEK_END :
				temp = data->zero + data->internal_file->uncompressed_filesize + offset;
				break;
			case SEEK_CUR :
				temp = data->zero + data->position + offset;
				break;
			case SEEK_SET :
				temp = data->zero + offset;
				break;
		}
		if (temp > data->zero + (off_t) data->internal_file->uncompressed_filesize) {
			*newoffset = -1;
			return -1;
		}
		if (temp < data->zero) {
			*newoffset = -1;
			return -1;
		}
		res = php_stream_seek(data->fp, temp, SEEK_SET);
		*newoffset = php_stream_tell(data->fp) - data->zero;
		data->position = *newoffset;
		return res;
	}
	if (whence != SEEK_SET) {
		/* use our proxy position, so the relative stuff works */
		php_stream_seek(data->fp, data->position, SEEK_SET);
	}
	/* now do the actual seek */
	res = php_stream_seek(data->fp, offset, whence);
	*newoffset = php_stream_tell(data->fp);
	data->position = *newoffset;
	return res;
}
/* }}} */

/**
 * Used for writing to a phar file
 */
static size_t phar_stream_write(php_stream *stream, const char *buf, size_t count TSRMLS_DC) /* {{{ */
{
	phar_entry_data *data = (phar_entry_data *) stream->abstract;

	php_stream_seek(data->fp, data->position, SEEK_SET);
	if (count != php_stream_write(data->fp, buf, count)) {
		php_stream_wrapper_log_error(stream->wrapper, stream->flags TSRMLS_CC, "phar error: Could not write %d characters to \"%s\" in phar \"%s\"", (int) count, data->internal_file->filename, data->phar->fname);
		return -1;
	}
	data->position = php_stream_tell(data->fp);
	if (data->position > (off_t)data->internal_file->uncompressed_filesize) {
		data->internal_file->uncompressed_filesize = data->position;
	}
	data->internal_file->compressed_filesize = data->internal_file->uncompressed_filesize;
	data->internal_file->old_flags = data->internal_file->flags;
	data->internal_file->is_modified = 1;
	return count;
}
/* }}} */

/**
 * Used to save work done on a writeable phar
 */
static int phar_stream_flush(php_stream *stream TSRMLS_DC) /* {{{ */
{
	char *error;
	int ret;
	if (stream->mode[0] == 'w' || (stream->mode[0] == 'r' && stream->mode[1] == '+')) {
		ret = phar_flush(((phar_entry_data *)stream->abstract)->phar, 0, 0, &error TSRMLS_CC);
		if (error) {
			php_stream_wrapper_log_error(stream->wrapper, REPORT_ERRORS TSRMLS_CC, error);
			efree(error);
		}
		return ret;
	} else {
		return EOF;
	}
}
/* }}} */

 /* {{{ phar_dostat */
/**
 * stat an opened phar file handle stream, used by phar_stat()
 */
void phar_dostat(phar_archive_data *phar, phar_entry_info *data, php_stream_statbuf *ssb, 
			zend_bool is_temp_dir, char *alias, int alias_len TSRMLS_DC)
{
	char *tmp;
	int tmp_len;
	memset(ssb, 0, sizeof(php_stream_statbuf));

	if (!is_temp_dir && !data->is_dir) {
		ssb->sb.st_size = data->uncompressed_filesize;
		ssb->sb.st_mode = data->flags & PHAR_ENT_PERM_MASK;
		ssb->sb.st_mode |= S_IFREG; /* regular file */
		/* timestamp is just the timestamp when this was added to the phar */
#ifdef NETWARE
		ssb->sb.st_mtime.tv_sec = data->timestamp;
		ssb->sb.st_atime.tv_sec = data->timestamp;
		ssb->sb.st_ctime.tv_sec = data->timestamp;
#else
		ssb->sb.st_mtime = data->timestamp;
		ssb->sb.st_atime = data->timestamp;
		ssb->sb.st_ctime = data->timestamp;
#endif
	} else if (!is_temp_dir && data->is_dir) {
		ssb->sb.st_size = 0;
		ssb->sb.st_mode = data->flags & PHAR_ENT_PERM_MASK;
		ssb->sb.st_mode |= S_IFDIR; /* regular directory */
		/* timestamp is just the timestamp when this was added to the phar */
#ifdef NETWARE
		ssb->sb.st_mtime.tv_sec = data->timestamp;
		ssb->sb.st_atime.tv_sec = data->timestamp;
		ssb->sb.st_ctime.tv_sec = data->timestamp;
#else
		ssb->sb.st_mtime = data->timestamp;
		ssb->sb.st_atime = data->timestamp;
		ssb->sb.st_ctime = data->timestamp;
#endif
	} else {
		ssb->sb.st_size = 0;
		ssb->sb.st_mode = 0777;
		ssb->sb.st_mode |= S_IFDIR; /* regular directory */
#ifdef NETWARE
		ssb->sb.st_mtime.tv_sec = phar->max_timestamp;
		ssb->sb.st_atime.tv_sec = phar->max_timestamp;
		ssb->sb.st_ctime.tv_sec = phar->max_timestamp;
#else
		ssb->sb.st_mtime = phar->max_timestamp;
		ssb->sb.st_atime = phar->max_timestamp;
		ssb->sb.st_ctime = phar->max_timestamp;
#endif
	}
	if (!phar->is_writeable) {
		ssb->sb.st_mode = (ssb->sb.st_mode & 0555) | (ssb->sb.st_mode & ~0777);
	}

	ssb->sb.st_nlink = 1;
	ssb->sb.st_rdev = -1;
	if (data) {
		tmp_len = data->filename_len + alias_len;
	} else {
		tmp_len = alias_len + 1;
	}
	tmp = (char *) emalloc(tmp_len);
	memcpy(tmp, alias, alias_len);
	if (data) {
		memcpy(tmp + alias_len, data->filename, data->filename_len);
	} else {
		*(tmp+alias_len) = '/';
	}
	/* this is only for APC, so use /dev/null device - no chance of conflict there! */
	ssb->sb.st_dev = 0xc;
	/* generate unique inode number for alias/filename, so no phars will conflict */
	ssb->sb.st_ino = (unsigned short)zend_get_hash_value(tmp, tmp_len);
	efree(tmp);
#ifndef PHP_WIN32
	ssb->sb.st_blksize = -1;
	ssb->sb.st_blocks = -1;
#endif
}
/* }}}*/

/**
 * Stat an opened phar file handle
 */
static int phar_stream_stat(php_stream *stream, php_stream_statbuf *ssb TSRMLS_DC) /* {{{ */
{
	phar_entry_data *data = (phar_entry_data *)stream->abstract;

	/* If ssb is NULL then someone is misbehaving */
	if (!ssb) {
		return -1;
	}

	phar_dostat(data->phar, data->internal_file, ssb, 0, data->phar->alias, data->phar->alias_len TSRMLS_CC);
	return 0;
}
/* }}} */

/**
 * Stream wrapper stat implementation of stat()
 */
static int phar_wrapper_stat(php_stream_wrapper *wrapper, char *url, int flags,
				  php_stream_statbuf *ssb, php_stream_context *context TSRMLS_DC) /* {{{ */
{
	php_url *resource = NULL;
	char *internal_file, *key, *error, *plain_map;
	uint keylen;
	ulong unused;
	phar_archive_data *phar;
	phar_entry_info *entry;
	uint host_len;
	int retval;

	if ((resource = phar_open_url(wrapper, url, "r", flags TSRMLS_CC)) == NULL) {
		return -1;
	}

	/* we must have at the very least phar://alias.phar/internalfile.php */
	if (!resource->scheme || !resource->host || !resource->path) {
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, flags TSRMLS_CC, "phar error: invalid url \"%s\"", url);
		return -1;
	}

	if (strcasecmp("phar", resource->scheme)) {
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, flags TSRMLS_CC, "phar error: not a phar url \"%s\"", url);
		return -1;
	}

	host_len = strlen(resource->host);
	phar_request_initialize(TSRMLS_C);
	if (zend_hash_find(&(PHAR_GLOBALS->phar_plain_map), resource->host, host_len+1, (void **)&plain_map) == SUCCESS) {
		spprintf(&internal_file, 0, "%s%s", plain_map, resource->path);
		retval = php_stream_stat_path_ex(internal_file, flags, ssb, context);
  		if (retval == -1) {
			php_stream_wrapper_log_error(wrapper, 0/* TODO:options */ TSRMLS_CC, "phar error: file \"%s\" extracted from \"%s\" could not be opened", internal_file, resource->host);
		}
		php_url_free(resource);
		efree(internal_file);
		return retval;
	}

	internal_file = resource->path + 1; /* strip leading "/" */
	/* find the phar in our trusty global hash indexed by alias (host of phar://blah.phar/file.whatever) */
	if (FAILURE == phar_get_archive(&phar, resource->host, strlen(resource->host), NULL, 0, &error TSRMLS_CC)) {
		php_url_free(resource);
		if (error) {
			php_stream_wrapper_log_error(wrapper, flags TSRMLS_CC, error);
			efree(error);
		}
		return 0;
	}
	if (error) {
		efree(error);
	}
	if (*internal_file == '\0') {
		/* root directory requested */
		phar_dostat(phar, NULL, ssb, 1, phar->alias, phar->alias_len TSRMLS_CC);
		php_url_free(resource);
		return 0;
	}
	if (!phar->manifest.arBuckets) {
		php_url_free(resource);
		return 0;
	}
	/* search through the manifest of files, and if we have an exact match, it's a file */
	if (SUCCESS == zend_hash_find(&phar->manifest, internal_file, strlen(internal_file), (void**)&entry)) {
		phar_dostat(phar, entry, ssb, 0, phar->alias, phar->alias_len TSRMLS_CC);
	} else {
		/* search for directory (partial match of a file) */
		zend_hash_internal_pointer_reset(&phar->manifest);
		while (FAILURE != zend_hash_has_more_elements(&phar->manifest)) {
			if (HASH_KEY_NON_EXISTANT !=
					zend_hash_get_current_key_ex(
						&phar->manifest, &key, &keylen, &unused, 0, NULL)) {
				if (0 == memcmp(internal_file, key, strlen(internal_file))) {
					/* directory found, all dirs have the same stat */
					if (key[strlen(internal_file)] == '/') {
						phar_dostat(phar, NULL, ssb, 1, phar->alias, phar->alias_len TSRMLS_CC);
						break;
					}
				}
			}
			if (SUCCESS != zend_hash_move_forward(&phar->manifest)) {
				break;
			}
		}
	}

	php_url_free(resource);
	return 0;
}
/* }}} */

/**
 * Unlink a file within a phar archive
 */
static int phar_wrapper_unlink(php_stream_wrapper *wrapper, char *url, int options, php_stream_context *context TSRMLS_DC) /* {{{ */
{
	php_url *resource;
	char *internal_file, *error, *plain_map;
	phar_entry_data *idata;
	uint host_len;
	int retval;

	if ((resource = phar_open_url(wrapper, url, "rb", options TSRMLS_CC)) == NULL) {
		return 0;
	}

	/* we must have at the very least phar://alias.phar/internalfile.php */
	if (!resource->scheme || !resource->host || !resource->path) {
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: invalid url \"%s\"", url);
		return 0;
	}

	if (strcasecmp("phar", resource->scheme)) {
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: not a phar stream url \"%s\"", url);
		return 0;
	}

	host_len = strlen(resource->host);
	phar_request_initialize(TSRMLS_C);
	if (zend_hash_find(&(PHAR_GLOBALS->phar_plain_map), resource->host, host_len+1, (void **)&plain_map) == SUCCESS) {
		spprintf(&internal_file, 0, "%s%s", plain_map, resource->path);
		retval = php_stream_unlink(internal_file, options, context);
		if (!retval) {
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: file \"%s\" extracted from \"%s\" could not be opened", internal_file, resource->host);
		}
		php_url_free(resource);
		efree(internal_file);
		return retval;
	}

	if (PHAR_G(readonly)) {
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: write operations disabled by INI setting");
		return 0;
	}

	/* need to copy to strip leading "/", will get touched again */
	internal_file = estrdup(resource->path + 1);
	if (FAILURE == phar_get_entry_data(&idata, resource->host, strlen(resource->host), internal_file, strlen(internal_file), "r", &error TSRMLS_CC)) {
		/* constraints of fp refcount were not met */
		if (error) {
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, error);
			efree(error);
		}
		efree(internal_file);
		php_url_free(resource);
		return 0;
	}
	if (error) {
		efree(error);
	}
	if (!idata) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: \"%s\" is not a file in phar \"%s\", cannot unlink", internal_file, resource->host);
		efree(internal_file);
		php_url_free(resource);
		return 0;
	}
	if (idata->internal_file->fp_refcount > 1) {
		/* more than just our fp resource is open for this file */ 
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: \"%s\" in phar \"%s\", has open file pointers, cannot unlink", internal_file, resource->host);
		efree(internal_file);
		php_url_free(resource);
		phar_entry_delref(idata TSRMLS_CC);
		return 0;
	}
	php_url_free(resource);
	efree(internal_file);
	phar_entry_remove(idata, &error TSRMLS_CC);
	if (error) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, error);
		efree(error);
	}
	return 1;
}
/* }}} */

static int phar_wrapper_rename(php_stream_wrapper *wrapper, char *url_from, char *url_to, int options, php_stream_context *context TSRMLS_DC) /* {{{ */
{
	php_url *resource_from, *resource_to;
	char *error, *plain_map;
	phar_archive_data *phar;
	phar_entry_info *entry;
	uint host_len;

	error = NULL;
	if (PHAR_G(readonly)) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: write operations disabled by INI setting");
		return 0;
	}

	if ((resource_from = phar_open_url(wrapper, url_from, "r+b", options TSRMLS_CC)) == NULL) {
		return 0;
	}
	
	if ((resource_to = phar_open_url(wrapper, url_to, "wb", options TSRMLS_CC)) == NULL) {
		php_url_free(resource_from);
		return 0;
	}

	/* we must have at the very least phar://alias.phar/internalfile.php */
	if (!resource_from->scheme || !resource_from->host || !resource_from->path) {
		php_url_free(resource_from);
		php_url_free(resource_to);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: invalid url \"%s\"", url_from);
		return 0;
	}
	
	if (!resource_to->scheme || !resource_to->host || !resource_to->path) {
		php_url_free(resource_from);
		php_url_free(resource_to);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: invalid url \"%s\"", url_to);
		return 0;
	}

	if (strcasecmp("phar", resource_from->scheme)) {
		php_url_free(resource_from);
		php_url_free(resource_to);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: not a phar stream url \"%s\"", url_from);
		return 0;
	}

	if (strcasecmp("phar", resource_to->scheme)) {
		php_url_free(resource_from);
		php_url_free(resource_to);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: not a phar stream url \"%s\"", url_to);
		return 0;
	}

	if (strcmp(resource_from->host, resource_to->host)) {
		php_url_free(resource_from);
		php_url_free(resource_to);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot rename \"%s\" to \"%s\", not within the same phar archive", url_from, url_to);
		return 0;
	}

	host_len = strlen(resource_from->host);
	phar_request_initialize(TSRMLS_C);
	if (zend_hash_find(&(PHAR_GLOBALS->phar_plain_map), resource_from->host, host_len+1, (void **)&plain_map) == SUCCESS) {
		/*TODO:use php_stream_rename() once available*/
		php_url_free(resource_from);
		php_url_free(resource_to);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot rename \"%s\" to \"%s\" from extracted phar archive", url_from, url_to);
		return 0;
	}

	if (SUCCESS != phar_get_archive(&phar, resource_from->host, strlen(resource_from->host), NULL, 0, &error TSRMLS_CC)) {
		php_url_free(resource_from);
		php_url_free(resource_to);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot rename \"%s\" to \"%s\": %s", url_from, url_to, error);
		efree(error);
		return 0;
	}

	if (SUCCESS == zend_hash_find(&(phar->manifest), resource_from->path+1, strlen(resource_from->path)-1, (void **)&entry)) {
		phar_entry_info new;

		/* perform rename magic */
		if (entry->is_deleted) {
			php_url_free(resource_from);
			php_url_free(resource_to);
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot rename \"%s\" to \"%s\" from extracted phar archive, source has been deleted", url_from, url_to);
			return 0;
		}
		/* transfer all data over to the new entry */
		memcpy((void *) &new, (void *) entry, sizeof(phar_entry_info));
		/* mark the old one for deletion */
		entry->is_deleted = 1;
		entry->fp = NULL;
		entry->metadata = 0;
		entry->link = NULL;
#if HAVE_PHAR_ZIP
		entry->zip = NULL;
#endif

		zend_hash_add(&(phar->manifest), resource_to->path+1, strlen(resource_to->path)-1, (void **)&new, sizeof(phar_entry_info), (void **) &entry);
		if (!entry->is_modified) {
			/* copy file contents into a new temp stream */
			if (!phar_open_jit(phar, entry, phar->fp, &error, 1 TSRMLS_CC)) {
				php_url_free(resource_from);
				php_url_free(resource_to);
				php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot rename \"%s\" to \"%s\": %s", url_from, url_to, error);
				efree(error);
				return 0;
			}
		}
		entry->is_modified = 1;
		entry->filename = estrdup(resource_to->path+1);
		entry->filename_len = strlen(entry->filename);
		phar_flush(phar, 0, 0, &error TSRMLS_CC);
		if (error) {
			php_url_free(resource_from);
			php_url_free(resource_to);
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot rename \"%s\" to \"%s\": %s", url_from, url_to, error);
			efree(error);
			return 0;
		}
	}
	php_url_free(resource_from);
	php_url_free(resource_to);
	return 1;
}
/* }}} */
