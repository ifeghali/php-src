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
   | Authors: Sascha Schumann <ss@2ns.de>                                 |
   +----------------------------------------------------------------------+
 */


#include "php.h"

#include <sys/stat.h>
#include <sys/types.h>

#if HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifdef PHP_WIN32
#include "win32/readdir.h"
#endif
#include <time.h>

#include <fcntl.h>
#include <errno.h>

#include "php_session.h"
#include "mod_files.h"
#include "ext/standard/flock_compat.h"

#define FILE_PREFIX "sess_"

typedef struct {
	int fd;
	char *lastkey;
	char *basedir;
	int dirdepth;
} ps_files;

ps_module ps_mod_files = {
	PS_MOD(files)
};

#ifdef PHP_WIN32
#define DIR_DELIMITER '\\'
#else
#define DIR_DELIMITER '/'
#endif

static int _ps_files_valid_key(const char *key)
{
	size_t len;
	const char *p;
	char c;
	int ret = 1;

	for (p = key; (c = *p); p++) {
		/* valid characters are a..z,A..Z,0..9 */
		if (!((c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9'))) {
			ret = 0;
			break;
		}
	}

	len = p - key;
	
	if (len == 0)
		ret = 0;
	
	return ret;
}

static char *_ps_files_path_create(char *buf, size_t buflen, ps_files *data, const char *key)
{
	int keylen;
	const char *p;
	int i;
	int n;
	
	keylen = strlen(key);
	if (keylen <= data->dirdepth || buflen < 
			(strlen(data->basedir) + 2 * data->dirdepth + keylen + 5 + sizeof(FILE_PREFIX))) 
		return NULL;
	p = key;
	n = sprintf(buf, "%s/", data->basedir);
	for (i = 0; i < data->dirdepth; i++) {
		buf[n++] = *p++;
		buf[n++] = DIR_DELIMITER;
	}
	buf[n] = '\0';
	strcat(buf, FILE_PREFIX);
	strcat(buf, key);
	
	return buf;
}

static void _ps_files_open(ps_files *data, const char *key)
{
	char buf[MAXPATHLEN];

	if (data->fd < 0 || !data->lastkey || strcmp(key, data->lastkey)) {
		if (data->lastkey) {
			efree(data->lastkey);
			data->lastkey = NULL;
		}
		if (data->fd != -1) {
			close(data->fd);
			data->fd = -1;
		}
		
		if (!_ps_files_valid_key(key) || 
				!_ps_files_path_create(buf, sizeof(buf), data, key))
			return;
		
		data->lastkey = estrdup(key);
		
#ifdef O_EXCL
		data->fd = V_OPEN((buf, O_RDWR));
		if (data->fd == -1) {
			if (errno == ENOENT) {
				data->fd = V_OPEN((buf, O_EXCL | O_RDWR | O_CREAT, 0600));
			}
		} else {
			flock(data->fd, LOCK_EX);
		}
#else
		data->fd = V_OPEN((buf, O_CREAT | O_RDWR, 0600));
		if (data->fd != -1)
			flock(data->fd, LOCK_EX);
#endif

		if (data->fd == -1)
			php_error(E_WARNING, "open(%s, O_RDWR) failed: %m (%d)", buf, errno);
	}
}

static int _ps_files_cleanup_dir(const char *dirname, int maxlifetime)
{
	DIR *dir;
	struct dirent *entry, dentry;
	struct stat sbuf;
	char buf[MAXPATHLEN];
	time_t now;
	int nrdels = 0;

	dir = opendir(dirname);
	if (!dir) {
		php_error(E_NOTICE, "_ps_files_cleanup_dir: opendir(%s) failed: %m (%d)\n", dirname, errno);
		return (0);
	}

	time(&now);

	while (php_readdir_r(dir, &dentry, &entry) == 0 && entry) {
		/* does the file start with our prefix? */
		if (!strncmp(entry->d_name, FILE_PREFIX, sizeof(FILE_PREFIX) - 1) &&
				/* create full path */
				snprintf(buf, MAXPATHLEN, "%s%c%s", dirname, DIR_DELIMITER,
					entry->d_name) > 0 &&
				/* stat the directory entry */
				V_STAT(buf, &sbuf) == 0 &&
				/* is it expired? */
				(now - sbuf.st_atime) > maxlifetime) {
			V_UNLINK(buf);
			nrdels++;
		}
	}

	closedir(dir);

	return (nrdels);
}

#define PS_FILES_DATA ps_files *data = PS_GET_MOD_DATA()

PS_OPEN_FUNC(files)
{
	ps_files *data;
	char *p;

	data = ecalloc(sizeof(*data), 1);
	PS_SET_MOD_DATA(data);

	data->fd = -1;
	if ((p = strchr(save_path, ';'))) {
		data->dirdepth = strtol(save_path, NULL, 10);
		save_path = p + 1;
	}
	data->basedir = estrdup(save_path);
	
	return SUCCESS;
}

PS_CLOSE_FUNC(files)
{
	PS_FILES_DATA;

	if (data->fd > -1) 
		close(data->fd);
	if (data->lastkey) 
		efree(data->lastkey);
	efree(data->basedir);
	efree(data);
	*mod_data = NULL;

	return SUCCESS;
}

PS_READ_FUNC(files)
{
	int n;
	struct stat sbuf;
	PS_FILES_DATA;

	_ps_files_open(data, key);
	if (data->fd < 0)
		return FAILURE;
	
	if (fstat(data->fd, &sbuf))
		return FAILURE;
	
	lseek(data->fd, 0, SEEK_SET);

	*vallen = sbuf.st_size;
	*val = emalloc(sbuf.st_size);

	n = read(data->fd, *val, sbuf.st_size);
	if (n != sbuf.st_size) {
		efree(*val);
		return FAILURE;
	}
	
	return SUCCESS;
}

PS_WRITE_FUNC(files)
{
	PS_FILES_DATA;

	_ps_files_open(data, key);
	if (data->fd < 0)
		return FAILURE;

	ftruncate(data->fd, 0);
	lseek(data->fd, 0, SEEK_SET);
	if (write(data->fd, val, vallen) != vallen) {
		php_error(E_WARNING, "write failed: %m (%d)", errno);
		return FAILURE;
	}

	return SUCCESS;
}

PS_DESTROY_FUNC(files)
{
	char buf[MAXPATHLEN];
	PS_FILES_DATA;

	if (!_ps_files_path_create(buf, sizeof(buf), data, key))
		return FAILURE;
	
	V_UNLINK(buf);

	return SUCCESS;
}

PS_GC_FUNC(files) 
{
	PS_FILES_DATA;
	
	/* we don't perform any cleanup, if dirdepth is larger than 0.
	   we return SUCCESS, since all cleanup should be handled by
	   an external entity (i.e. find -ctime x | xargs rm) */
	   
	if (data->dirdepth == 0)
		*nrdels = _ps_files_cleanup_dir(data->basedir, maxlifetime);
	
	return SUCCESS;
}
