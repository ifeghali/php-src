/*
  +----------------------------------------------------------------------+
  | phar php single-file executable PHP extension                        |
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
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "phar_internal.h"

#define PHAR_FUNC(name) \
	static PHP_NAMED_FUNCTION(name)

PHAR_FUNC(phar_opendir) /* {{{ */
{
	char *filename;
	int filename_len;
	zval *zcontext = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &filename, &filename_len, &zcontext) == FAILURE) {
		return;
	}

	if (!IS_ABSOLUTE_PATH(filename, filename_len)) {
		char *arch, *entry, *fname;
		int arch_len, entry_len, fname_len;
		fname = zend_get_executed_filename(TSRMLS_C);

		/* we are checking for existence of a file within the relative path.  Chances are good that this is
		   retrieving something from within the phar archive */

		if (strncasecmp(fname, "phar://", 7)) {
			goto skip_phar;
		}
		fname_len = strlen(fname);
		if (SUCCESS == phar_split_fname(fname, fname_len, &arch, &arch_len, &entry, &entry_len TSRMLS_CC)) {
			php_stream_context *context = NULL;
			php_stream *stream;
			char *name;

			efree(entry);
			entry = filename;
			/* fopen within phar, if :// is not in the url, then prepend phar://<archive>/ */
			entry_len = filename_len;
			if (strstr(entry, "://")) {
				efree(arch);
				goto skip_phar;
			}
			/* retrieving a file within the current directory, so use this if possible */
			entry = phar_fix_filepath(entry, &entry_len, 1 TSRMLS_CC);

			spprintf(&name, 4096, "phar://%s/%s", arch, entry);
			efree(entry);
			efree(arch);
			if (zcontext) {
				context = php_stream_context_from_zval(zcontext, 0);
			}
			stream = php_stream_opendir(name, REPORT_ERRORS, context);
			efree(name);
			if (!stream) {
				RETURN_FALSE;
			}
			php_stream_to_zval(stream, return_value);
			efree(entry);
			return;
		}
	}
skip_phar:
	PHAR_G(orig_opendir)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	return;
}
/* }}} */

PHAR_FUNC(phar_file_get_contents) /* {{{ */
{
	char *filename;
	int filename_len;
	char *contents;
	zend_bool use_include_path = 0;
	php_stream *stream;
	int len, newlen;
	long offset = -1;
	long maxlen = PHP_STREAM_COPY_ALL;
	zval *zcontext = NULL;

	/* Parse arguments */
	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "s|br!ll", &filename, &filename_len, &use_include_path, &zcontext, &offset, &maxlen) == FAILURE) {
		return;
	}
	if (use_include_path || !IS_ABSOLUTE_PATH(filename, filename_len)) {
		char *arch, *entry, *fname;
		int arch_len, entry_len, fname_len, free_filename = 0;
		php_stream_context *context = NULL;
		fname = zend_get_executed_filename(TSRMLS_C);

		if (strncasecmp(fname, "phar://", 7)) {
			goto skip_phar;
		}
		fname_len = strlen(fname);
		if (SUCCESS == phar_split_fname(fname, fname_len, &arch, &arch_len, &entry, &entry_len TSRMLS_CC)) {
			char *name;
			phar_archive_data **pphar;

			efree(entry);
			entry = filename;
			/* fopen within phar, if :// is not in the url, then prepend phar://<archive>/ */
			entry_len = filename_len;
			if (strstr(entry, "://")) {
				efree(arch);
				if (free_filename) {
					efree(filename);
				}
				goto skip_phar;
			}

			if (ZEND_NUM_ARGS() == 5 && maxlen < 0) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "length must be greater than or equal to zero");
				RETURN_FALSE;
			}

			if (!IS_ABSOLUTE_PATH(filename, filename_len)) {
				/* retrieving a file defaults to within the current directory, so use this if possible */
				if (SUCCESS == (zend_hash_find(&(PHAR_GLOBALS->phar_fname_map), arch, arch_len, (void **) &pphar))) {
					name = entry;
					entry = phar_fix_filepath(entry, &entry_len, 1 TSRMLS_CC);
					if (!zend_hash_exists(&((*pphar)->manifest), entry, entry_len)) {
						/* this file is not in the current directory, use the original path */
						efree(entry);
						entry = name;
					}
				}
			}
			/* auto-convert to phar:// */
			spprintf(&name, 4096, "phar://%s/%s", arch, entry);
			if (entry != filename) {
				efree(entry);
			}
			efree(arch);
			if (zcontext) {
				context = php_stream_context_from_zval(zcontext, 0);
			}
			stream = php_stream_open_wrapper_ex(name, "rb", 0 | REPORT_ERRORS, NULL, context);
			if (free_filename) {
				efree(filename);
			}
			efree(name);


			if (!stream) {
				RETURN_FALSE;
			}

			if (offset > 0 && php_stream_seek(stream, offset, SEEK_SET) < 0) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to seek to position %ld in the stream", offset);
				php_stream_close(stream);
				RETURN_FALSE;
			}

			/* uses mmap if possible */
			if ((len = php_stream_copy_to_mem(stream, &contents, maxlen, 0)) > 0) {
#if PHP_MAJOR_VERSION < 6
				if (PG(magic_quotes_runtime)) {
					contents = php_addslashes(contents, len, &newlen, 1 TSRMLS_CC); /* 1 = free source string */
					len = newlen;
				}
#endif
				RETVAL_STRINGL(contents, len, 0);
			} else if (len == 0) {
				RETVAL_EMPTY_STRING();
			} else {
				RETVAL_FALSE;
			}

			php_stream_close(stream);
			return;
		}
	}
