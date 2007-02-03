--TEST--
Phar: opendir test - no dir specified at all
--SKIPIF--
<?php if (!extension_loaded("phar")) print "skip"; ?>
--INI--
phar.require_hash=0
--FILE--
<?php
$fname = dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.php';
$pname = 'phar://' . $fname;
$file = "<?php
Phar::mapPhar('hio');
__HALT_COMPILER(); ?>";

$files = array();
$files['a'] = 'abc';
include 'phar_test.inc';

include $fname;
$dir = opendir('phar://hio');
?>
--CLEAN--
<?php unlink(dirname(__FILE__) . '/' . basename(__FILE__, '.clean.php') . '.phar.php'); ?>
--EXPECTF--
Warning: opendir(phar://hio): failed to open dir: phar error: no directory in "phar://hio", must have at least phar://hio/ for root directory (always use full path to a new phar) in %s on line %d
