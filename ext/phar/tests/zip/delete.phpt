--TEST--
Phar: delete test, zip-based phar
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
<?php if (!extension_loaded("zip")) die("skip"); ?>
--INI--
phar.readonly=0
phar.require_hash=0
--FILE--
<?php
include dirname(__FILE__) . '/tarmaker.php.inc';
$fname = dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.php';
$pname = 'phar://' . $fname;
$file = "<?php
Phar::mapPhar('hio');
__HALT_COMPILER(); ?>";

$a = new tarmaker($fname, 'none');
$a->init();
$a->addFile('a', 'a');
$a->addFile('.phar/stub.php', $file);
$a->close();

$phar = new Phar($fname);

echo file_get_contents($pname . '/a') . "\n";
$phar->delete('a');
echo file_get_contents($pname . '/a') . "\n";
?>
--CLEAN--
<?php unlink(dirname(__FILE__) . '/' . basename(__FILE__, '.clean.php') . '.phar.php'); ?>
--EXPECTF--
a

Warning: file_get_contents(phar://%sdelete.phar.php/a): failed to open stream: phar error: "a" is not a file in phar "%sdelete.phar.php" in %sdelete.php on line %d