skip_phar:
	PHAR_G(orig_file_get_contents)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	return;
}
/* }}} */

PHAR_FUNC(phar_fopen) /* {{{ */
{
	char *filename, *mode;
	int filename_len, mode_len;
	zend_bool use_include_path = 0;
	zval *zcontext = NULL;
	php_stream *stream;

	if (!zend_hash_num_elements(&(PHAR_GLOBALS->phar_fname_map))) {
		/* no need to check, include_path not even specified in fopen/ no active phars */
		goto skip_phar;
	}
	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "ss|br", &filename, &filename_len, &mode, &mode_len, &use_include_path, &zcontext) == FAILURE) {
		goto skip_phar;
	}
	if (use_include_path || !IS_ABSOLUTE_PATH(filename, filename_len)) {
		char *arch, *entry, *fname;
		int arch_len, entry_len, fname_len, free_filename = 0;
		php_stream_context *context = NULL;
		fname = zend_get_executed_filename(TSRMLS_C);

		if (strncasecmp(fname, "phar://", 7)) {
			goto skip_phar;
		}
		fname_len = strlen(fname);
		if (SUCCESS == phar_split_fname(fname, fname_len, &arch, &arch_len, &entry, &entry_len TSRMLS_CC)) {
			char *name;
			phar_archive_data **pphar;

			efree(entry);
			entry = filename;
			/* fopen within phar, if :// is not in the url, then prepend phar://<archive>/ */
			entry_len = filename_len;
			if (strstr(entry, "://")) {
				efree(arch);
				if (free_filename) {
					efree(filename);
				}
				goto skip_phar;
			}
			/* retrieving a file defaults to within the current directory, so use this if possible */
			if (SUCCESS == (zend_hash_find(&(PHAR_GLOBALS->phar_fname_map), arch, arch_len, (void **) &pphar))) {
				name = entry;
				entry = phar_fix_filepath(entry, &entry_len, 1 TSRMLS_CC);
				if (!zend_hash_exists(&((*pphar)->manifest), entry, entry_len)) {
					/* this file is not in the current directory, use the original path */
					efree(entry);
					entry = name;
				}
			}
			/* auto-convert to phar:// */
			spprintf(&name, 4096, "phar://%s/%s", arch, entry);
			if (entry != filename) {
				efree(entry);
			}
			efree(arch);
			context = php_stream_context_from_zval(zcontext, 0);
			stream = php_stream_open_wrapper_ex(name, mode, 0 | REPORT_ERRORS, NULL, context);
			efree(name);
			if (stream == NULL) {
				RETURN_FALSE;
			}
			php_stream_to_zval(stream, return_value);
			if (zcontext) {
				zend_list_addref(Z_RESVAL_P(zcontext));
			}
			if (free_filename) {
				efree(filename);
			}
			return;
		}
	}
skip_phar:
	PHAR_G(orig_fopen)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	return;
}
/* }}} */

#ifndef S_ISDIR
#define S_ISDIR(mode)	(((mode)&S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(mode)	(((mode)&S_IFMT) == S_IFREG)
#endif
#ifndef S_ISLNK
#define S_ISLNK(mode)	(((mode)&S_IFMT) == S_IFLNK)
#endif

