--TEST--
mysqli fetch functions 
--FILE--
<?php
	include "connect.inc";
	
	/*** test mysqli_connect 127.0.0.1 ***/
	$link = mysqli_connect("localhost", $user, $passwd);

	mysqli_select_db($link, "test");

	$stmt = mysqli_prepare($link, "SELECT current_user(), database(), 'foo'");
	mysqli_bind_result($stmt, $c0, $c1, $c2); 
	mysqli_execute($stmt);

	mysqli_fetch($stmt);
//	mysqli_stmt_close($stmt);

	$test = array($c0, $c1, $c2);

	var_dump($test);
	mysqli_close($link);
?>
--EXPECT--
array(3) {
  [0]=>
  string(14) "root@localhost"
  [1]=>
  string(4) "test"
  [2]=>
  string(3) "foo"
}
