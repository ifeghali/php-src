--TEST--
reading/writing BFILE LOBs
--SKIPIF--
<?php if (!extension_loaded('oci8')) die("skip no oci8 extension");
include "details.inc";
if (empty($oracle_on_localhost)) die("skip this test won't work with remote Oracle");
?>
--FILE--
<?php

require dirname(__FILE__).'/connect.inc';

$realdirname = dirname(__FILE__);
$realfilename1 = "oci8bfiletest1.txt";
$fullname1 = $realdirname."/".$realfilename1;
$realfilename2 = "oci8bfiletest2.txt";
$fullname2 = $realdirname."/".$realfilename2;
$realfilename3 = "oci8bfiletest3.txt";
$fullname3 = $realdirname."/".$realfilename3;

// Setup
$s = oci_parse($c, "drop table FileTest");
@oci_execute($s);

$s = oci_parse($c, "drop directory TestDir");
@oci_execute($s);

$s = oci_parse($c, "create directory TestDir as '$realdirname'");
oci_execute($s);

file_put_contents($fullname1, 'Some text in the bfile 1');
file_put_contents($fullname2, 'Some text in the bfile 2');
file_put_contents($fullname3, 'Some text in the bfile 3');

$s = oci_parse($c, "create table FileTest (FileNum number, FileDesc varchar2(30), Image bfile)");
oci_execute($s);

$s = oci_parse($c, "insert into FileTest (FileNum, FileDesc, Image) values (1, 'Description 1', bfilename('TESTDIR', '$realfilename1'))");
oci_execute($s);

$s = oci_parse($c, "insert into FileTest (FileNum, FileDesc, Image) values (2, 'Description 2', bfilename('TESTDIR', '$realfilename2'))");
oci_execute($s);

$s = oci_parse($c, "insert into FileTest (FileNum, FileDesc, Image) values (3, 'Description 3', bfilename('TESTDIR', '$realfilename3'))");
oci_execute($s);

// Run tests

echo "Test 1. Check how many rows in the table\n";

$s = oci_parse($c, "select count(*) numrows from FileTest");
oci_execute($s);
oci_fetch_all($s, $res);
var_dump($res);

echo "Test 2\n";
$s = oci_parse($c, "select * from FileTest order by FileNum");
oci_execute($s);
oci_fetch_all($s, $res);
var_dump($res);

echo "Test 3\n";
$d = oci_new_descriptor($c, OCI_D_FILE);

$s = oci_parse($c, "insert into FileTest (FileNum, FileDesc, Image) values (2, 'Description 2', bfilename('TESTDIR', '$realfilename1')) returning Image into :im");
oci_bind_by_name($s, ":im", $d, -1, OCI_B_BFILE);
oci_execute($s);

$r = $d->read(40);
var_dump($r);

unlink($fullname1);
unlink($fullname2);
unlink($fullname3);

$s = oci_parse($c, "drop table FileTest");
oci_execute($s);

$s = oci_parse($c, "drop directory TestDir");
oci_execute($s);

echo "Done\n";
?>
--EXPECTF-- 
Test 1. Check how many rows in the table
array(1) {
  [u"NUMROWS"]=>
  array(1) {
    [0]=>
    unicode(1) "3"
  }
}
Test 2
array(3) {
  [u"FILENUM"]=>
  array(3) {
    [0]=>
    unicode(1) "1"
    [1]=>
    unicode(1) "2"
    [2]=>
    unicode(1) "3"
  }
  [u"FILEDESC"]=>
  array(3) {
    [0]=>
    unicode(13) "Description 1"
    [1]=>
    unicode(13) "Description 2"
    [2]=>
    unicode(13) "Description 3"
  }
  [u"IMAGE"]=>
  array(3) {
    [0]=>
    string(24) "Some text in the bfile 1"
    [1]=>
    string(24) "Some text in the bfile 2"
    [2]=>
    string(24) "Some text in the bfile 3"
  }
}
Test 3
string(24) "Some text in the bfile 1"
Done
