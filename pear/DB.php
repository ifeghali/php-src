<?php
//
// +----------------------------------------------------------------------+
// | PHP version 4.0                                                      |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available at through the world-wide-web at                           |
// | http://www.php.net/license/2_0.txt.                                  |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Stig Bakken <ssb@fast.no>                                   |
// |                                                                      |
// +----------------------------------------------------------------------+
//
// $Id$
//
// Database independent query interface.
//

if ($GLOBALS['USED_PACKAGES']['DB']) return;
$GLOBALS['USED_PACKAGES']['DB'] = true;

// {{{ Database independent error codes.

/*
 * The method mapErrorCode in each DB_dbtype implementation maps
 * native error codes to one of these.
 *
 * If you add an error code here, make sure you also add a textual
 * version of it in DB::errorMessage().
 */
define("DB_OK",                    	 0);
define("DB_ERROR",                 	-1);
define("DB_ERROR_SYNTAX",          	-2);
define("DB_ERROR_CONSTRAINT",      	-3);
define("DB_ERROR_NOT_FOUND",       	-4);
define("DB_ERROR_ALREADY_EXISTS",  	-5);
define("DB_ERROR_UNSUPPORTED",     	-6);
define("DB_ERROR_MISMATCH",        	-7);
define("DB_ERROR_INVALID",         	-8);
define("DB_ERROR_NOT_CAPABLE",     	-9);
define("DB_ERROR_TRUNCATED",       -10);
define("DB_ERROR_INVALID_NUMBER",  -11);
define("DB_ERROR_INVALID_DATE",    -12);
define("DB_ERROR_DIVZERO",         -13);
define("DB_ERROR_NODBSELECTED",    -14);
define("DB_ERROR_CANNOT_CREATE",   -15);
define("DB_ERROR_CANNOT_DELETE",   -16);
define("DB_ERROR_CANNOT_DROP",     -17);
define("DB_ERROR_NOSUCHTABLE",     -18);
define("DB_ERROR_NOSUCHFIELD",     -19);
define("DB_ERROR_NEED_MORE_DATA",  -20);

/*
 * Warnings are not detected as errors by DB::isError(), and are not
 * fatal.  You can detect whether an error is in fact a warning with
 * DB::isWarning().
 */
define("DB_WARNING_READ_ONLY",   -1000);

// }}}
// {{{ Prepare/execute parameter types

/*
 * These constants are used when storing information about prepared
 * statements (using the "prepare" method in DB_dbtype).
 *
 * The prepare/execute model in DB is mostly borrowed from the ODBC
 * extension, in a query the "?" character means a scalar parameter.
 * There is one extension though, a "*" character means an opaque
 * parameter.  An opaque parameter is simply a file name, the real
 * data are in that file (useful for stuff like putting uploaded files
 * into your database).
 */
define("DB_PARAM_SCALAR",           1);
define("DB_PARAM_OPAQUE",           2);

// }}}
// {{{ Binary data modes

/*
 * These constants define different ways of returning binary data
 * from queries.  Again, this model has been borrowed from the ODBC
 * extension.
 *
 * DB_BINMODE_PASSTHRU sends the data directly through to the browser
 * when data is fetched from the database.
 * DB_BINMODE_RETURN lets you return data as usual.
 * DB_BINMODE_CONVERT returns data as well, only it is converted to
 * hex format, for example the string "123" would become "313233".
 */
define("DB_BINMODE_PASSTHRU",       1);
define("DB_BINMODE_RETURN",         2);
define("DB_BINMODE_CONVERT",        3);

// }}}
// {{{ Get modes: flags that control the layout of query result structures

/**
 * Column data indexed by numbers, ordered from 0 and up
 */
define('DB_GETMODE_ORDERED', 1);
/**
 * Column data indexed by column names
 */
