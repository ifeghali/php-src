--TEST--
fribidi_log2vis
--SKIPIF--
<?php if (!extension_loaded("fribidi")) print "skip"; ?>
--POST--
--GET--
--FILE--
<?php
	error_reporting (E_ALL ^ E_NOTICE);
	$a =  fribidi_log2vis("THE dog 123 IS THE biggest", FRIBIDI_AUTO, FRIBIDI_CHARSET_CAP_RTL);
	$b =  fribidi_log2vis("��� 198 foo ��� BAR 12", FRIBIDI_RTL, FRIBIDI_CHARSET_8859_8);
	$c =  fribidi_log2vis("Fri ���� ���� bla 12% bla", FRIBIDI_AUTO, FRIBIDI_CHARSET_CP1255);

	$d = array($a, $b, $c);
	var_dump($d);
?>
--EXPECT--
array(3) {
  [0]=>
  string(26) "biggest EHT SI dog 123 EHT"
  [1]=>
  string(22) "BAR 12 ��� foo 198 ���"
  [2]=>
  string(25) "Fri ���� ���� bla 12% bla"
}
