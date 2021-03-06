--TEST--
SPL: ArrayObject and \0
--FILE--
<?php

try
{
	$foo = new ArrayObject();
	$foo->offsetSet("\0", "Foo");
}
catch (Exception $e)
{
	var_dump($e->getMessage());
}

var_dump($foo);

try
{
	$foo = new ArrayObject();
	$data = explode("=", "=Foo");
	$foo->offsetSet($data[0], $data[1]);
}
catch (Exception $e)
{
	var_dump($e->getMessage());
}

var_dump($foo);

?>
===DONE===
--EXPECTF--
object(ArrayObject)#1 (1) {
  [u"storage":u"ArrayObject":private]=>
  array(1) {
    [u" "]=>
    unicode(3) "Foo"
  }
}
object(ArrayObject)#2 (1) {
  [u"storage":u"ArrayObject":private]=>
  array(1) {
    [u""]=>
    unicode(3) "Foo"
  }
}
===DONE===
