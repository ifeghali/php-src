--TEST--
Phar front controller alternate index file
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller7.php/
REQUEST_URI=/frontcontroller7.php/
--FILE_EXTERNAL--
frontcontroller2.phar
--EXPECTHEADERS--
Status: 301 Moved Permanently
Location: /frontcontroller7.php/a.php
--EXPECT--
