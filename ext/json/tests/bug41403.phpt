--TEST--
Bug #41403 (json_decode cannot decode floats if localeconv decimal_point is not '.')
--SKIPIF--
<?php
if (setlocale(LC_NUMERIC, "de_DE") === false) {
	die("skip no de_DE locale");
}
?>
--INI--
precision=14
--FILE--
<?php

setlocale(LC_NUMERIC, 'de_DE');
var_dump(json_decode('[2.1]'));
var_dump(json_decode('[0.15]'));
var_dump(json_decode('[123.13452345]'));
var_dump(json_decode('[123,13452345]'));

echo "Done\n";
?>
--EXPECTF--	
array(1) {
  [0]=>
  float(2,1)
}
array(1) {
  [0]=>
  float(0,15)
}
array(1) {
  [0]=>
  float(123,13452345)
}
array(2) {
  [0]=>
  int(123)
  [1]=>
  int(13452345)
}
Done
--UEXPECTF--
Strict Standards: setlocale(): deprecated in Unicode mode, please use ICU locale functions in %s on line %d
array(1) {
  [0]=>
  float(2,1)
}
array(1) {
  [0]=>
  float(0,15)
}
array(1) {
  [0]=>
  float(123,13452345)
}
array(2) {
  [0]=>
  int(123)
  [1]=>
  int(13452345)
}
Done