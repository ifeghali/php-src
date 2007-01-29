--TEST--
Phar: require hash
--SKIPIF--
<?php if (!extension_loaded("phar")) print "skip"; ?>
--INI--
phar.require_hash=1
--FILE--
<?php

$pharconfig = 0;

require_once 'phar_oo_test.inc';

try {
Phar::loadPhar($fname);
} catch (Exception $e) {
echo $e->getMessage();
}

?>
===DONE===
--CLEAN--
<?php 
unlink(dirname(__FILE__) . '/phar_oo_test.phar.php');
__halt_compiler();
?>
--EXPECTF--

phar "%sphar_oo_test.phar.php" does not have a signature===DONE===