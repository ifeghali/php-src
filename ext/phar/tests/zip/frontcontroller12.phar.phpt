--TEST--
Phar front controller mime type unknown int zip-based
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
<?php if (!extension_loaded("zip")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller12.phar.php/a.php
REQUEST_URI=/frontcontroller12.phar.php/a.php
--FILE_EXTERNAL--
frontcontroller6.phar.zip
--EXPECTHEADERS--
Content-type: text/html
--EXPECTF--
Fatal error: Uncaught exception 'UnexpectedValueException' with message 'Unknown mime type specifier used, only Phar::PHP, Phar::PHPS and a mime type string are allowed' in %sfrontcontroller12.phar.php:2
Stack trace:
#0 %sfrontcontroller12.phar.php(2): Phar::webPhar('whatever', 'index.php', '', Array)
#1 {main}
  thrown in %sfrontcontroller12.phar.php on line 2