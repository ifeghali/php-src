--TEST--
mysqli_autocommit() tests
--SKIPIF--
<?php 
require_once('skipif.inc'); 
require_once('skipifconnectfailure.inc');
?>
--FILE--
<?php

	include "connect.inc";

	$mysqli = new mysqli($host, $user, $passwd, $db, $port, $socket);

	var_dump($mysqli->autocommit(false));
	$result = $mysqli->query("SELECT @@autocommit");
	var_dump($result->fetch_row());

	var_dump($mysqli->autocommit(true));
	$result = $mysqli->query("SELECT @@autocommit");
	var_dump($result->fetch_row());

?>
--EXPECTF--
bool(true)
array(1) {
  [0]=>
  string(1) "0"
}
bool(true)
array(1) {
  [0]=>
  string(1) "1"
}
--UEXPECTF--
bool(true)
array(1) {
  [0]=>
  unicode(1) "0"
}
bool(true)
array(1) {
  [0]=>
  unicode(1) "1"
}