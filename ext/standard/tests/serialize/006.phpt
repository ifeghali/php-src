--TEST--
serialize()/unserialize() with exotic letters
--INI--
unicode.script_encoding=ISO-8859-1
unicode.output_encoding=ISO-8859-1
--FILE--
<?php
	$������ = array('������' => '������');

	class �berK��li�� 
	{
		public $��������ber = '������';
	}
  
    $foo = new �berk��li��();
  
	var_dump(serialize($foo));
	var_dump(unserialize(serialize($foo)));
	var_dump(serialize($������));
	var_dump(unserialize(serialize($������)));
?>
--EXPECT--
string(83) "O:11:"�berK��li��":1:{S:11:"\e5\e4\f6\c5\c4\d6\fc\dcber";S:6:"\e5\e4\f6\c5\c4\d6";}"
object(�berK��li��)#2 (1) {
  ["��������ber"]=>
  string(6) "������"
}
string(56) "a:1:{S:6:"\e5\e4\f6\c5\c4\d6";S:6:"\e5\e4\f6\c5\c4\d6";}"
array(1) {
  ["������"]=>
  string(6) "������"
}
--UEXPECT--
unicode(131) "O:11:"\00dcberK\00f6\00f6li\00e4\00e5":1:{U:11:"\00e5\00e4\00f6\00c5\00c4\00d6\00fc\00dcber";U:6:"\00e5\00e4\00f6\00c5\00c4\00d6";}"
object(�berK��li��)#2 (1) {
  [u"��������ber"]=>
  unicode(6) "������"
}
unicode(80) "a:1:{U:6:"\00e5\00e4\00f6\00c5\00c4\00d6";U:6:"\00e5\00e4\00f6\00c5\00c4\00d6";}"
array(1) {
  [u"������"]=>
  unicode(6) "������"
}
