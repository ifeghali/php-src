--TEST--
mb_http_input() 
--SKIPIF--
<?php
	include('skipif.inc');
	if(php_sapi_name()!='cgi') {
		die("skip\n");
	}
?>
--POST--
a=���ܸ�0123456789���ܸ쥫�����ʤҤ餬��
--GET--
b=���ܸ�0123456789���ܸ쥫�����ʤҤ餬��
--FILE--
<?php include('003.inc'); ?>
--EXPECT--
���ܸ�0123456789���ܸ쥫�����ʤҤ餬��
���ܸ�0123456789���ܸ쥫�����ʤҤ餬��
OK

