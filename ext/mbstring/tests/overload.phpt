--TEST--
Function overloading test (said to be harmful)
--SKIPIF--
<?php 
	extension_loaded('mbstring') or die('skip mbstring not available'); 
	if (!function_exists("mail")) {
		die('skip mail() function is not available.');
	}
?>
--INI--
output_handler=
mbstring.func_overload=7
mbstring.internal_encoding=EUC-JP
--FILE--
<?php
echo mb_internal_encoding()."\n";

$ngchars = array('ǽ','ɽ','��','��');
$str = '��Ͻ�ܻ���Һ���ɽ��ǽ��ɽ��������˽��Ž�չ�ʸ����ͽ���Ƭ���ե���';
var_dump(strlen($str));
var_dump(mb_strlen($str));

$converted_str = mb_convert_encoding($str, 'Shift_JIS');
mb_regex_encoding('Shift_JIS');
foreach($ngchars as $c) {
	$c = mb_convert_encoding($c, 'Shift_JIS');
	$replaced = mb_convert_encoding(ereg_replace($c, '!!', $converted_str), mb_internal_encoding(), 'Shift_JIS');
	var_dump(strpos($replaced, '!!'));
}
?>
--EXPECT--
EUC-JP
int(33)
int(33)
int(10)
int(8)
int(3)
int(29)