define('DB_GETMODE_ASSOC',   2);
/**
 * For multi-dimensional results: normally the first level of arrays
 * is the row number, and the second level indexed by column number or name.
 * DB_GETMODE_FLIPPED switches this order, so the first level of arrays
 * is the column name, and the second level the row number.
 */
define('DB_GETMODE_FLIPPED', 4);

/**
 * This constant DB's default get mode.  It is possible to override by
 * defining in your scripts before including DB.
 */
if (!defined('DB_GETMODE_DEFAULT')) {
	define('DB_GETMODE_DEFAULT', DB_GETMODE_ORDERED);
}

// }}}

/**
 * The main "DB" class is simply a container class with some static
 * methods for creating DB objects as well as some utility functions
 * common to all parts of DB.
 *
 * The object model of DB is as follows (indentation means inheritance):
 *
 * DB           The main DB class.  This is simply a utility class
 *              with some "static" methods for creating DB objects as
 *              well as common utility functions for other DB classes.
 * 
 * DB_common    The base for each DB implementation.  Provides default
 * |            implementations (in OO lingo virtual methods) for
 * |            the actual DB implementations as well as a bunch of
 * |            query utility functions.
 * |
 * +-DB_mysql   The DB implementation for MySQL.  Inherits DB_common.
 *              When calling DB::factory or DB::connect for MySQL
 *              connections, the object returned is an instance of this
 *              class.
 *
 * @version  1.00
 * @author   Stig Bakken <ssb@fast.no>
 * @since    PHP 4.0
 */
class DB {
    // {{{ factory()

	/**
	 * Create a new DB object for the specified database type
	 *
	 * @param $type string database type, for example "mysql"
	 *
	 * @return object a newly created DB object, or a DB error code on
	 * error
	 */
    function &factory($type) {
		global $USED_PACKAGES;
		// "include" should be replaced with "import" once PHP gets it
		$pkgname = 'DB/' . $type;
		if (!is_array($USED_PACKAGES) || !$USED_PACKAGES[$pkgname]) {
			if (!@include("${pkgname}.php")) {
				return DB_ERROR_NOT_FOUND;
			} else {
				$USED_PACKAGES[$pkgname] = true;
			}
		}
		$classname = 'DB_' . $type;
		$obj = new $classname;
		return $obj;
    }

    // }}}
    // {{{ connect()

	/**
	 * Create a new DB object and connect to the specified database
	 *
	 * @param $dsn string "data source name", see the DB::parseDSN
	 * method for a description of the dsn format.
	 *
	 * @param $persistent bool whether this connection should be
	 * persistent.  Ignored if the backend extension does not support
	 * persistent connections.
	 *
	 * @return object a newly created DB object, or a DB error code on
	 * error
	 */
	function &connect($dsn, $persistent = false) {
		global $USED_PACKAGES;

		$dsninfo = DB::parseDSN($dsn);
		$type = $dsninfo['phptype'];
		// "include" should be replaced with "import" once PHP gets it
		$pkgname = 'DB/' . $type;
		if (!is_array($USED_PACKAGES) || !$USED_PACKAGES[$pkgname]) {
			$file = "${pkgname}.php";
			include($file);
/*

   This suddenly stopped working.  I thought include() was supposed to
   return a boolean? [ssb 20000218]

			if (!include($file)) {
				print "'$file' not found<br>\n";
				return DB_ERROR_NOT_FOUND;
			} else {
				$USED_PACKAGES[$pkgname] = true;
			}
*/
		}
		$classname = 'DB_' . $type;
		$obj = new $classname;
		$err = $obj->connect(&$dsninfo, $persistent);
		if (DB::isError($err)) {
			return $err;
		}
		return $obj;
	}

    // }}}
    // {{{ apiVersion()

	/**
	 * Return the DB API version
	 *
	 * @return int the DB API version number
	 */
    function apiVersion() {
		return 1;
    }

    // }}}
    // {{{ isError()

	/**
	 * Tell whether a result code from a DB method is an error
	 *
	 * @param $code int result code
	 *
	 * @return bool whether $code is an error
	 */
	function isError($code) {
		return is_int($code) && ($code < 0) && ($code > -1000);
	}

