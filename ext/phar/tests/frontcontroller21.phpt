--TEST--
Phar front controller $_SERVER munging success
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller21.php/index.php
REQUEST_URI=/frontcontroller21.php/index.php?test=hi
--FILE_EXTERNAL--
frontcontroller12.phar
--EXPECTHEADERS--
Content-type: text/html
--EXPECTF--
string(10) "/index.php"
string(%d) "phar://%sfrontcontroller21.php/index.php"
string(%d) "phar://%sfrontcontroller21.php/index.php"
string(18) "/index.php?test=hi"
string(32) "/frontcontroller21.php/index.php"
string(32) "/frontcontroller21.php/index.php"
string(%d) "%sfrontcontroller21.php"
string(40) "/frontcontroller21.php/index.php?test=hi"