#define S_IXROOT ( S_IXUSR | S_IXGRP | S_IXOTH )

#define IS_LINK_OPERATION(__t) ((__t) == FS_TYPE || (__t) == FS_IS_LINK || (__t) == FS_LSTAT)
#define IS_EXISTS_CHECK(__t) ((__t) == FS_EXISTS  || (__t) == FS_IS_W || (__t) == FS_IS_R || (__t) == FS_IS_X || (__t) == FS_IS_FILE || (__t) == FS_IS_DIR || (__t) == FS_IS_LINK)
#define IS_ABLE_CHECK(__t) ((__t) == FS_IS_R || (__t) == FS_IS_W || (__t) == FS_IS_X)
#define IS_ACCESS_CHECK(__t) (IS_ABLE_CHECK(type) || (__t) == FS_EXISTS)

/* {{{ php_stat
 */
void phar_fancy_stat(struct stat *stat_sb, int type, zval *return_value TSRMLS_DC)
{
	zval *stat_dev, *stat_ino, *stat_mode, *stat_nlink, *stat_uid, *stat_gid, *stat_rdev,
		 *stat_size, *stat_atime, *stat_mtime, *stat_ctime, *stat_blksize, *stat_blocks;
	int rmask=S_IROTH, wmask=S_IWOTH, xmask=S_IXOTH; /* access rights defaults to other */
	char *stat_sb_names[13] = {
		"dev", "ino", "mode", "nlink", "uid", "gid", "rdev",
		"size", "atime", "mtime", "ctime", "blksize", "blocks"
	};

#ifndef NETWARE
	if (type >= FS_IS_W && type <= FS_IS_X) {
		if(stat_sb->st_uid==getuid()) {
			rmask=S_IRUSR;
			wmask=S_IWUSR;
			xmask=S_IXUSR;
		} else if(stat_sb->st_gid==getgid()) {
			rmask=S_IRGRP;
			wmask=S_IWGRP;
			xmask=S_IXGRP;
		} else {
			int   groups, n, i;
			gid_t *gids;

			groups = getgroups(0, NULL);
			if(groups > 0) {
				gids=(gid_t *)safe_emalloc(groups, sizeof(gid_t), 0);
				n=getgroups(groups, gids);
				for(i=0;i<n;i++){
					if(stat_sb->st_gid==gids[i]) {
						rmask=S_IRGRP;
						wmask=S_IWGRP;
						xmask=S_IXGRP;
						break;
					}
				}
				efree(gids);
			}
		}
	}
#endif

	switch (type) {
	case FS_PERMS:
		RETURN_LONG((long)stat_sb->st_mode);
	case FS_INODE:
		RETURN_LONG((long)stat_sb->st_ino);
	case FS_SIZE:
		RETURN_LONG((long)stat_sb->st_size);
	case FS_OWNER:
		RETURN_LONG((long)stat_sb->st_uid);
	case FS_GROUP:
		RETURN_LONG((long)stat_sb->st_gid);
	case FS_ATIME:
#ifdef NETWARE
		RETURN_LONG((long)stat_sb->st_atime.tv_sec);
#else
		RETURN_LONG((long)stat_sb->st_atime);
#endif
	case FS_MTIME:
#ifdef NETWARE
		RETURN_LONG((long)stat_sb->st_mtime.tv_sec);
#else
		RETURN_LONG((long)stat_sb->st_mtime);
#endif
	case FS_CTIME:
#ifdef NETWARE
		RETURN_LONG((long)stat_sb->st_ctime.tv_sec);
#else
		RETURN_LONG((long)stat_sb->st_ctime);
#endif
	case FS_TYPE:
		if (S_ISLNK(stat_sb->st_mode)) {
			RETURN_STRING("link", 1);
		}
		switch(stat_sb->st_mode & S_IFMT) {
		case S_IFIFO: RETURN_STRING("fifo", 1);
		case S_IFCHR: RETURN_STRING("char", 1);
		case S_IFDIR: RETURN_STRING("dir", 1);
		case S_IFBLK: RETURN_STRING("block", 1);
		case S_IFREG: RETURN_STRING("file", 1);
#if defined(S_IFSOCK) && !defined(ZEND_WIN32)&&!defined(__BEOS__)
		case S_IFSOCK: RETURN_STRING("socket", 1);
#endif
		}
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Unknown file type (%d)", stat_sb->st_mode&S_IFMT);
		RETURN_STRING("unknown", 1);
	case FS_IS_W:
		RETURN_BOOL((stat_sb->st_mode & wmask) != 0);
	case FS_IS_R:
		RETURN_BOOL((stat_sb->st_mode&rmask)!=0);
	case FS_IS_X:
		RETURN_BOOL((stat_sb->st_mode&xmask)!=0 && !S_ISDIR(stat_sb->st_mode));
	case FS_IS_FILE:
		RETURN_BOOL(S_ISREG(stat_sb->st_mode));
	case FS_IS_DIR:
		RETURN_BOOL(S_ISDIR(stat_sb->st_mode));
	case FS_IS_LINK:
		RETURN_BOOL(S_ISLNK(stat_sb->st_mode));
	case FS_EXISTS:
		RETURN_TRUE; /* the false case was done earlier */
	case FS_LSTAT:
		/* FALLTHROUGH */
	case FS_STAT:
		array_init(return_value);

		MAKE_LONG_ZVAL_INCREF(stat_dev, stat_sb->st_dev);
		MAKE_LONG_ZVAL_INCREF(stat_ino, stat_sb->st_ino);
		MAKE_LONG_ZVAL_INCREF(stat_mode, stat_sb->st_mode);
		MAKE_LONG_ZVAL_INCREF(stat_nlink, stat_sb->st_nlink);
		MAKE_LONG_ZVAL_INCREF(stat_uid, stat_sb->st_uid);
		MAKE_LONG_ZVAL_INCREF(stat_gid, stat_sb->st_gid);
#ifdef HAVE_ST_RDEV
		MAKE_LONG_ZVAL_INCREF(stat_rdev, stat_sb->st_rdev);
#else
		MAKE_LONG_ZVAL_INCREF(stat_rdev, -1);
#endif
		MAKE_LONG_ZVAL_INCREF(stat_size, stat_sb->st_size);
#ifdef NETWARE
		MAKE_LONG_ZVAL_INCREF(stat_atime, (stat_sb->st_atime).tv_sec);
		MAKE_LONG_ZVAL_INCREF(stat_mtime, (stat_sb->st_mtime).tv_sec);
		MAKE_LONG_ZVAL_INCREF(stat_ctime, (stat_sb->st_ctime).tv_sec);
#else
		MAKE_LONG_ZVAL_INCREF(stat_atime, stat_sb->st_atime);
		MAKE_LONG_ZVAL_INCREF(stat_mtime, stat_sb->st_mtime);
		MAKE_LONG_ZVAL_INCREF(stat_ctime, stat_sb->st_ctime);
#endif
#ifdef HAVE_ST_BLKSIZE
		MAKE_LONG_ZVAL_INCREF(stat_blksize, stat_sb->st_blksize);
#else
		MAKE_LONG_ZVAL_INCREF(stat_blksize,-1);
#endif
#ifdef HAVE_ST_BLOCKS
		MAKE_LONG_ZVAL_INCREF(stat_blocks, stat_sb->st_blocks);
#else
		MAKE_LONG_ZVAL_INCREF(stat_blocks,-1);
#endif
		/* Store numeric indexes in propper order */
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_dev, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_ino, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_mode, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_nlink, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_uid, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_gid, sizeof(zval *), NULL);

		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_rdev, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_size, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_atime, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_mtime, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_ctime, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_blksize, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_blocks, sizeof(zval *), NULL);

		/* Store string indexes referencing the same zval*/
		zend_hash_update(HASH_OF(return_value), stat_sb_names[0], strlen(stat_sb_names[0])+1, (void *) &stat_dev, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[1], strlen(stat_sb_names[1])+1, (void *) &stat_ino, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[2], strlen(stat_sb_names[2])+1, (void *) &stat_mode, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[3], strlen(stat_sb_names[3])+1, (void *) &stat_nlink, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[4], strlen(stat_sb_names[4])+1, (void *) &stat_uid, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[5], strlen(stat_sb_names[5])+1, (void *) &stat_gid, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[6], strlen(stat_sb_names[6])+1, (void *) &stat_rdev, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[7], strlen(stat_sb_names[7])+1, (void *) &stat_size, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[8], strlen(stat_sb_names[8])+1, (void *) &stat_atime, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[9], strlen(stat_sb_names[9])+1, (void *) &stat_mtime, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[10], strlen(stat_sb_names[10])+1, (void *) &stat_ctime, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[11], strlen(stat_sb_names[11])+1, (void *) &stat_blksize, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[12], strlen(stat_sb_names[12])+1, (void *) &stat_blocks, sizeof(zval *), NULL);

		return;
	}
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Didn't understand stat call");
	RETURN_FALSE;
}
/* }}} */

