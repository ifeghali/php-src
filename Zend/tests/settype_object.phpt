--TEST--
casting different variables to object using settype() 
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
	settype($var, "object");
	var_dump($var);
}

echo "Done\n";
?>
--EXPECTF--
object(stdClass)#%d (1) {
  [u"scalar"]=>
  unicode(6) "string"
}
object(stdClass)#%d (1) {
  [u"scalar"]=>
  unicode(7) "8754456"
}
object(stdClass)#%d (1) {
  [u"scalar"]=>
  unicode(0) ""
}
object(stdClass)#%d (1) {
  [u"scalar"]=>
  unicode(1) " "
}
object(stdClass)#%d (1) {
  [u"scalar"]=>
  int(9876545)
}
object(stdClass)#%d (1) {
  [u"scalar"]=>
  float(0.1)
}
object(stdClass)#%d (0) {
}
object(stdClass)#%d (3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}
object(stdClass)#%d (1) {
  [u"scalar"]=>
  bool(false)
}
object(stdClass)#%d (1) {
  [u"scalar"]=>
  bool(true)
}
object(stdClass)#%d (0) {
}
object(stdClass)#%d (1) {
  [u"scalar"]=>
  resource(%d) of type (stream)
}
object(test)#%d (0) {
}
Done
