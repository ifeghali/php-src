--TEST--
Unicode identifiers normalization (indirect function call)
--INI--
unicode.semantics=on
--FILE--
<?php
declare(encoding = "ISO-8859-1");

function �() {
  echo "ok\n";
}

$f1 = "\u212B";
$f1();
$f2 = "\u00C5";
$f2();
?>
--EXPECT--
ok
ok