void phar_file_stat(const char *filename, php_stat_len filename_length, int type, void (*orig_stat_func)(INTERNAL_FUNCTION_PARAMETERS), INTERNAL_FUNCTION_PARAMETERS) /* {{{ */
{
	if (!filename_length) {
		RETURN_FALSE;
	}

	if (!IS_ABSOLUTE_PATH(filename, filename_length)) {
		char *arch, *entry, *fname;
		int arch_len, entry_len, fname_len;
		struct stat sb = {0};
		phar_entry_info *data = NULL;
		char *tmp;
		int tmp_len;

		fname = zend_get_executed_filename(TSRMLS_C);

		/* we are checking for existence of a file within the relative path.  Chances are good that this is
		   retrieving something from within the phar archive */

		if (strncasecmp(fname, "phar://", 7)) {
			goto skip_phar;
		}
		fname_len = strlen(fname);
		if (SUCCESS == phar_split_fname(fname, fname_len, &arch, &arch_len, &entry, &entry_len TSRMLS_CC)) {
			phar_archive_data **pphar;

			efree(entry);
			entry = (char *)filename;
			/* fopen within phar, if :// is not in the url, then prepend phar://<archive>/ */
			entry_len = (int) filename_length;
			if (strstr(entry, "://")) {
				efree(arch);
				goto skip_phar;
			}
			if (SUCCESS == (zend_hash_find(&(PHAR_GLOBALS->phar_fname_map), arch, arch_len, (void **) &pphar))) {
				entry = phar_fix_filepath(entry, &entry_len, 1 TSRMLS_CC);
				if (SUCCESS == zend_hash_find(&((*pphar)->manifest), entry, entry_len, (void **) &data)) {
					efree(entry);
					goto stat_entry;
				} else {
					char *save = PHAR_G(cwd), *save2 = entry;
					int save_len = PHAR_G(cwd_len), save2_len = entry_len;

					/* this file is not in the current directory, use the original path */
					entry = (char *)filename;
					PHAR_G(cwd) = "/";
					PHAR_G(cwd_len) = 0;
					/* clean path without cwd */
					entry = phar_fix_filepath(entry, &entry_len, 1 TSRMLS_CC);
					if (SUCCESS == zend_hash_find(&((*pphar)->manifest), entry, entry_len, (void **) &data)) {
						PHAR_G(cwd) = save;
						PHAR_G(cwd_len) = save_len;
						efree(entry);
						efree(save2);
						goto stat_entry;
					} else {
						phar_archive_data *phar = *pphar;
						char *key;
						uint keylen;
						ulong unused;

						/* original not found either, this is possibly a directory relative to cwd */
						zend_hash_internal_pointer_reset(&phar->manifest);
						while (FAILURE != zend_hash_has_more_elements(&phar->manifest)) {
							if (HASH_KEY_NON_EXISTANT !=
									zend_hash_get_current_key_ex(
										&phar->manifest, &key, &keylen, &unused, 0, NULL)) {
								if (0 == memcmp(save2, key, save2_len)) {
									PHAR_G(cwd) = save;
									PHAR_G(cwd_len) = save_len;
									efree(save2);
									efree(entry);
									/* directory found, all dirs have the same stat */
									if (key[save2_len] == '/') {
										sb.st_size = 0;
										sb.st_mode = 0777;
										sb.st_mode |= S_IFDIR; /* regular directory */
#ifdef NETWARE
										sb.st_mtime.tv_sec = phar->max_timestamp;
										sb.st_atime.tv_sec = phar->max_timestamp;
										sb.st_ctime.tv_sec = phar->max_timestamp;
#else
										sb.st_mtime = phar->max_timestamp;
										sb.st_atime = phar->max_timestamp;
										sb.st_ctime = phar->max_timestamp;
#endif
										goto statme_baby;
									}
								}
							}
							if (SUCCESS != zend_hash_move_forward(&phar->manifest)) {
								break;
							}
						}
					}
					PHAR_G(cwd) = save;
					PHAR_G(cwd_len) = save_len;
					efree(entry);
					efree(save2);
					/* Error Occured */
					if (!IS_EXISTS_CHECK(type)) {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "%sstat failed for %s", IS_LINK_OPERATION(type) ? "L" : "", filename);
					}
					RETURN_FALSE;
				}
			}
stat_entry:
			if (!data->is_dir) {
				sb.st_size = data->uncompressed_filesize;
				sb.st_mode = data->flags & PHAR_ENT_PERM_MASK;
				sb.st_mode |= S_IFREG; /* regular file */
				/* timestamp is just the timestamp when this was added to the phar */
#ifdef NETWARE
				sb.st_mtime.tv_sec = data->timestamp;
				sb.st_atime.tv_sec = data->timestamp;
				sb.st_ctime.tv_sec = data->timestamp;
#else
				sb.st_mtime = data->timestamp;
				sb.st_atime = data->timestamp;
				sb.st_ctime = data->timestamp;
#endif
			} else {
				sb.st_size = 0;
				sb.st_mode = data->flags & PHAR_ENT_PERM_MASK;
				sb.st_mode |= S_IFDIR; /* regular directory */
				/* timestamp is just the timestamp when this was added to the phar */
#ifdef NETWARE
				sb.st_mtime.tv_sec = data->timestamp;
				sb.st_atime.tv_sec = data->timestamp;
				sb.st_ctime.tv_sec = data->timestamp;
#else
				sb.st_mtime = data->timestamp;
				sb.st_atime = data->timestamp;
				sb.st_ctime = data->timestamp;
#endif
			}

statme_baby:
			efree(arch);
			if (!(*pphar)->is_writeable) {
				sb.st_mode = (sb.st_mode & 0555) | (sb.st_mode & ~0777);
			}
		
			sb.st_nlink = 1;
			sb.st_rdev = -1;
			if (data) {
				tmp_len = data->filename_len + (*pphar)->alias_len;
			} else {
				tmp_len = (*pphar)->alias_len + 1;
			}
			tmp = (char *) emalloc(tmp_len);
			memcpy(tmp, (*pphar)->alias, (*pphar)->alias_len);
			if (data) {
				memcpy(tmp + (*pphar)->alias_len, data->filename, data->filename_len);
			} else {
				*(tmp + (*pphar)->alias_len) = '/';
			}
			/* this is only for APC, so use /dev/null device - no chance of conflict there! */
			sb.st_dev = 0xc;
			/* generate unique inode number for alias/filename, so no phars will conflict */
			sb.st_ino = (unsigned short)zend_get_hash_value(tmp, tmp_len);
			efree(tmp);
#ifndef PHP_WIN32
			sb.st_blksize = -1;
			sb.st_blocks = -1;
#endif
			phar_fancy_stat(&sb, type, return_value TSRMLS_CC);
			return;
		}
	}
