--TEST--
ZE2 Late Static Binding ensuring implementing 'static' is not allowed
--FILE--
<?php

class Foo implements static {
}

?>
==DONE==
--EXPECTF--
Parse error: syntax error, unexpected T_STATIC, expecting T_STRING or T_PAAMAYIM_NEKUDOTAYIM in %s on line %d
