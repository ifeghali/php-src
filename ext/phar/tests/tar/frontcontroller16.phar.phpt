--TEST--
Phar front controller mime type override, Phar::PHP tar-based
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller16.phar.php/a.phps
REQUEST_URI=/frontcontroller16.phar.php/a.phps
--FILE_EXTERNAL--
frontcontroller8.phar.tar
--EXPECTHEADERS--
Content-type: text/html
--EXPECT--
hio1