skip_phar:
	orig_stat_func(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	return;
}
/* }}} */

#define PharFileFunction(fname, funcnum, orig) \
void fname(INTERNAL_FUNCTION_PARAMETERS) { \
	char *filename; \
	int filename_len; \
	\
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) { \
		return; \
	} \
	\
	phar_file_stat(filename, (php_stat_len) filename_len, funcnum, PHAR_G(orig), INTERNAL_FUNCTION_PARAM_PASSTHRU); \
}
/* }}} */

/* {{{ proto int fileperms(string filename)
   Get file permissions */
PharFileFunction(phar_fileperms, FS_PERMS, orig_fileperms)
/* }}} */

/* {{{ proto int fileinode(string filename)
   Get file inode */
PharFileFunction(phar_fileinode, FS_INODE, orig_fileinode)
/* }}} */

/* {{{ proto int filesize(string filename)
   Get file size */
PharFileFunction(phar_filesize, FS_SIZE, orig_filesize)
/* }}} */

/* {{{ proto int fileowner(string filename)
   Get file owner */
PharFileFunction(phar_fileowner, FS_OWNER, orig_fileowner)
/* }}} */

/* {{{ proto int filegroup(string filename)
   Get file group */
PharFileFunction(phar_filegroup, FS_GROUP, orig_filegroup)
/* }}} */

