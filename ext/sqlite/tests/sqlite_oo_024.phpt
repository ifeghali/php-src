--TEST--
sqlite-oo: sqlite::fetch_object
--INI--
sqlite.assoc_case=0
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded("sqlite")) print "skip"; ?>
--FILE--
<?php 
include "blankdb_oo.inc";

class class24 {
	function __construct() {
		echo __METHOD__ . "\n";
	}
}

$data = array(
	"one",
	"two",
	"three"
	);

$db->query("CREATE TABLE strings(a)");

foreach ($data as $str) {
	$db->query("INSERT INTO strings VALUES('$str')");
}

echo "====stdclass====\n";
$res = $db->query("SELECT a FROM strings", SQLITE_ASSOC);
while ($res->has_more()) {
	var_dump($res->fetch_object('class24'));
}

echo "====class24!====\n";
$res = $db->query("SELECT a FROM strings", SQLITE_ASSOC);
while ($res->has_more()) {
	var_dump($res->fetch_object());
}

echo "====DONE!====\n";
?>
--EXPECTF--
====stdclass====
class24::__construct
object(class24)#%d (1) {
  ["a"]=>
  string(3) "one"
}
class24::__construct
object(class24)#%d (1) {
  ["a"]=>
  string(3) "two"
}
class24::__construct
object(class24)#%d (1) {
  ["a"]=>
  string(5) "three"
}
====class24!====
object(stdClass)#%d (1) {
  ["a"]=>
  string(3) "one"
}
object(stdClass)#%d (1) {
  ["a"]=>
  string(3) "two"
}
object(stdClass)#%d (1) {
  ["a"]=>
  string(5) "three"
}
====DONE!====
