--TEST--
Test array_change_key_case() function
--FILE--
<?php
/* Prototype: array array_change_key_case ( array $input [, int $case] )
   Description: Changes the keys in the input array to be all lowercase 
   or uppercase. The change depends on the last optional case parameter. 
   You can pass two constants there, CASE_UPPER and CASE_LOWER(default).
   The function will leave number indices as is. 
*/
$arrays = array (
  array (),
  array (0),
  array (1),
  array (-1),
  array (0, 2, 3, 4, 5),
  array (1, 2, 3, 4, 5),
  array ("" => 1),
  array ("a" => 1),
  array ("Z" => 1),
  array ("one" => 1),
  array ("ONE" => 1),
  array ("OnE" => 1),
  array ("oNe" => 1),
  array ("one" => 1, "two" => 2),
  array ("ONE" => 1, "two" => 2),
  array ("OnE" => 1, "two" => 2),
  array ("oNe" => 1, "two" => 2),
  array ("one" => 1, "TWO" => 2),
  array ("ONE" => 1, "TWO" => 2),
  array ("OnE" => 1, "TWO" => 2),
  array ("oNe" => 1, "TWO" => 2),
  array ("one" => 1, "TwO" => 2),  
  array ("ONE" => 1, "TwO" => 2),
  array ("OnE" => 1, "TwO" => 2),
  array ("oNe" => 1, "TwO" => 2),
  array ("one" => 1, "tWo" => 2),
  array ("ONE" => 1, "tWo" => 2),
  array ("OnE" => 1, "tWo" => 2),
  array ("oNe" => 1, "tWo" => 2),
  array ("one" => 1, 2),
  array ("ONE" => 1, 2),
  array ("OnE" => 1, 2),
  array ("oNe" => 1, 2),
  array ("ONE" => 1, "TWO" => 2, "THREE" => 3, "FOUR" => "four"),
  array ("one" => 1, "two" => 2, "three" => 3, "four" => "FOUR"),
  array ("ONE" => 1, "TWO" => 2, "three" => 3, "four" => "FOUR"),
  array ("one" => 1, "two" => 2, "THREE" => 3, "FOUR" => "four")
);

echo "*** Testing basic operations ***\n";
$loop_counter = 1;
foreach ($arrays as $item) {
        echo "** Iteration $loop_counter **\n"; $loop_counter++;
	var_dump(array_change_key_case($item));
	var_dump(array_change_key_case($item, CASE_UPPER));
	var_dump(array_change_key_case($item, CASE_LOWER));
	echo "\n";
}

echo "\n*** Testing possible variations ***\n";
$int_var  = -19;
$item = array ("one" => 1, "two" => 2, "THREE" => 3, "FOUR" => "four");

/* use 'case' argument other than CASE_LOWER & CASE_UPPER */
var_dump(array_change_key_case($item, 5)); 

/* when keys are different in terms of only case */
/* should return one value key pair with key being in lowercase */
var_dump( array_change_key_case( array("ONE" => 1, "one" => 3, "One" => 4) ) ); 

/* should return one value key pair with key being in uppercase */
var_dump( array_change_key_case( array("ONE" => 1, "one" => 2, "One" => 3), CASE_UPPER ) ); 
var_dump( array_change_key_case( array("ONE" => 1, "one" => 1, "One" => 2), 5 ) ); 

echo "\n*** Testing error conditions ***\n";
/* generate different failure conditions */
var_dump( array_change_key_case($int_var) ); // args less than expected
var_dump( array_change_key_case($int_var, CASE_UPPER) ); // invalid first argument
var_dump( array_change_key_case( array("ONE" => 1, "one" => 6, "One" => 5), "CASE_UPPER" ) ); //invalid 2rd arg
var_dump( array_change_key_case() ); // Zero argument
var_dump( array_change_key_case($item, $item["one"], "CASE_UPPER") ); // more than expected numbers

echo "end\n";
?>
--EXPECTF--
*** Testing basic operations ***
** Iteration 1 **
array(0) {
}
array(0) {
}
array(0) {
}

** Iteration 2 **
array(1) {
  [0]=>
  int(0)
}
array(1) {
  [0]=>
  int(0)
}
array(1) {
  [0]=>
  int(0)
}

** Iteration 3 **
array(1) {
  [0]=>
  int(1)
}
array(1) {
  [0]=>
  int(1)
}
array(1) {
  [0]=>
  int(1)
}

** Iteration 4 **
array(1) {
  [0]=>
  int(-1)
}
array(1) {
  [0]=>
  int(-1)
}
array(1) {
  [0]=>
  int(-1)
}

** Iteration 5 **
array(5) {
  [0]=>
  int(0)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
}
array(5) {
  [0]=>
  int(0)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
}
array(5) {
  [0]=>
  int(0)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
}

** Iteration 6 **
array(5) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
}
array(5) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
}
array(5) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
}

** Iteration 7 **
array(1) {
  [""]=>
  int(1)
}
array(1) {
  [""]=>
  int(1)
}
array(1) {
  [""]=>
  int(1)
}