/* {{{ proto int fileatime(string filename)
   Get last access time of file */
PharFileFunction(phar_fileatime, FS_ATIME, orig_fileatime)
/* }}} */

/* {{{ proto int filemtime(string filename)
   Get last modification time of file */
PharFileFunction(phar_filemtime, FS_MTIME, orig_filemtime)
/* }}} */

/* {{{ proto int filectime(string filename)
   Get inode modification time of file */
PharFileFunction(phar_filectime, FS_CTIME, orig_filectime)
/* }}} */

/* {{{ proto string filetype(string filename)
   Get file type */
PharFileFunction(phar_filetype, FS_TYPE, orig_filetype)
/* }}} */

/* {{{ proto bool is_writable(string filename)
   Returns true if file can be written */
PharFileFunction(phar_is_writable, FS_IS_W, orig_is_writable)
/* }}} */

/* {{{ proto bool is_readable(string filename)
   Returns true if file can be read */
PharFileFunction(phar_is_readable, FS_IS_R, orig_is_readable)
/* }}} */

/* {{{ proto bool is_executable(string filename)
   Returns true if file is executable */
PharFileFunction(phar_is_executable, FS_IS_X, orig_is_executable)
/* }}} */

/* {{{ proto bool is_executable(string filename)
   Returns true if file is executable */
