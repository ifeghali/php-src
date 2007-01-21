--TEST--
Phar::mapPhar filesize mismatch
--SKIPIF--
<?php if (!extension_loaded("phar")) print "skip";?>
--INI--
phar.require_hash=0
--FILE--
<?php
$file = "<?php
Phar::mapPhar('hio');
__HALT_COMPILER(); ?>";
// wrong crc32

$files = array();
$files['a'] = 'a';
$manifest = '';
foreach($files as $name => $cont) {
	$len = strlen($cont);
	$manifest .= pack('V', strlen($name)) . $name . pack('VVVVVV', $len, time(), $len, crc32($cont.'X'), 0x00000000, 0);
}
$alias = 'hio';
$manifest = pack('VnVV', count($files), 0x0900, 0x00000000, strlen($alias)) . $alias . $manifest;
$file .= pack('V', strlen($manifest)) . $manifest;
foreach($files as $cont)
{
	$file .= $cont;
}

file_put_contents(dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.php', $file);
include dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.php';
echo file_get_contents('phar://hio/a');
?>
--CLEAN--
<?php unlink(dirname(__FILE__) . '/' . basename(__FILE__, '.clean.php') . '.phar.php'); ?>
--EXPECTF--
Warning: file_get_contents(phar://hio/a): failed to open stream: phar error: internal corruption of phar "%s" (crc32 mismatch on file "a") in %s on line %d