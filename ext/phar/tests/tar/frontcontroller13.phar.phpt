--TEST--
Phar front controller mime type not string/int tar-based
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller13.phar.php/a.php
REQUEST_URI=/frontcontroller13.phar.php/a.php
--FILE_EXTERNAL--
frontcontroller7.phar.tar
--EXPECTHEADERS--
Content-type: text/html
--EXPECTF--
Fatal error: Uncaught exception 'UnexpectedValueException' with message 'Unknown mime type specifier used (not a string or int), only Phar::PHP, Phar::PHPS and a mime type string are allowed' in %sfrontcontroller13.phar.php:2
Stack trace:
#0 %sfrontcontroller13.phar.php(2): Phar::webPhar('whatever', 'index.php', '', Array)
#1 {main}
  thrown in %sfrontcontroller13.phar.php on line 2