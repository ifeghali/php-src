--TEST--
strripos() offset integer overflow
--FILE--
<?php

var_dump(strripos("t", "t", PHP_INT_MAX+1));
var_dump(strripos("tttt", "tt", PHP_INT_MAX+1));
var_dump(strripos(100, 101, PHP_INT_MAX+1));
var_dump(strripos(1024, 1024, PHP_INT_MAX+1));
var_dump(strripos(array(), array(), PHP_INT_MAX+1));
var_dump(strripos(1024, 1024, -PHP_INT_MAX));
var_dump(strripos(1024, "te", -PHP_INT_MAX));
var_dump(strripos(1024, 1024, -PHP_INT_MAX-1));
var_dump(strripos(1024, "te", -PHP_INT_MAX-1));

echo "Done\n";
?>
--EXPECTF--	
bool(false)
bool(false)
bool(false)
bool(false)

Warning: strripos() expects parameter 1 to be string (Unicode or binary), array given in %s on line %d
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
Done
--UEXPECTF--
bool(false)
bool(false)
bool(false)
bool(false)

Warning: strripos() expects parameter 1 to be string (Unicode or binary), array given in %s on line %d
bool(false)
bool(false)
bool(false)
bool(false)
bool(false)
Done