--TEST--
Phar: fopen a .phar for writing (existing file) tar-based
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
--INI--
phar.readonly=1
phar.require_hash=0
--FILE--
<?php
include dirname(__FILE__) . '/tarmaker.php.inc';
$fname = dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.tar.php';
$pname = 'phar://' . $fname;
$a = new tarmaker($fname, 'none');
$a->init();
$a->addFile('.phar/stub.php', "<?php __HALT_COMPILER(); ?>");

$files = array();
$files['a.php'] = '<?php echo "This is a\n"; ?>';
$files['b.php'] = '<?php echo "This is b\n"; ?>';
$files['b/c.php'] = '<?php echo "This is b/c\n"; ?>';
foreach ($files as $n => $file) {
$a->addFile($n, $file);
}
$a->close();

$fp = fopen($pname . '/b/c.php', 'wb');
fwrite($fp, 'extra');
fclose($fp);
include $pname . '/b/c.php';
?>
===DONE===
--CLEAN--
<?php unlink(dirname(__FILE__) . '/' . basename(__FILE__, '.clean.php') . '.phar.tar.php'); ?>
--EXPECTF--

Warning: fopen(phar://%sopen_for_write_existing_c.phar.tar.php/b/c.php): failed to open stream: phar error: write operations disabled by INI setting in %sopen_for_write_existing_c.php on line %d

Warning: fwrite(): supplied argument is not a valid stream resource in %sopen_for_write_existing_c.php on line %d

Warning: fclose(): supplied argument is not a valid stream resource in %sopen_for_write_existing_c.php on line %d
This is b/c
===DONE===