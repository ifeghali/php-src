--TEST--
Phar front controller mime type extension is not a string zip-based
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
<?php if (!extension_loaded("zip")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller11.phar.php/a.php
REQUEST_URI=/frontcontroller11.phar.php/a.php
--FILE_EXTERNAL--
frontcontroller5.phar.zip
--EXPECTHEADERS--
Content-type: text/html
--EXPECTF--
Fatal error: Uncaught exception 'UnexpectedValueException' with message 'Key of MIME type overrides array must be a file extension, was "0"' in %sfrontcontroller11.phar.php:2
Stack trace:
#0 %sfrontcontroller11.phar.php(2): Phar::webPhar('whatever', 'index.php', '', Array)
#1 {main}
  thrown in %sfrontcontroller11.phar.php on line 2