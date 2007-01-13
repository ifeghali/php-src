--TEST--
Phar::mapPhar too many manifest entries
--SKIPIF--
<?php if (!extension_loaded("phar")) print "skip";?>
--INI--
phar.require_hash=0
--FILE--
<?php
$file = "<?php
Phar::mapPhar('hio');
__HALT_COMPILER(); ?>";
$file .= pack('VVnVV', 500, 500, 0x0900, 0x00000000, 0) . str_repeat('A', 500);
file_put_contents(dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.php', $file);
include dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.php';
?>
--CLEAN--
<?php unlink(dirname(__FILE__) . '/' . basename(__FILE__, '.clean.php') . '.phar.php'); ?>
--EXPECTF--
%satal error: Phar::mapPhar(): internal corruption of phar "%s009.phar.php" (too many manifest entries for size of manifest) in %s on line %d
