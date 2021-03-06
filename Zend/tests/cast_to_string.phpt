--TEST--
casting different variables to string 
--FILE--
<?php

$r = fopen(__FILE__, "r");

class test {
	function  __toString() {
		return "10";
	}
}

$o = new test;

$vars = array(
	"string",
	"8754456",
	"",
	"\0",
	9876545,
	0.10,
	array(),
	array(1,2,3),
	false,
	true,
	NULL,
	$r,
	$o
);

foreach ($vars as $var) {
	$tmp = (string)$var;
	var_dump($tmp);
}

echo "Done\n";
?>
--EXPECTF--
unicode(6) "string"
unicode(7) "8754456"
unicode(0) ""
unicode(1) " "
unicode(7) "9876545"
unicode(3) "0.1"

Notice: Array to string conversion in %s on line %d
unicode(5) "Array"

Notice: Array to string conversion in %s on line %d
unicode(5) "Array"
unicode(0) ""
unicode(1) "1"
unicode(0) ""
unicode(14) "Resource id #5"
unicode(2) "10"
Done
