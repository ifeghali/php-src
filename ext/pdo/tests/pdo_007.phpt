--TEST--
PDO Common: PDO_FETCH_UNIQUE
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

$db->exec('CREATE TABLE test(id CHAR(1) NOT NULL PRIMARY KEY, val VARCHAR(10))');
$db->exec("INSERT INTO test VALUES('A', 'A')");
$db->exec("INSERT INTO test VALUES('B', 'A')");
$db->exec("INSERT INTO test VALUES('C', 'C')");

$stmt = $db->prepare('SELECT id, val from test');

$stmt->execute();
var_dump($stmt->fetchAll(PDO_FETCH_NUM|PDO_FETCH_UNIQUE));

$stmt->execute();
var_dump($stmt->fetchAll(PDO_FETCH_ASSOC|PDO_FETCH_UNIQUE));

?>
--EXPECT--
array(3) {
  ["A"]=>
  array(1) {
    [0]=>
    string(1) "A"
  }
  ["B"]=>
  array(1) {
    [0]=>
    string(1) "A"
  }
  ["C"]=>
  array(1) {
    [0]=>
    string(1) "C"
  }
}
array(3) {
  ["A"]=>
  array(1) {
    ["val"]=>
    string(1) "A"
  }
  ["B"]=>
  array(1) {
    ["val"]=>
    string(1) "A"
  }
  ["C"]=>
  array(1) {
    ["val"]=>
    string(1) "C"
  }
}