    // }}}
    // {{{ isWarning()

	/**
	 * Tell whether a result code from a DB method is a warning.
	 * Warnings differ from errors in that they are generated by DB,
	 * and are not fatal.
	 *
	 * @param $code int result code
	 *
	 * @return bool whether $code is a warning
	 */
	function isWarning($code) {
		return is_int($code) && ($code <= -1000);
	}

    // }}}
    // {{{ errorMessage()

	/**
	 * Return a textual error message for a DB error code
	 *
	 * @param $code int error code
	 *
	 * @return string error message, or false if the error code was
	 * not recognized
	 */
	function errorMessage($code) {
		if (!is_array($errorMessages)) {
			$errorMessages = array(
				DB_OK                   => "no error",
				DB_ERROR                => "unknown error",
				DB_ERROR_SYNTAX         => "syntax error",
				DB_ERROR_CONSTRAINT     => "constraint violation",
				DB_ERROR_NOT_FOUND      => "not found",
				DB_ERROR_ALREADY_EXISTS => "already exists",
				DB_ERROR_UNSUPPORTED    => "not supported",
				DB_ERROR_MISMATCH       => "mismatch",
				DB_ERROR_INVALID        => "invalid",
				DB_ERROR_NOT_CAPABLE    => "DB implementation not capable",
				DB_ERROR_INVALID_NUMBER => "invalid number",
				DB_ERROR_INVALID_DATE   => "invalid date or time",
				DB_ERROR_DIVZERO        => "division by zero",
				DB_ERROR_NODBSELECTED   => "no database selected",
				DB_ERROR_CANNOT_CREATE  => "can not create",
				DB_ERROR_CANNOT_DELETE  => "can not delete",
				DB_ERROR_CANNOT_DROP    => "can not drop",
				DB_ERROR_NOSUCHTABLE    => "no such table",
				DB_ERROR_NOSUCHFIELD    => "no such field",
				DB_WARNING_READ_ONLY    => "warning: read only"
			);
		}
		return $errorMessages[$code];
	}

    // }}}
    // {{{ parseDSN()

