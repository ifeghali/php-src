--TEST--
Test oci_pconnect end-of-scope when statement returned
--SKIPIF--
<?php if (!extension_loaded('oci8')) die ("skip no oci8 extension"); ?>
--FILE--
<?php

require(dirname(__FILE__).'/details.inc');

// Initialization

$stmtarray = array(
	"drop table connect_scope2_tab",
	"create table connect_scope2_tab (c1 number)",
);

if (!empty($dbase))
	$c1 = oci_new_connect($user,$password,$dbase);
else
	$c1 = oci_new_connect($user,$password);
						 						 
foreach ($stmtarray as $stmt) {
	$s1 = oci_parse($c1, $stmt);
	@oci_execute($s1);
}

// Run Test

echo "Test 1 - oci_pconnect\n";

function f()
{
	global $user, $password, $dbase;

	if (!empty($dbase))
		$c = oci_pconnect($user,$password,$dbase);
	else
		$c = oci_pconnect($user,$password);
	$s = oci_parse($c, "insert into connect_scope2_tab values (1)");
	oci_execute($s, OCI_DEFAULT);  // no commit
	return($s); // this keeps the connection refcount positive so the connection isn't closed
}

$s2 = f();

// Check nothing committed yet

$s1 = oci_parse($c1, "select * from connect_scope2_tab");
oci_execute($s1, OCI_DEFAULT);
oci_fetch_all($s1, $r);
var_dump($r);

// insert 2nd row on returned statement, committing both rows
oci_execute($s2);

// Verify data was committed

$s1 = oci_parse($c1, "select * from connect_scope2_tab");
oci_execute($s1);
oci_fetch_all($s1, $r);
var_dump($r);

// Cleanup

$stmtarray = array(
	"drop table connect_scope2_tab"
);

foreach ($stmtarray as $stmt) {
	$s1 = oci_parse($c1, $stmt);
	oci_execute($s1);
}

echo "Done\n";

?>
--EXPECTF--
Test 1 - oci_pconnect
array(1) {
  [u"C1"]=>
  array(0) {
  }
}
array(1) {
  [u"C1"]=>
  array(2) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(1) "1"
  }
}
Done
