--TEST--
ucfirst() function
--INI--
precision=14
--FILE--
<?php
/* Make a string's first character uppercase */

echo "#### Basic and Various operations ####\n";
$str_array = array(
		    "testing ucfirst.",
 		    "1.testing ucfirst",
		    "hELLO wORLD",
		    'hELLO wORLD',
                    "\0",		// Null 
                    "\x00",		// Hex Null
                    "\x000",
                    "abcd",		// double quoted string
                    'xyz',		// single quoted string
                    string,		// without quotes
                    "-3",
                    -3,
                    '-3.344',
                    -3.344,
                    NULL,
                    "NULL",
                    "0",
                    0,
                    TRUE,		// bool type
                    "TRUE",
                    "1",
                    1,
                    1.234444,
                    FALSE,
                    "FALSE",
                    " ",
                    "     ",
                    'b',		// single char
                    '\t',		// escape sequences
                    "\t",
                    "12",
                    "12twelve",		// int + string
	     	  );
/* loop to test working of ucfirst with different values */
foreach ($str_array as $string) {
  var_dump( ucfirst($string) );
}



echo "\n#### Testing Miscelleneous inputs ####\n";

echo "--- Testing arrays ---";
$str_arr = array("hello", "?world", "!$%**()%**[][[[&@#~!", array());
var_dump( ucfirst($str_arr) );  

echo "\n--- Testing objects ---\n";
/* we get "Catchable fatal error: saying Object of class could not be converted
        to string" by default when an object is passed instead of string:
The error can be  avoided by chosing the __toString magix method as follows: */

class string {
  function __toString() {
    return "hello, world";
  }
}
$obj_string = new string;

var_dump(ucfirst("$obj_string"));


echo "\n--- Testing Resources ---\n";
$filename1 = "dummy.txt";
$file1 = fopen($filename1, "w");                // creating new file

/* getting resource type for file handle */
$string1 = get_resource_type($file1);
$string2 = (int)get_resource_type($file1);      // converting stream type to int

/* $string1 is of "stream" type */
var_dump(ucfirst($string1)); 

/* $string2 holds a value of "int(0)" */
var_dump(ucfirst($string2));

fclose($file1);                                 // closing the file "dummy.txt"
unlink("$filename1");                           // deletes "dummy.txt"


echo "\n--- Testing a longer and heredoc string ---\n";
$string = <<<EOD
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
@#$%^&**&^%$#@!~:())))((((&&&**%$###@@@!!!~~~~@###$%^&*
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
EOD;
var_dump(ucfirst($string));

echo "\n--- Testing a heredoc null string ---\n";
$str = <<<EOD
EOD;
var_dump(ucfirst($str));


echo "\n--- Testing simple and complex syntax strings ---\n";
$str = 'world';

/* Simple syntax */
var_dump(ucfirst("$str"));
var_dump(ucfirst("$str'S"));
var_dump(ucfirst("$strS"));

/* String with curly braces, complex syntax */
var_dump(ucfirst("${str}S"));
var_dump(ucfirst("{$str}S"));

echo "\n--- Nested ucfirst() ---\n";
var_dump(ucfirst(ucfirst("hello")));


echo "\n#### error conditions ####";
/* Zero arguments */
ucfirst();
/* More than expected no. of args */
ucfirst($str_array[0], $str_array[1]);
ucfirst((int)10, (int)20);

echo "Done\n";
?>
--EXPECTF--
#### Basic and Various operations ####

Notice: Use of undefined constant string - assumed 'string' in %s on line %d
unicode(16) "Testing ucfirst."
unicode(17) "1.testing ucfirst"
unicode(11) "HELLO wORLD"
unicode(11) "HELLO wORLD"
unicode(1) " "
unicode(1) " "
unicode(2) " 0"
unicode(4) "Abcd"
unicode(3) "Xyz"
unicode(6) "String"
unicode(2) "-3"
unicode(2) "-3"
unicode(6) "-3.344"
unicode(6) "-3.344"
unicode(0) ""
unicode(4) "NULL"
unicode(1) "0"
unicode(1) "0"
unicode(1) "1"
unicode(4) "TRUE"
unicode(1) "1"
unicode(1) "1"
unicode(8) "1.234444"
unicode(0) ""
unicode(5) "FALSE"
unicode(1) " "
unicode(5) "     "
unicode(1) "B"
unicode(2) "\t"
unicode(1) "	"
unicode(2) "12"
unicode(8) "12twelve"

#### Testing Miscelleneous inputs ####
--- Testing arrays ---
Warning: ucfirst() expects parameter 1 to be string (Unicode or binary), array given in %s on line %d
NULL

--- Testing objects ---
unicode(12) "Hello, world"

--- Testing Resources ---
unicode(6) "Stream"
unicode(1) "0"

--- Testing a longer and heredoc string ---
unicode(639) "Abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
@#$%^&**&^%$#@!~:())))((((&&&**%$###@@@!!!~~~~@###$%^&*
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789"

--- Testing a heredoc null string ---
unicode(0) ""

--- Testing simple and complex syntax strings ---
unicode(5) "World"
unicode(7) "World'S"

Notice: Undefined variable: strS in %s on line %d
unicode(0) ""
unicode(6) "WorldS"
unicode(6) "WorldS"

--- Nested ucfirst() ---
unicode(5) "Hello"

#### error conditions ####
Warning: ucfirst() expects exactly 1 parameter, 0 given in %s on line %d

Warning: ucfirst() expects exactly 1 parameter, 2 given in %s on line %d

Warning: ucfirst() expects exactly 1 parameter, 2 given in %s on line %d
Done
