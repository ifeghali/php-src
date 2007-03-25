--TEST--
Phar::setSupportedSignatures()
--SKIPIF--
<?php if (!extension_loaded("phar")) print "skip"; ?>
<?php if ( extension_loaded("hash")) print "skip extension hash conflicts"; ?>
--INI--
phar.require_hash=0
phar.readonly=0
--FILE--
<?php
var_dump(Phar::getSupportedSignatures());
?>
===DONE===
?>
--EXPECT--
array(2) {
  [0]=>
  string(3) "MD5"
  [1]=>
  string(5) "SHA-1"
}
===DONE===
