--TEST--
Script encoding autodetection (UTF-16LE)
--INI--
unicode.semantics=on
unicode.output_encoding=CP866
--FILE--
<?php
include(dirname(__FILE__)."/autodetect-UTF16LE.inc");
?>
--EXPECT--
��� - ok
