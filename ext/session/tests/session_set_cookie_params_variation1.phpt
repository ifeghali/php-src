--TEST--
Test session_set_cookie_params() function : variation
--INI--
session.cookie_lifetime=3600
--SKIPIF--
<?php include('skipif.inc'); ?>
--FILE--
<?php

ob_start();

/* 
 * Prototype : void session_set_cookie_params(int $lifetime [, string $path [, string $domain [, bool $secure [, bool $httponly]]]])
 * Description : Set the session cookie parameters
 * Source code : ext/session/session.c 
 */

echo "*** Testing session_set_cookie_params() : variation ***\n";

var_dump(ini_get("session.cookie_lifetime"));
var_dump(session_set_cookie_params(3600));
var_dump(ini_get("session.cookie_lifetime"));
var_dump(session_start());
var_dump(ini_get("session.cookie_lifetime"));
var_dump(session_set_cookie_params(1800));
var_dump(ini_get("session.cookie_lifetime"));
var_dump(session_destroy());
var_dump(ini_get("session.cookie_lifetime"));
var_dump(session_set_cookie_params(1234567890));
var_dump(ini_get("session.cookie_lifetime"));

echo "Done";
ob_end_flush();
?>
--EXPECTF--
*** Testing session_set_cookie_params() : variation ***
string(4) "3600"
NULL
string(4) "3600"
bool(true)
string(4) "3600"
NULL
string(4) "1800"
bool(true)
string(4) "1800"
NULL
string(10) "1234567890"
Done
--UEXPECTF--
*** Testing session_set_cookie_params() : variation ***
unicode(4) "3600"
NULL
unicode(4) "3600"
bool(true)
unicode(4) "3600"
NULL
unicode(4) "1800"
bool(true)
unicode(4) "1800"
NULL
unicode(10) "1234567890"
Done
