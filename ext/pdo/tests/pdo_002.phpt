--TEST--
PDO Common: PDO_FETCH_NUM
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) print 'skip';
$dir = getenv('REDIR_TEST_DIR');
if (false == $dir) print 'skip no driver';
require_once $dir . 'pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php
require getenv('REDIR_TEST_DIR') . 'pdo_test.inc';
$db = PDOTest::factory();

$db->exec('CREATE TABLE test(id int NOT NULL PRIMARY KEY, val VARCHAR(10))');
$db->exec("INSERT INTO test VALUES(1, 'A')");
$db->exec("INSERT INTO test VALUES(2, 'B')");
$db->exec("INSERT INTO test VALUES(3, 'C')");

$stmt = $db->prepare('SELECT * from test');
$stmt->execute();

var_dump($stmt->fetchAll(PDO_FETCH_NUM));
?>
--EXPECT--
array(3) {
  [0]=>
  array(2) {
    [0]=>
    string(1) "1"
    [1]=>
    string(1) "A"
  }
  [1]=>
  array(2) {
    [0]=>
    string(1) "2"
    [1]=>
    string(1) "B"
  }
  [2]=>
  array(2) {
    [0]=>
    string(1) "3"
    [1]=>
    string(1) "C"
  }
}