** Iteration 8 **
array(1) {
  ["a"]=>
  int(1)
}
array(1) {
  ["A"]=>
  int(1)
}
array(1) {
  ["a"]=>
  int(1)
}

** Iteration 9 **
array(1) {
  ["z"]=>
  int(1)
}
array(1) {
  ["Z"]=>
  int(1)
}
array(1) {
  ["z"]=>
  int(1)
}

** Iteration 10 **
array(1) {
  ["one"]=>
  int(1)
}
array(1) {
  ["ONE"]=>
  int(1)
}
array(1) {
  ["one"]=>
  int(1)
}

** Iteration 11 **
array(1) {
  ["one"]=>
  int(1)
}
array(1) {
  ["ONE"]=>
  int(1)
}
array(1) {
  ["one"]=>
  int(1)
}

** Iteration 12 **
array(1) {
  ["one"]=>
  int(1)
}
array(1) {
  ["ONE"]=>
  int(1)
}
array(1) {
  ["one"]=>
  int(1)
}

** Iteration 13 **
array(1) {
  ["one"]=>
  int(1)
}
array(1) {
  ["ONE"]=>
  int(1)
}
array(1) {
  ["one"]=>
  int(1)
}

** Iteration 14 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 15 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 16 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 17 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 18 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 19 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 20 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 21 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 22 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 23 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 24 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 25 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 26 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 27 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 28 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 29 **
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
}

** Iteration 30 **
array(2) {
  ["one"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  [0]=>
  int(2)
}

** Iteration 31 **
array(2) {
  ["one"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  [0]=>
  int(2)
}

** Iteration 32 **
array(2) {
  ["one"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  [0]=>
  int(2)
}

** Iteration 33 **
array(2) {
  ["one"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  ["ONE"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  ["one"]=>
  int(1)
  [0]=>
  int(2)
}

** Iteration 34 **
array(4) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
  ["three"]=>
  int(3)
  ["four"]=>
  string(4) "four"
}
array(4) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
  ["THREE"]=>
  int(3)
  ["FOUR"]=>
  string(4) "four"
}
array(4) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
  ["three"]=>
  int(3)
  ["four"]=>
  string(4) "four"
}

** Iteration 35 **
array(4) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
  ["three"]=>
  int(3)
  ["four"]=>
  string(4) "FOUR"
}
array(4) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
  ["THREE"]=>
  int(3)
  ["FOUR"]=>
  string(4) "FOUR"
}
array(4) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
  ["three"]=>
  int(3)
  ["four"]=>
  string(4) "FOUR"
}

** Iteration 36 **
array(4) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
  ["three"]=>
  int(3)
  ["four"]=>
  string(4) "FOUR"
}
array(4) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
  ["THREE"]=>
  int(3)
  ["FOUR"]=>
  string(4) "FOUR"
}
array(4) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
  ["three"]=>
  int(3)
  ["four"]=>
  string(4) "FOUR"
}

** Iteration 37 **
array(4) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
  ["three"]=>
  int(3)
  ["four"]=>
  string(4) "four"
}
array(4) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
  ["THREE"]=>
  int(3)
  ["FOUR"]=>
  string(4) "four"
}
array(4) {
  ["one"]=>
  int(1)
  ["two"]=>
  int(2)
  ["three"]=>
  int(3)
  ["four"]=>
  string(4) "four"
}


*** Testing possible variations ***
array(4) {
  ["ONE"]=>
  int(1)
  ["TWO"]=>
  int(2)
  ["THREE"]=>
  int(3)
  ["FOUR"]=>
  string(4) "four"
}
array(1) {
  ["one"]=>
  int(4)
}
array(1) {
  ["ONE"]=>
  int(3)
}
array(1) {
  ["ONE"]=>
  int(2)
}

*** Testing error conditions ***

Warning: array_change_key_case() expects parameter 1 to be array, integer given in %s on line %d
NULL

Warning: array_change_key_case() expects parameter 1 to be array, integer given in %s on line %d
NULL

Warning: array_change_key_case() expects parameter 2 to be long, string given in %s on line %d
NULL

Warning: array_change_key_case() expects at least 1 parameter, 0 given in %s on line %d
NULL

Warning: array_change_key_case() expects at most 2 parameters, 3 given in %s on line %d
NULL
end
--UEXPECTF--
*** Testing basic operations ***
** Iteration 1 **
array(0) {
}
array(0) {
}
array(0) {
}

** Iteration 2 **
array(1) {
  [0]=>
  int(0)
}
array(1) {
  [0]=>
  int(0)
}
array(1) {
  [0]=>
  int(0)
}

** Iteration 3 **
array(1) {
  [0]=>
  int(1)
}
array(1) {
  [0]=>
  int(1)
}
array(1) {
  [0]=>
  int(1)
}

** Iteration 4 **
array(1) {
  [0]=>
  int(-1)
}
array(1) {
  [0]=>
  int(-1)
}
array(1) {
  [0]=>
  int(-1)
}

