--TEST--
Test filegroup() function: basic functionality
--SKIPIF--
<?php
if (!function_exists("posix_getgrgid")) { 
   die("skip no posix_getgrgid");
}
?>
--FILE--
<?php
/* Prototype: int filegroup ( string $filename )
 * Description: Returns the group ID of the file, or FALSE in case of an error.
 */

echo "*** Testing filegroup(): basic functionality ***\n"; 

echo "-- Testing with the file or directory created by owner --\n";

$file_path = dirname(__FILE__);
var_dump( posix_getgrgid( filegroup(__FILE__) ) );
var_dump( filegroup(".") );
var_dump( filegroup("./..") );

/* Newly created files and dirs */
$file_name = $file_path."/filegroup_basic.tmp";
$file_handle = fopen($file_name, "w");

$string = "Hello, world\n1234\n123Hello";
fwrite($file_handle, $string);
var_dump( filegroup($file_name) );
fclose($file_handle);

$dir_name = $file_path."/filegroup_basic";
mkdir($dir_name);
var_dump( filegroup($dir_name) );

echo "\n-- Testing with the standard file or directory --\n";
var_dump( filegroup("/etc/passwd") );
var_dump( filegroup("/etc") );
var_dump( filegroup("/") );

echo "\n*** Done ***\n";
?>

--CLEAN--
<?php

$file_path = dirname(__FILE__);
$file_name = $file_path."/filegroup_basic.tmp";
$dir_name  = $file_path."/filegroup_basic";
unlink($file_name);
rmdir($dir_name);
?>

--EXPECTF--
*** Testing filegroup(): basic functionality ***
-- Testing with the file or directory created by owner --
array(4) {
  ["name"]=>
  string(%d) "%s"
  ["passwd"]=>
  string(1) "x"
  ["members"]=>
  array(0) {
  }
  ["gid"]=>
  int(%d)
}
int(%d)
int(%d)
int(%d)
int(%d)

-- Testing with the standard file or directory --
int(0)
int(0)
int(0)

*** Done ***
--UEXPECTF--
*** Testing filegroup(): basic functionality ***
-- Testing with the file or directory created by owner --
array(4) {
  ["name"]=>
  string(%d) "%s"
  ["passwd"]=>
  string(1) "x"
  ["members"]=>
  array(0) {
  }
  ["gid"]=>
  int(%d)
}
int(%d)
int(%d)

Notice: fwrite(): 26 character unicode buffer downcoded for binary stream runtime_encoding in %s on line %d
int(%d)
int(%d)

-- Testing with the standard file or directory --
int(0)
int(0)
int(0)

*** Done ***