PharFileFunction(phar_file_exists, FS_EXISTS, orig_file_exists)
/* }}} */

/* {{{ proto bool is_executable(string filename)
   Returns true if file is executable */
PharFileFunction(phar_is_dir, FS_IS_DIR, orig_is_dir)
/* }}} */

PHAR_FUNC(phar_is_file) /* {{{ */
{
	char *filename;
	int filename_len;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
		goto skip_phar;
	}
	if (!IS_ABSOLUTE_PATH(filename, filename_len)) {
		char *arch, *entry, *fname;
		int arch_len, entry_len, fname_len, free_filename = 0;
		fname = zend_get_executed_filename(TSRMLS_C);

		/* we are checking for existence of a file within the relative path.  Chances are good that this is
		   retrieving something from within the phar archive */

		if (strncasecmp(fname, "phar://", 7)) {
			goto skip_phar;
		}
		fname_len = strlen(fname);
		if (SUCCESS == phar_split_fname(fname, fname_len, &arch, &arch_len, &entry, &entry_len TSRMLS_CC)) {
			phar_archive_data **pphar;

			efree(entry);
			entry = filename;
			/* fopen within phar, if :// is not in the url, then prepend phar://<archive>/ */
			entry_len = filename_len;
			if (strstr(entry, "://")) {
				efree(arch);
				if (free_filename) {
					efree(filename);
				}
				goto skip_phar;
			}
			/* retrieving a file within the current directory, so use this if possible */
			if (SUCCESS == (zend_hash_find(&(PHAR_GLOBALS->phar_fname_map), arch, arch_len, (void **) &pphar))) {
				entry = phar_fix_filepath(entry, &entry_len, 1 TSRMLS_CC);
				if (zend_hash_exists(&((*pphar)->manifest), entry, entry_len)) {
					/* this file is not in the current directory, use the original path */
					efree(entry);
					efree(arch);
					RETURN_TRUE;
				}
			}
			efree(entry);
			efree(arch);
			RETURN_FALSE;
		}
	}
skip_phar:
	PHAR_G(orig_is_file)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	return;
}
/* }}} */

PHAR_FUNC(phar_is_link) /* {{{ */
{
	char *filename;
	int filename_len;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
		goto skip_phar;
	}
	if (!IS_ABSOLUTE_PATH(filename, filename_len)) {
		char *arch, *entry, *fname;
		int arch_len, entry_len, fname_len, free_filename = 0;
		fname = zend_get_executed_filename(TSRMLS_C);

		/* we are checking for existence of a file within the relative path.  Chances are good that this is
		   retrieving something from within the phar archive */

		if (strncasecmp(fname, "phar://", 7)) {
			goto skip_phar;
		}
		fname_len = strlen(fname);
		if (SUCCESS == phar_split_fname(fname, fname_len, &arch, &arch_len, &entry, &entry_len TSRMLS_CC)) {
			phar_archive_data **pphar;

			efree(entry);
			entry = filename;
			/* fopen within phar, if :// is not in the url, then prepend phar://<archive>/ */
			entry_len = filename_len;
			if (strstr(entry, "://")) {
				efree(arch);
				if (free_filename) {
					efree(filename);
				}
				goto skip_phar;
			}
			/* retrieving a file within the current directory, so use this if possible */
			if (SUCCESS == (zend_hash_find(&(PHAR_GLOBALS->phar_fname_map), arch, arch_len, (void **) &pphar))) {
				phar_entry_info *etemp;

				entry = phar_fix_filepath(entry, &entry_len, 1 TSRMLS_CC);
				if (SUCCESS == zend_hash_find(&((*pphar)->manifest), entry, entry_len, (void **) &etemp)) {
					/* this file is not in the current directory, use the original path */
					efree(entry);
					efree(arch);
					RETURN_BOOL(etemp->link);
				}
			}
			efree(entry);
			efree(arch);
			RETURN_FALSE;
		}
	}
