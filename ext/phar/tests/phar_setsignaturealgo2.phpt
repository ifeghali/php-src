--TEST--
Phar::setSupportedSignatures() with hash
--SKIPIF--
<?php if (!extension_loaded("phar")) print "skip"; ?>
<?php if (!extension_loaded("hash")) print "skip extension hash required"; ?>
--INI--
phar.require_hash=0
phar.readonly=0
--FILE--
<?php
$p = new Phar(dirname(__FILE__) . '/brandnewphar.phar', 0, 'brandnewphar.phar');
$p['file1.txt'] = 'hi';
var_dump($p->getSignature());
$p->setSignatureAlgorithm(Phar::MD5);
var_dump($p->getSignature());
$p->setSignatureAlgorithm(Phar::SHA1);
var_dump($p->getSignature());
try {
$p->setSignatureAlgorithm(Phar::SHA256);
var_dump($p->getSignature());
} catch (Exception $e) {
echo $e->getMessage();
}
try {
$p->setSignatureAlgorithm(Phar::SHA512);
var_dump($p->getSignature());
} catch (Exception $e) {
echo $e->getMessage();
}
try {
$p->setSignatureAlgorithm(Phar::PGP);
var_dump($p->getSignature());
} catch (Exception $e) {
echo $e->getMessage();
}
?>
===DONE===
--CLEAN--
<?php 
unlink(dirname(__FILE__) . '/brandnewphar.phar');
?>
--EXPECTF--
array(2) {
  ["hash"]=>
  string(%d) "%s"
  ["hash_type"]=>
  string(5) "SHA-1"
}
array(2) {
  ["hash"]=>
  string(%d) "%s"
  ["hash_type"]=>
  string(3) "MD5"
}
array(2) {
  ["hash"]=>
  string(%d) "%s"
  ["hash_type"]=>
  string(5) "SHA-1"
}
array(2) {
  ["hash"]=>
  string(%d) "%s"
  ["hash_type"]=>
  string(7) "SHA-256"
}
array(2) {
  ["hash"]=>
  string(%d) "%s"
  ["hash_type"]=>
  string(7) "SHA-512"
}
array(2) {
  ["hash"]=>
  string(%d) "%s"
  ["hash_type"]=>
  string(5) "SHA-1"
}
===DONE===