** Iteration 5 **
array(5) {
  [0]=>
  int(0)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
}
array(5) {
  [0]=>
  int(0)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
}
array(5) {
  [0]=>
  int(0)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
}

** Iteration 6 **
array(5) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
}
array(5) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
}
array(5) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
  [3]=>
  int(4)
  [4]=>
  int(5)
}

** Iteration 7 **
array(1) {
  [u""]=>
  int(1)
}
array(1) {
  [u""]=>
  int(1)
}
array(1) {
  [u""]=>
  int(1)
}

** Iteration 8 **
array(1) {
  [u"a"]=>
  int(1)
}
array(1) {
  [u"A"]=>
  int(1)
}
array(1) {
  [u"a"]=>
  int(1)
}

** Iteration 9 **
array(1) {
  [u"z"]=>
  int(1)
}
array(1) {
  [u"Z"]=>
  int(1)
}
array(1) {
  [u"z"]=>
  int(1)
}

** Iteration 10 **
array(1) {
  [u"one"]=>
  int(1)
}
array(1) {
  [u"ONE"]=>
  int(1)
}
array(1) {
  [u"one"]=>
  int(1)
}

** Iteration 11 **
array(1) {
  [u"one"]=>
  int(1)
}
array(1) {
  [u"ONE"]=>
  int(1)
}
array(1) {
  [u"one"]=>
  int(1)
}

** Iteration 12 **
array(1) {
  [u"one"]=>
  int(1)
}
array(1) {
  [u"ONE"]=>
  int(1)
}
array(1) {
  [u"one"]=>
  int(1)
}

** Iteration 13 **
array(1) {
  [u"one"]=>
  int(1)
}
array(1) {
  [u"ONE"]=>
  int(1)
}
array(1) {
  [u"one"]=>
  int(1)
}

** Iteration 14 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 15 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 16 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 17 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 18 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 19 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 20 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 21 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 22 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 23 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 24 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 25 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 26 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 27 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 28 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 29 **
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
}

** Iteration 30 **
array(2) {
  [u"one"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [0]=>
  int(2)
}

** Iteration 31 **
array(2) {
  [u"one"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [0]=>
  int(2)
}

** Iteration 32 **
array(2) {
  [u"one"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [0]=>
  int(2)
}

** Iteration 33 **
array(2) {
  [u"one"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  [u"ONE"]=>
  int(1)
  [0]=>
  int(2)
}
array(2) {
  [u"one"]=>
  int(1)
  [0]=>
  int(2)
}

** Iteration 34 **
array(4) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
  [u"three"]=>
  int(3)
  [u"four"]=>
  unicode(4) "four"
}
array(4) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
  [u"THREE"]=>
  int(3)
  [u"FOUR"]=>
  unicode(4) "four"
}
array(4) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
  [u"three"]=>
  int(3)
  [u"four"]=>
  unicode(4) "four"
}

** Iteration 35 **
array(4) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
  [u"three"]=>
  int(3)
  [u"four"]=>
  unicode(4) "FOUR"
}
array(4) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
  [u"THREE"]=>
  int(3)
  [u"FOUR"]=>
  unicode(4) "FOUR"
}
array(4) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
  [u"three"]=>
  int(3)
  [u"four"]=>
  unicode(4) "FOUR"
}

** Iteration 36 **
array(4) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
  [u"three"]=>
  int(3)
  [u"four"]=>
  unicode(4) "FOUR"
}
array(4) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
  [u"THREE"]=>
  int(3)
  [u"FOUR"]=>
  unicode(4) "FOUR"
}
array(4) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
  [u"three"]=>
  int(3)
  [u"four"]=>
  unicode(4) "FOUR"
}

** Iteration 37 **
array(4) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
  [u"three"]=>
  int(3)
  [u"four"]=>
  unicode(4) "four"
}
array(4) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
  [u"THREE"]=>
  int(3)
  [u"FOUR"]=>
  unicode(4) "four"
}
array(4) {
  [u"one"]=>
  int(1)
  [u"two"]=>
  int(2)
  [u"three"]=>
  int(3)
  [u"four"]=>
  unicode(4) "four"
}


*** Testing possible variations ***
array(4) {
  [u"ONE"]=>
  int(1)
  [u"TWO"]=>
  int(2)
  [u"THREE"]=>
  int(3)
  [u"FOUR"]=>
  unicode(4) "four"
}
array(1) {
  [u"one"]=>
  int(4)
}
array(1) {
  [u"ONE"]=>
  int(3)
}
array(1) {
  [u"ONE"]=>
  int(2)
}

*** Testing error conditions ***

Warning: array_change_key_case() expects parameter 1 to be array, integer given in %s on line %d
NULL

Warning: array_change_key_case() expects parameter 1 to be array, integer given in %s on line %d
NULL

Warning: array_change_key_case() expects parameter 2 to be long, Unicode string given in %s on line %d
NULL

Warning: array_change_key_case() expects at least 1 parameter, 0 given in %s on line %d
NULL

Warning: array_change_key_case() expects at most 2 parameters, 3 given in %s on line %d
NULL
end
