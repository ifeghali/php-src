--TEST--
Phar front controller alternate index file zip-based
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
<?php if (!extension_loaded("zip")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller7.phar.php/
REQUEST_URI=/frontcontroller7.phar.php/
--FILE_EXTERNAL--
frontcontroller2.phar.zip
--EXPECTHEADERS--
Status: 301 Moved Permanently
Location: /frontcontroller7.phar.php/a.php
--EXPECT--
