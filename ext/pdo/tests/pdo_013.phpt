--TEST--
PDO Common: PDOStatement iterator
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

$db->exec('CREATE TABLE test(id int NOT NULL PRIMARY KEY, val VARCHAR(10), grp VARCHAR(10))');
$db->exec('INSERT INTO test VALUES(1, \'A\', \'Group1\')'); 
$db->exec('INSERT INTO test VALUES(2, \'B\', \'Group2\')'); 

$SELECT = 'SELECT val, grp FROM test';

$stmt = $db->prepare($SELECT);

$stmt->execute();
$stmt->setFetchMode(PDO_FETCH_NUM);
foreach ($stmt as $data)
{
	var_dump($data);
}

class Test
{
	function __construct($name = 'N/A')
	{
		echo __METHOD__ . "($name)\n";
	}
}

foreach ($db->query($SELECT, PDO_FETCH_CLASS, 'Test') as $data)
{
	var_dump($data);
}

$stmt = $db->query($SELECT, PDO_FETCH_CLASS, 'Test', array('WOW'));

foreach($stmt as $data)
{
	var_dump($data);
}
?>
--EXPECTF--
array(2) {
  [0]=>
  string(1) "A"
  [1]=>
  string(6) "Group1"
}
array(2) {
  [0]=>
  string(1) "B"
  [1]=>
  string(6) "Group2"
}
Test::__construct(N/A)
object(Test)#%d (2) {
  ["val"]=>
  string(1) "A"
  ["grp"]=>
  string(6) "Group1"
}
Test::__construct(N/A)
object(Test)#%d (2) {
  ["val"]=>
  string(1) "B"
  ["grp"]=>
  string(6) "Group2"
}
Test::__construct(WOW)
object(Test)#%d (2) {
  ["val"]=>
  string(1) "A"
  ["grp"]=>
  string(6) "Group1"
}
Test::__construct(WOW)
object(Test)#%d (2) {
  ["val"]=>
  string(1) "B"
  ["grp"]=>
  string(6) "Group2"
}
