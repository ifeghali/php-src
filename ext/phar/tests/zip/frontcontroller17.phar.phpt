--TEST--
Phar front controller mime type unknown zip-based
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
<?php if (!extension_loaded("zip")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller17.phar.php/fronk.gronk
REQUEST_URI=/frontcontroller17.phar.php/fronk.gronk
--FILE_EXTERNAL--
frontcontroller8.phar.zip
--EXPECTHEADERS--
Content-type: application/octet-stream
Content-length: 4
--EXPECT--
hio3
