--TEST--
declare script encoding (SJIS)
--SKIPIF--
<?php if (!unicode_semantics()) die('skip unicode.semantics=off'); ?>
--INI--
unicode.output_encoding=CP866
--FILE--
<?php
declare(encoding="SJIS");

function ���u����() {
  echo "���u���� - ok\n";
}

���u����();
?>
--EXPECT--
��� - ok
