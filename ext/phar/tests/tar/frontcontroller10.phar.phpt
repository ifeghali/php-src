--TEST--
Phar front controller rewrite array invalid tar-based
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller10.phar.php/hi
REQUEST_URI=/frontcontroller10.phar.php/hi
--FILE_EXTERNAL--
frontcontroller4.phar.tar
--EXPECTHEADERS--
Content-type: text/html
--EXPECTF--
Fatal error: Uncaught exception 'UnexpectedValueException' with message 'phar rewrite value for "/hi" was not a string' in %sfrontcontroller10.phar.php/.phar/stub.php:2
Stack trace:
#0 %sfrontcontroller10.phar.php/.phar/stub.php(2): Phar::webPhar('whatever', 'index.php', '', Array, Array)
#1 {main}
  thrown in %sfrontcontroller10.phar.php/.phar/stub.php on line 2