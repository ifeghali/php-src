--TEST--
Phar front controller mime type override, other zip-based
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
<?php if (!extension_loaded("zip")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller14.phar.php/a.jpg
REQUEST_URI=/frontcontroller14.phar.php/a.jpg
--FILE_EXTERNAL--
frontcontroller8.phar.zip
--EXPECTHEADERS--
Content-type: foo/bar
Content-length: 4
--EXPECT--
hio2
