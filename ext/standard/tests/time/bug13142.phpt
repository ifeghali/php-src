--TEST--
Bug #13142 strtotime handling of "M d H:i:s Y" format
--SKIPIF--
<?php
if (!@putenv("TZ=US/Eastern") || getenv("TZ") != 'US/Eastern') {
	die("skip unable to change TZ enviroment variable\n");
}
?>
--FILE--
<?php
putenv("TZ=US/Eastern");
echo date("r\n", strtotime("Sep 04 16:39:45 2001"));
echo date("r\n", strtotime("Sep 04 2001 16:39:45"));	
?>
--EXPECT--
Tue,  4 Sep 2001 16:39:45 -0400
Tue,  4 Sep 2001 16:39:45 -0400
