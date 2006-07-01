--TEST--
catch assert() errors
--FILE--
<?php
function handler($errno, $errstr) {
	echo "in handler()\n";
	assert(E_RECOVERABLE_ERROR === $errno);
	var_dump($errstr);
}

set_error_handler('handler', E_RECOVERABLE_ERROR);

assert(1);
assert('1');
assert('$a');

assert('aa=sd+as+safsafasfasafsaf');

assert('0');

assert_options(ASSERT_BAIL, 1);
assert('aa=sd+as+safsafasfasafsaf');

echo "done\n";

?>
--EXPECTF--
Notice: Undefined variable: a in %sassert02.php(12) : assert code on line 1

Warning: assert(): Assertion "$a" failed in %sassert02.php on line 12

Parse error: syntax error, unexpected '=' in %sassert02.php(14) : assert code on line 1
in handler()
string(61) "assert(): Failure evaluating code: 
aa=sd+as+safsafasfasafsaf"

Warning: assert(): Assertion "0" failed in %sassert02.php on line 16

Parse error: syntax error, unexpected '=' in %sassert02.php(19) : assert code on line 1
in handler()
string(61) "assert(): Failure evaluating code: 
aa=sd+as+safsafasfasafsaf"
--EXPECTUF--
Notice: Undefined variable: a in %sassert02.php(12) : assert code on line 1

Warning: assert(): Assertion "$a" failed in %sassert02.php on line 12

Parse error: syntax error, unexpected '=' in %sassert02.php(14) : assert code on line 1
in handler()
unicode(61) "assert(): Failure evaluating code: 
aa=sd+as+safsafasfasafsaf"

Warning: assert(): Assertion "0" failed in %sassert02.php on line 16

Parse error: syntax error, unexpected '=' in %sassert02.php(19) : assert code on line 1
in handler()
unicode(61) "assert(): Failure evaluating code: 
aa=sd+as+safsafasfasafsaf"