	/**
	 * Parse a data source name
	 *
	 * @param $dsn string Data Source Name to be parsed
	 *
	 * @return array an associative array with the following keys:
	 * <dl>
	 *  <dt>phptype</dt>
	 *  <dd>Database backend used in PHP (mysql, odbc etc.)</dd>
	 *  <dt>dbsyntax</dt>
	 *  <dd>Database used with regards to SQL syntax etc.</dd>
	 *  <dt>protocol</dt>
	 *  <dd>Communication protocol to use (tcp, unix etc.)</dd>
	 *  <dt>hostspec</dt>
	 *  <dd>Host specification (hostname[:port])</dd>
	 *  <dt>database</dt>
	 *  <dd>Database to use on the DBMS server</dd>
	 *  <dt>username</dt>
	 *  <dd>User name for login</dd>
	 *  <dt>password</dt>
	 *  <dd>Password for login</dd>
	 * </dl>
	 * </p>
	 *
	 * <p>
	 * The format of the supplied DSN is in its fullest form:
	 * <ul>
	 *  <li>phptype(dbsyntax)://username:password@protocol+hostspec/database</li>
	 * </ul>
	 * Most variations are allowed:
	 * <ul>
	 *  <li>phptype://username:password@protocol+hostspec/database</li>
	 *  <li>phptype://username:password@hostspec/database</li>
	 *  <li>phptype://username:password@hostspec</li>
	 *  <li>phptype://hostspec/database</li>
	 *  <li>phptype://hostspec</li>
	 *  <li>phptype(dbsyntax)</li>
	 *  <li>phptype</li>
	 * </ul>
	 * </p>
	 *
	 * @return bool FALSE is returned on error
	 */
	function parseDSN($dsn) {
		$parsed = array(
			'phptype'  => false,
			'dbsyntax' => false,
			'protocol' => false,
			'hostspec' => false,
			'database' => false,
			'username' => false,
			'password' => false
		);
		if (preg_match('|^([^:]+)://|', $dsn, &$arr)) {
			$dbtype = $arr[1];
			$dsn = preg_replace('|^[^:]+://|', '', $dsn);
			// match "phptype(dbsyntax)"
			if (preg_match('|^([^\(]+)\((.+)\)$|', $dbtype, &$arr)) {
				$parsed['phptype'] = $arr[1];
				$parsed['dbsyntax'] = $arr[2];
			} else {
				$parsed['phptype'] = $dbtype;
			}
		} else {
			// match "phptype(dbsyntax)"
			if (preg_match('|^([^\(]+)\((.+)\)$|', $dsn, &$arr)) {
				$parsed['phptype'] = $arr[1];
				$parsed['dbsyntax'] = $arr[2];
			} else {
				$parsed['phptype'] = $dsn;
			}
			return $parsed;
		}

		if (preg_match('|^(.*)/([^/]+)/?$|', $dsn, &$arr)) {
			$parsed['database'] = $arr[2];
			$dsn = $arr[1];
		}

		if (preg_match('|^([^:]+):([^@]+)@?(.*)$|', $dsn, &$arr)) {
			$parsed['username'] = $arr[1];
			$parsed['password'] = $arr[2];
			$dsn = $arr[3];
		} elseif (preg_match('|^([^:]+)@(.*)$|', $dsn, &$arr)) {
			$parsed['username'] = $arr[1];
			$dsn = $arr[3];
		}

		if (preg_match('|^([^\+]+)\+(.*)$|', $dsn, &$arr)) {
			$parsed['protocol'] = $arr[1];
			$dsn = $arr[2];
		}

		if (!$parsed['database'])
			$dsn = preg_replace('|/+$|', '', $dsn);

		$parsed['hostspec'] = $dsn;

		if (!$parsed['dbsyntax']) {
			$parsed['dbsyntax'] = $parsed['phptype'];
		}

		return $parsed;
	}

    // }}}
}

/**
 * This class implements a wrapper for a DB result set.
 * A new instance of this class will be returned by the DB implementation
 * after processing a query that returns data.
 */
class DB_result {
    // {{{ properties

    var $dbh;
    var $result;

    // }}}
    // {{{ DB_result()

    /**
	 * DB_result constructor.
	 * @param   $dbh    DB object reference
	 * @param   $result result resource id
	 */
    function DB_result(&$dbh, $result) {
		$this->dbh = &$dbh;
		$this->result = $result;
    }

    // }}}
    // {{{ fetchRow()

	/**
	 * Fetch and return a row of data.
	 * @return  array   a row of data, or false on error
	 */
    function fetchRow($getmode = DB_GETMODE_DEFAULT) {
		return $this->dbh->fetchRow($this->result, $getmode);
    }

    // }}}
    // {{{ fetchInto()

    /**
	 * Fetch a row of data into an existing array.
	 *
	 * @param   $arr    reference to data array
	 * @return  int     error code
	 */
    function fetchInto(&$arr, $getmode = DB_GETMODE_DEFAULT) {
		return $this->dbh->fetchInto($this->result, &$arr, $getmode);
    }

    // }}}
	// {{{ numCols()

	/**
	 * Get the the number of columns in a result set.
	 *
	 * @return int the number of columns, or a DB error code
	 */
	function numCols() {
		return $this->dbh->numCols($this->result);
	}

	// }}}
    // {{{ free()

    /**
	 * Frees the resources allocated for this result set.
	 * @return  int     error code
	 */
    function free() {
		$err = $this->dbh->freeResult($this->result);
		if (DB::isError($err)) {
			return $err;
		}
		$this->dbh = $this->result = false;
		return true;
    }

    // }}}
}

// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
?>
