--TEST--
Phar::setStub()/stopBuffering()
--SKIPIF--
<?php if (!extension_loaded("phar")) print "skip"; ?>
--INI--
phar.require_hash=0
phar.readonly=0
--FILE--
<?php
$p = new Phar(dirname(__FILE__) . '/brandnewphar.phar', 0, 'brandnewphar.phar');
$p['file1.txt'] = 'hi';
$p->stopBuffering();
var_dump($p->getStub());
$p->setStub("<?php
function __autoload(\$class)
{
    include 'phar://' . str_replace('_', '/', \$class);
}
Phar::mapPhar('brandnewphar.phar');
include 'phar://brandnewphar.phar/startup.php';
__HALT_COMPILER();
?>");
var_dump($p->getStub());
?>
===DONE===
--CLEAN--
<?php 
unlink(dirname(__FILE__) . '/brandnewphar.phar');
?>
--EXPECT--
string(29) "<?php __HALT_COMPILER(); ?>
"
string(200) "<?php
function __autoload($class)
{
    include 'phar://' . str_replace('_', '/', $class);
}
Phar::mapPhar('brandnewphar.phar');
include 'phar://brandnewphar.phar/startup.php';
__HALT_COMPILER(); ?>
"
===DONE===