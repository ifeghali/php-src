--TEST--
Phar::mapPhar truncated manifest/improper params
--SKIPIF--
<?php if (!extension_loaded("phar")) print "skip"; ?>
--FILE--
<?php
try {
Phar::mapPhar(5, 5);
Phar::mapPhar(5, 'hio');
Phar::mapPhar(5, 'hio', 'hi');

Phar::mapPhar();
Phar::mapPhar(5);
} catch (Exception $e) {
	echo $e->getMessage();
}
__HALT_COMPILER(); ?>
--EXPECTF--
Warning: Phar::mapPhar() expects at most 1 parameter, 2 given in %s002.php on line %d

Warning: Phar::mapPhar() expects at most 1 parameter, 2 given in %s002.php on line %d

Warning: Phar::mapPhar() expects at most 1 parameter, 3 given in %s002.php on line %d
internal corruption of phar "%s002.php" (truncated manifest)