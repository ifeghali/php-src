/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000, 2001 The PHP Group             |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Stig S�ther Bakken <ssb@fast.no>                            |
   |                                                                      |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "php.h"
#include "php_versioning.h"

#define sign(n) ((n)<0?-1:((n)>0?1:0))

/* {{{ php_canonicalize_version() */

PHPAPI char *
php_canonicalize_version(const char *version)
{
    int len = strlen(version);
    char *buf = emalloc(len * 2 + 1), *q, lp, lq;
    const char *p;

    if (len == 0) {
        *buf = '\0';
        return buf;
    }

    p = version;
    q = buf;
    *q++ = lp = *p++;
    lq = '\0';
    while (*p) {
/*  s/[-_+]/./g;
 *  s/([^\d\.])([^\D\.])/$1.$2/g;
 *  s/([^\D\.])([^\d\.])/$1.$2/g;
 */
#define isdigdot(x) (isdigit(x)||(x)=='.')
#define isspecialver(x) ((x)=='-'||(x)=='_'||(x)=='+')

        lq = *(q - 1);
        if ((isdigdot(*p) != isdigdot(lp) || isspecialver(*p)) &&
            (lq != '.' && *p != '.')) {
            lq = *q;
            *q++ = '.';
        }
        *q++ = lp = *p++;
    }
    *q++ = '\0';
    return buf;
}

/* }}} */
/* {{{ compare_special_version_forms() */

static int
compare_special_version_forms(const char *form1, const char *form2)
{
	int i, found1 = -1, found2 = -1;
	char **pp;
	static char *special_forms[] = {
		"dev",
		"a",
		"b",
		"RC",
		"#N#",
		"pl",
		NULL
	};

	for (pp = special_forms, i = 0; *pp != NULL; pp++, i++) {
		if (strncmp(form1, *pp, strlen(*pp)) == 0) {
			found1 = i;
			break;
		}
	}
	for (pp = special_forms, i = 0; *pp != NULL; pp++, i++) {
		if (strncmp(form2, *pp, strlen(*pp)) == 0) {
			found2 = i;
			break;
		}
	}
	return sign(found1 - found2);
}

/* }}} */
/* {{{ php_version_compare() */

PHPAPI int
php_version_compare(const char *orig_ver1, const char *orig_ver2)
{
	char *ver1 = php_canonicalize_version(orig_ver1);
	char *ver2 = php_canonicalize_version(orig_ver2);
	char *p1, *p2, *n1, *n2;
	long l1, l2;
	int compare = 0;

	p1 = n1 = ver1;
	p2 = n2 = ver2;
	while (*p1 && *p2 && n1 && n2) {
		if ((n1 = strchr(p1, '.')) != NULL) {
			*n1 = '\0';
		}
		if ((n2 = strchr(p2, '.')) != NULL) {
			*n2 = '\0';
		}
		if (isdigit(*p1) && isdigit(*p2)) {
			/* compare element numerically */
			l1 = strtol(p1, NULL, 10);
			l2 = strtol(p2, NULL, 10);
			compare = sign(l1 - l2);
		} else if (!isdigit(*p1) && !isdigit(*p2)) {
			/* compare element names */
			compare = compare_special_version_forms(p1, p2);
		} else {
			/* mix of names and numbers */
			if (isdigit(*p1)) {
				compare = compare_special_version_forms("#N#", p2);
			} else {
				compare = compare_special_version_forms(p1, "#N#");
			}
		}
		if (compare != 0) {
			break;
		}
		if (n1 != NULL) {
			p1 = n1 + 1;
		}
		if (n2 != NULL) {
			p2 = n2 + 1;
		}
	}
	if (compare == 0) {
		if (n1 != NULL) {
			if (isdigit(*p1)) {
				compare = 1;
			} else {
				compare = php_version_compare(p1, "#N#");
			}
		} else if (n2 != NULL) {
			if (isdigit(*p2)) {
				compare = -1;
			} else {
				compare = php_version_compare("#N#", p2);
			}
		}
	}
	efree(ver1);
	efree(ver2);
	return compare;
}

/* }}} */
/* {{{ do_version_compare() */

/* {{{ proto int version_compare(string ver1, string ver2 [, string oper])
  Compares two "PHP-standardized" version number strings */

PHP_FUNCTION(version_compare)
{
	char *v1, *v2, *op;
	int v1_len, v2_len, op_len;
	int compare, argc;

	argc = ZEND_NUM_ARGS();
	if (zend_parse_parameters(argc TSRMLS_CC, "ss|s", &v1, &v1_len, &v2,
							  &v2_len, &op, &op_len) == FAILURE) {
		return;
	}
	compare = php_version_compare(v1, v2);
	if (argc == 2) {
		RETURN_LONG(compare);
	}
	if (!strcmp(op, "<") || !strcmp(op, "lt")) {
		RETURN_LONG(compare == -1);
	}
	if (!strcmp(op, "<=") || !strcmp(op, "le")) {
		RETURN_LONG(compare != 1);
	}
	if (!strcmp(op, ">") || !strcmp(op, "gt")) {
		RETURN_LONG(compare == 1);
	}
	if (!strcmp(op, ">=") || !strcmp(op, "ge")) {
		RETURN_LONG(compare != -1);
	}
	if (!strcmp(op, "==") || !strcmp(op, "=") || !strcmp(op, "eq")) {
		RETURN_LONG(compare == 0);
	}
	if (!strcmp(op, "!=") || !strcmp(op, "<>") || !strcmp(op, "ne")) {
		RETURN_LONG(compare != 0);
	}
	RETURN_NULL();
}

/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
