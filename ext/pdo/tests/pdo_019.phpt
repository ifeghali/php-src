--TEST--
PDO Common: fetch() and while()
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

$db->exec('CREATE TABLE test(idx int NOT NULL PRIMARY KEY, txt VARCHAR(20))');
$db->exec('INSERT INTO test VALUES(0, \'String0\')'); 
$db->exec('INSERT INTO test VALUES(1, \'String1\')'); 
$db->exec('INSERT INTO test VALUES(2, \'String2\')'); 
$db->exec('INSERT INTO test VALUES(3, \'String3\')'); 


var_dump($db->query('SELECT COUNT(*) FROM test')->fetchColumn());

$stmt = $db->prepare('SELECT idx, txt FROM test ORDER by idx');

$stmt->execute();
$cont = $stmt->fetchAll(PDO_FETCH_COLUMN|PDO_FETCH_UNIQUE);
var_dump($cont);

echo "===WHILE===\n";

$stmt->bindColumn('idx', $idx);
$stmt->bindColumn('txt', $txt);
$stmt->execute();

while($stmt->fetch(PDO_FETCH_BOUND)) {
	var_dump(array($idx=>$txt));
}

?>
--EXPECT--
string(1) "4"
array(4) {
  [0]=>
  string(7) "String0"
  [1]=>
  string(7) "String1"
  [2]=>
  string(7) "String2"
  [3]=>
  string(7) "String3"
}
===WHILE===
array(1) {
  [0]=>
  string(7) "String0"
}
array(1) {
  [1]=>
  string(7) "String1"
}
array(1) {
  [2]=>
  string(7) "String2"
}
array(1) {
  [3]=>
  string(7) "String3"
}
