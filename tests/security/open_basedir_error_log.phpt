--TEST--
Test open_basedir configuration
--INI--
open_basedir=.
--FILE--
<?php
require_once "open_basedir.inc";
test_open_basedir_before("error_log");
$directory = dirname(__FILE__);

var_dump(ini_set("error_log", $directory."/test/bad/bad.txt"));
var_dump(ini_set("error_log", $directory."/test/bad.txt"));
var_dump(ini_set("error_log", $directory."/bad.txt"));
var_dump(ini_set("error_log", $directory."/test/ok/ok.txt"));
var_dump(ini_set("error_log", $directory."/test/ok/ok.txt"));

test_open_basedir_after("error_log");
?>
--CLEAN--
<?php
require_once "open_basedir.inc";
delete_directories();
?>
--EXPECTF--
*** Testing open_basedir configuration [error_log] ***
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)

Warning: ini_set(): open_basedir restriction in effect. File(%s/test/bad/bad.txt) is not within the allowed path(s): (.) in %s on line %d
bool(false)

Warning: ini_set(): open_basedir restriction in effect. File(%s/test/bad.txt) is not within the allowed path(s): (.) in %s on line %d
bool(false)

Warning: ini_set(): open_basedir restriction in effect. File(%s/bad.txt) is not within the allowed path(s): (.) in %s on line %d
bool(false)
string(0) ""
string(%d) "%s/test/ok/ok.txt"
*** Finished testing open_basedir configuration [error_log] ***
--UEXPECTF--
*** Testing open_basedir configuration [error_log] ***
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)

Warning: ini_set(): open_basedir restriction in effect. File(%s/test/bad/bad.txt) is not within the allowed path(s): (.) in %s on line %d
bool(false)

Warning: ini_set(): open_basedir restriction in effect. File(%s/test/bad.txt) is not within the allowed path(s): (.) in %s on line %d
bool(false)

Warning: ini_set(): open_basedir restriction in effect. File(%s/bad.txt) is not within the allowed path(s): (.) in %s on line %d
bool(false)
unicode(0) ""
unicode(%d) "%s/test/ok/ok.txt"
*** Finished testing open_basedir configuration [error_log] ***
