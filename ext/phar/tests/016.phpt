--TEST--
Phar::mapPhar invalid file (gzipped file length is too short)
--SKIPIF--
<?php if (!extension_loaded("phar")) print "skip";
if (!extension_loaded("zlib")) print "skip zlib not present"; ?>
--INI--
phar.require_hash=0
--FILE--
<?php
$file = "<?php
Phar::mapPhar('hio');
__HALT_COMPILER(); ?>";
// file length is too short

$files = array();
$files['a'] = array('a', chr(75)/*. chr(4) . chr(0): 'a' gzdeflated */, 0x00000001);
$files['b'] = array('a', chr(75)/*. chr(4) . chr(0): 'a' gzdeflated */, 0x00000001);
$files['c'] = array('*', '*',                                           0x00000000);
$files['d'] = array('a', chr(75)/*. chr(4) . chr(0): 'a' gzdeflated */, 0x00000001);
$manifest = '';
foreach($files as $name => $cont) {
	$ulen = strlen($cont[0]);
	$clen = strlen($cont[1]);
	$manifest .= pack('V', strlen($name)) . $name 
	          . pack('VVVVV', $ulen, time(), $clen, crc32($cont[0]), $cont[2]);
}
$alias = 'hio';
$manifest = pack('VnVV', count($files), 0x0900, 0x00000001, strlen($alias)) . $alias . $manifest;
$file .= pack('V', strlen($manifest)) . $manifest;
foreach($files as $cont)
{
	$file .= $cont[1];
}

file_put_contents(dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.php', $file);
include dirname(__FILE__) . '/' . basename(__FILE__, '.php') . '.phar.php';
var_dump(file_get_contents('phar://hio/a'));
var_dump(file_get_contents('phar://hio/b'));
var_dump(file_get_contents('phar://hio/c'));
var_dump(file_get_contents('phar://hio/d'));
?>
--CLEAN--
<?php unlink(dirname(__FILE__) . '/' . basename(__FILE__, '.clean.php') . '.phar.php'); ?>
--EXPECTF--
Warning: file_get_contents(phar://hio/a): failed to open stream: phar error: internal corruption of phar "%s" (actual filesize mismatch on file "a") in %s on line %d
bool(false)

Warning: file_get_contents(phar://hio/b): failed to open stream: phar error: internal corruption of phar "%s" (actual filesize mismatch on file "b") in %s on line %d
bool(false)
string(1) "*"

Warning: file_get_contents(phar://hio/d): failed to open stream: phar error: internal corruption of phar "%s" (actual filesize mismatch on file "d") in %s on line %d
bool(false)
