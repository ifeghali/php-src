--TEST--
PDO-SQLite: PDO_FETCH_ASSOC
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded("pdo_sqlite")) print "skip"; ?>
--FILE--
<?php

$db =new pdo('sqlite::memory:');

$db->exec('CREATE TABLE test(id int PRIMARY KEY, val VARCHAR(10))');
$db->exec('INSERT INTO test VALUES(1, "A")'); 
$db->exec('INSERT INTO test VALUES(2, "B")'); 
$db->exec('INSERT INTO test VALUES(3, "C")'); 

$stmt = $db->query('SELECT * FROM test');

var_dump($stmt->fetchAll(PDO_FETCH_ASSOC));
?>
===DONE===
<?php exit(0); ?>
--EXPECT--
array(3) {
  [0]=>
  array(2) {
    ["id"]=>
    string(1) "1"
    ["val"]=>
    string(1) "A"
  }
  [1]=>
  array(2) {
    ["id"]=>
    string(1) "2"
    ["val"]=>
    string(1) "B"
  }
  [2]=>
  array(2) {
    ["id"]=>
    string(1) "3"
    ["val"]=>
    string(1) "C"
  }
}
===DONE===
