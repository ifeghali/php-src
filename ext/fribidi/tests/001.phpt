--TEST--
fribidi_log2vis
--SKIPIF--
<?php if (!extension_loaded("fribidi")) print "skip"; ?>
--POST--
--GET--
--FILE--
<?php
	error_reporting (E_ALL ^ E_NOTICE);
	echo fribidi_log2vis("THE dog 123 IS THE biggest", FRIBIDI_AUTO, FRIBIDI_CHARSET_CAP_RTL)."\n";
	echo fribidi_log2vis("��� 198 foo ��� BAR 12", FRIBIDI_RTL, FRIBIDI_CHARSET_8859_8)."\n";
	echo fribidi_log2vis("Fri ���� ���� bla 12% bla", FRIBIDI_AUTO, FRIBIDI_CHARSET_CP1255);
?>
--EXPECT--
biggest EHT SI dog 123 EHT
BAR 12 ��� foo 198 ���
Fri ���� ���� bla 12% bla
