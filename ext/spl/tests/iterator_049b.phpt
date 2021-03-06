--TEST--
SPL: ArrayIterator with NULL key
--SKIPIF--
<?php if (!extension_loaded("spl")) print "skip"; ?>
--FILE--
<?php

$ar = new ArrayIterator(array(
	NULL=>1, 
	"\0"=>2,
	"\0\0"=>3,
	"\0\0\0"=>4,
	"\0*"=>5,
	"\0*\0"=>6,
	));
@var_dump($ar);
var_dump($ar->getArrayCopy());

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
object(ArrayIterator)#%d (1) {
  [u"storage":u"ArrayIterator":private]=>
  array(6) {
    [u""]=>
    int(1)
    [u" "]=>
    int(2)
    [u"  "]=>
    int(3)
    [u"   "]=>
    int(4)
    [u" *"]=>
    int(5)
    [u" * "]=>
    int(6)
  }
}
array(6) {
  [u""]=>
  int(1)
  [u" "]=>
  int(2)
  [u"  "]=>
  int(3)
  [u"   "]=>
  int(4)
  [u" *"]=>
  int(5)
  [u" * "]=>
  int(6)
}
===DONE===
