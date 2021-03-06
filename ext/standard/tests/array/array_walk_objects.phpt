--TEST--
array_walk() and objects
--FILE--
<?php

function walk($key, $value) { 
	var_dump($value, $key); 
}

class test {
	private $var_pri = "test_private";
	protected $var_pro = "test_protected";
	public $var_pub = "test_public";
}

$stdclass = new stdclass;
$stdclass->foo = "foo";
$stdclass->bar = "bar";
array_walk($stdclass, "walk");

$t = new test;
array_walk($t, "walk");

$var = array();
array_walk($var, "walk");
$var = "";
array_walk($var, "walk");

echo "Done\n";
?>
--EXPECTF--	
Warning: array_walk() expects parameter 1 to be array, object given in %s on line %d

Warning: array_walk() expects parameter 1 to be array, object given in %s on line %d

Warning: array_walk() expects parameter 1 to be array, Unicode string given in %s on line %d
Done
