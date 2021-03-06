--TEST--
"lcfirst()" function
--INI--
precision=14
--FILE--
<?php
/* Make a string's first character uppercase */

echo "#### Basic and Various operations ####\n";
$str_array = array(
		    "TesTing lcfirst.",
 		    "1.testing lcfirst",
		    "HELLO wORLD",
		    'HELLO wORLD',
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
/* loop to test working of lcfirst with different values */
foreach ($str_array as $string) {
  var_dump( lcfirst($string) );
}



echo "\n#### Testing Miscelleneous inputs ####\n";

echo "--- Testing arrays ---";
$str_arr = array("Hello", "?world", "!$%**()%**[][[[&@#~!", array());
var_dump( lcfirst($str_arr) );  

echo "\n--- Testing lowercamelcase action call example ---\n";
class Setter {
    
    protected $vars = array('partnerName' => false);
    
    public function __call($m, $v) {
        if (stristr($m, 'set')) {
            $action = lcfirst(substr($m, 3));
            $this->$action = $v[0];
        }
    }

    public function __set($key, $value) {
        if (array_key_exists($key, $this->vars)) {
            $this->vars[$key] = $value;
        }
    }

    public function __get($key) {
        if (array_key_exists($key, $this->vars)) {
            return $this->vars[$key];
        }
    }
}

$class = new Setter();
$class->setPartnerName('partnerName');
var_dump($class->partnerName);

echo "\n--- Testing objects ---\n";
/* we get "Catchable fatal error: saying Object of class could not be converted
        to string" by default when an object is passed instead of string:
The error can be  avoided by chosing the __toString magix method as follows: */

class string {
  function __toString() {
    return "Hello world";
  }
}
$obj_string = new string;

var_dump(lcfirst("$obj_string"));


echo "\n--- Testing Resources ---\n";
$filename1 = "dummy.txt";
$file1 = fopen($filename1, "w");                // creating new file

/* getting resource type for file handle */
$string1 = get_resource_type($file1);
$string2 = (int)get_resource_type($file1);      // converting stream type to int

/* $string1 is of "stream" type */
var_dump(lcfirst($string1)); 

/* $string2 holds a value of "int(0)" */
var_dump(lcfirst($string2));

fclose($file1);                                 // closing the file "dummy.txt"
unlink("$filename1");                           // deletes "dummy.txt"


echo "\n--- Testing a longer and heredoc string ---\n";
$string = <<<EOD
Abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
@#$%^&**&^%$#@!~:())))((((&&&**%$###@@@!!!~~~~@###$%^&*
abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
EOD;
var_dump(lcfirst($string));

echo "\n--- Testing a heredoc null string ---\n";
$str = <<<EOD
EOD;
var_dump(lcfirst($str));


echo "\n--- Testing simple and complex syntax strings ---\n";
$str = 'world';

/* Simple syntax */
var_dump(lcfirst("$str"));
var_dump(lcfirst("$str'S"));
var_dump(lcfirst("$strS"));

/* String with curly braces, complex syntax */
var_dump(lcfirst("${str}S"));
var_dump(lcfirst("{$str}S"));

echo "\n--- Nested lcfirst() ---\n";
var_dump(lcfirst(lcfirst("hello")));


echo "\n#### error conditions ####";
/* Zero arguments */
lcfirst();
/* More than expected no. of args */
lcfirst($str_array[0], $str_array[1]);
lcfirst((int)10, (int)20);

echo "Done\n";
?>
--EXPECTF--
#### Basic and Various operations ####

Notice: Use of undefined constant string - assumed 'string' in %s on line %d
unicode(16) "tesTing lcfirst."
unicode(17) "1.testing lcfirst"
unicode(11) "hELLO wORLD"
unicode(11) "hELLO wORLD"
unicode(1) " "
unicode(1) " "
unicode(2) " 0"
unicode(4) "abcd"
unicode(3) "xyz"
unicode(6) "string"
unicode(2) "-3"
unicode(2) "-3"
unicode(6) "-3.344"
unicode(6) "-3.344"
unicode(0) ""
unicode(4) "nULL"
unicode(1) "0"
unicode(1) "0"
unicode(1) "1"
unicode(4) "tRUE"
unicode(1) "1"
unicode(1) "1"
unicode(8) "1.234444"
unicode(0) ""
unicode(5) "fALSE"
unicode(1) " "
unicode(5) "     "
unicode(1) "b"
unicode(2) "\t"
unicode(1) "	"
unicode(2) "12"
unicode(8) "12twelve"

#### Testing Miscelleneous inputs ####
--- Testing arrays ---
Warning: lcfirst() expects parameter 1 to be string (Unicode or binary), array given in %s on line %d
NULL

--- Testing lowercamelcase action call example ---
unicode(%d) "partnerName"

--- Testing objects ---
unicode(11) "hello world"

--- Testing Resources ---
unicode(6) "stream"
unicode(1) "0"

--- Testing a longer and heredoc string ---
unicode(639) "abcdefghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz0123456789
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
unicode(5) "world"
unicode(7) "world'S"

Notice: Undefined variable: strS in %s on line %d
unicode(0) ""
unicode(6) "worldS"
unicode(6) "worldS"

--- Nested lcfirst() ---
unicode(5) "hello"

#### error conditions ####
Warning: lcfirst() expects exactly 1 parameter, 0 given in %s on line %d

Warning: lcfirst() expects exactly 1 parameter, 2 given in %s on line %d

Warning: lcfirst() expects exactly 1 parameter, 2 given in %s on line %d
Done
