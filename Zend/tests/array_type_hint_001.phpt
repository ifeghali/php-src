--TEST--
Array type hint
--FILE--
<?php
function foo(array $a) {
	echo count($a)."\n";
}

foo(array(1,2,3));
foo(123);
?>
--EXPECTF--
3

Fatal error: Argument 1 must be an array in %sarray_type_hint_001.php on line 2