skip_phar:
	PHAR_G(orig_file_exists)(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	return;
}
/* }}} */

/* {{{ proto array lstat(string filename)
   Give information about a file or symbolic link */
PharFileFunction(phar_lstat, FS_LSTAT, orig_lstat)
/* }}} */

/* {{{ proto array stat(string filename)
   Give information about a file */
PharFileFunction(phar_stat, FS_STAT, orig_stat)
/* }}} */

/* {{{ void phar_intercept_functions(TSRMLS_D) */
#define PHAR_INTERCEPT(func) \
	PHAR_G(orig_##func) = NULL; \
	if (SUCCESS == zend_hash_find(CG(function_table), #func, sizeof(#func), (void **)&orig)) { \
		PHAR_G(orig_##func) = orig->internal_function.handler; \
		orig->internal_function.handler = phar_##func; \
	}

void phar_intercept_functions(TSRMLS_D)
{
	zend_function *orig;

	if (!PHAR_G(request_init)) {
		PHAR_G(cwd) = NULL;
		PHAR_G(cwd_len) = 0;
	} else if (PHAR_G(orig_fopen)) {
		/* don't double-intercept */
		return;
	}
	PHAR_INTERCEPT(fopen);
	PHAR_INTERCEPT(file_get_contents);
	PHAR_INTERCEPT(is_file);
	PHAR_INTERCEPT(is_link);
	PHAR_INTERCEPT(is_dir);
	PHAR_INTERCEPT(opendir);
	PHAR_INTERCEPT(file_exists);
	PHAR_INTERCEPT(fileperms);
	PHAR_INTERCEPT(fileinode);
	PHAR_INTERCEPT(filesize);
	PHAR_INTERCEPT(fileowner);
	PHAR_INTERCEPT(filegroup);
	PHAR_INTERCEPT(fileatime);
	PHAR_INTERCEPT(filemtime);
	PHAR_INTERCEPT(filectime);
	PHAR_INTERCEPT(filetype);
	PHAR_INTERCEPT(is_writable);
	PHAR_INTERCEPT(is_readable);
	PHAR_INTERCEPT(is_executable);
	PHAR_INTERCEPT(lstat);
	PHAR_INTERCEPT(stat);
}
/* }}} */

/* {{{ void phar_release_functions(TSRMLS_D) */
#define PHAR_RELEASE(func) \
	if (PHAR_G(orig_##func) && SUCCESS == zend_hash_find(CG(function_table), #func, sizeof(#func), (void **)&orig)) { \
		orig->internal_function.handler = PHAR_G(orig_##func); \
	} \
	PHAR_G(orig_##func) = NULL;

void phar_release_functions(TSRMLS_D)
{
	zend_function *orig;

	PHAR_RELEASE(fopen);
	PHAR_RELEASE(file_get_contents);
	PHAR_RELEASE(is_file);
	PHAR_RELEASE(is_dir);
	PHAR_RELEASE(opendir);
	PHAR_RELEASE(file_exists);
	PHAR_RELEASE(fileperms);
	PHAR_RELEASE(fileinode);
	PHAR_RELEASE(filesize);
	PHAR_RELEASE(fileowner);
	PHAR_RELEASE(filegroup);
	PHAR_RELEASE(fileatime);
	PHAR_RELEASE(filemtime);
	PHAR_RELEASE(filectime);
	PHAR_RELEASE(filetype);
	PHAR_RELEASE(is_writable);
	PHAR_RELEASE(is_readable);
	PHAR_RELEASE(is_executable);
	PHAR_RELEASE(lstat);
	PHAR_RELEASE(stat);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */