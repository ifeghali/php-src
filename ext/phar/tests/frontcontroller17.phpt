--TEST--
Phar front controller mime type unknown
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller17.php/fronk.gronk
REQUEST_URI=/frontcontroller17.php/fronk.gronk
--FILE_EXTERNAL--
frontcontroller8.phar
--EXPECTHEADERS--
Content-type: application/octet-stream
Content-length: 4
--EXPECT--
hio3
