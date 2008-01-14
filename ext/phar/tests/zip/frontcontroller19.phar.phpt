--TEST--
Phar front controller $_SERVER munging failure 2 zip-based
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
<?php if (!extension_loaded("zip")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller19.phar.php/
REQUEST_URI=/frontcontroller19.phar.php/
--FILE_EXTERNAL--
frontcontroller10.phar.zip
--EXPECTF--
Fatal error: Uncaught exception 'UnexpectedValueException' with message 'Too many values passed to Phar::mungServer(), expecting an array of any of these strings: PHP_SELF, REQUEST_URI, SCRIPT_FILENAME, SCRIPT_NAME' in %sfrontcontroller19.phar.php:2
Stack trace:
#0 %sfrontcontroller19.phar.php(2): Phar::mungServer(Array)
#1 {main}
  thrown in %sfrontcontroller19.phar.php on line 2
