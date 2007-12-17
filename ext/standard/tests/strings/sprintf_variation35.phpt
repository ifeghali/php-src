--TEST--
Test sprintf() function : usage variations - hexa formats with float values
--FILE--
<?php
/* Prototype  : string sprintf(string $format [, mixed $arg1 [, mixed ...]])
 * Description: Return a formatted string 
 * Source code: ext/standard/formatted_print.c
*/

echo "*** Testing sprintf() : hexa formats with float values ***\n";

// array of float values 
$float_values = array(
  2147483647,
  0x800000001, // float value, beyond max positive int
  020000000001, // float value, beyond max positive int
  0.0,
  -0.1,
  1.0,
  1e5,
  -1e6,
  1E8,
  -1E9,
  10.0000000000000000005,
  10.5e+5
);

// array of hexa formats
$hexa_formats = array(  
  "%x", "%xx", "%lx", 
  "%Lx", " %x", "%x ",
  "\t%x", "\n%x", "%4x",
  "%30x", "%[0-9A-Fa-f]", "%*x"
);

$count = 1;
foreach($float_values as $float_value) {
  echo "\n-- Iteration $count --\n";
  
  foreach($hexa_formats as $format) {
    // with two arguments
    var_dump( sprintf($format, $float_value) );
  }
  $count++;
};

echo "Done";
?>
--EXPECTF--
*** Testing sprintf() : hexa formats with float values ***

-- Iteration 1 --
string(8) "7fffffff"
string(9) "7fffffffx"
string(8) "7fffffff"
string(1) "x"
string(9) " 7fffffff"
string(9) "7fffffff "
string(9) "	7fffffff"
string(9) "
7fffffff"
string(8) "7fffffff"
string(30) "                      7fffffff"
string(10) "0-9A-Fa-f]"
string(1) "x"

-- Iteration 2 --
string(8) "7fffffff"
string(9) "7fffffffx"
string(8) "7fffffff"
string(1) "x"
string(9) " 7fffffff"
string(9) "7fffffff "
string(9) "	7fffffff"
string(9) "
7fffffff"
string(8) "7fffffff"
string(30) "                      7fffffff"
string(10) "0-9A-Fa-f]"
string(1) "x"

-- Iteration 3 --
string(8) "80000001"
string(9) "80000001x"
string(8) "80000001"
string(1) "x"
string(9) " 80000001"
string(9) "80000001 "
string(9) "	80000001"
string(9) "
80000001"
string(8) "80000001"
string(30) "                      80000001"
string(10) "0-9A-Fa-f]"
string(1) "x"

-- Iteration 4 --
string(1) "0"
string(2) "0x"
string(1) "0"
string(1) "x"
string(2) " 0"
string(2) "0 "
string(2) "	0"
string(2) "
0"
string(4) "   0"
string(30) "                             0"
string(10) "0-9A-Fa-f]"
string(1) "x"

-- Iteration 5 --
string(1) "0"
string(2) "0x"
string(1) "0"
string(1) "x"
string(2) " 0"
string(2) "0 "
string(2) "	0"
string(2) "
0"
string(4) "   0"
string(30) "                             0"
string(10) "0-9A-Fa-f]"
string(1) "x"

-- Iteration 6 --
string(1) "1"
string(2) "1x"
string(1) "1"
string(1) "x"
string(2) " 1"
string(2) "1 "
string(2) "	1"
string(2) "
1"
string(4) "   1"
string(30) "                             1"
string(10) "0-9A-Fa-f]"
string(1) "x"

-- Iteration 7 --
string(5) "186a0"
string(6) "186a0x"
string(5) "186a0"
string(1) "x"
string(6) " 186a0"
string(6) "186a0 "
string(6) "	186a0"
string(6) "
186a0"
string(5) "186a0"
string(30) "                         186a0"
string(10) "0-9A-Fa-f]"
string(1) "x"

-- Iteration 8 --
string(8) "fff0bdc0"
string(9) "fff0bdc0x"
string(8) "fff0bdc0"
string(1) "x"
string(9) " fff0bdc0"
string(9) "fff0bdc0 "
string(9) "	fff0bdc0"
string(9) "
fff0bdc0"
string(8) "fff0bdc0"
string(30) "                      fff0bdc0"
string(10) "0-9A-Fa-f]"
string(1) "x"

-- Iteration 9 --
string(7) "5f5e100"
string(8) "5f5e100x"
string(7) "5f5e100"
string(1) "x"
string(8) " 5f5e100"
string(8) "5f5e100 "
string(8) "	5f5e100"
string(8) "
5f5e100"
string(7) "5f5e100"
string(30) "                       5f5e100"
string(10) "0-9A-Fa-f]"
string(1) "x"

-- Iteration 10 --
string(8) "c4653600"
string(9) "c4653600x"
string(8) "c4653600"
string(1) "x"
string(9) " c4653600"
string(9) "c4653600 "
string(9) "	c4653600"
string(9) "
c4653600"
string(8) "c4653600"
string(30) "                      c4653600"
string(10) "0-9A-Fa-f]"
string(1) "x"

-- Iteration 11 --
string(1) "a"
string(2) "ax"
string(1) "a"
string(1) "x"
string(2) " a"
string(2) "a "
string(2) "	a"
string(2) "
a"
string(4) "   a"
string(30) "                             a"
string(10) "0-9A-Fa-f]"
string(1) "x"

-- Iteration 12 --
string(6) "100590"
string(7) "100590x"
string(6) "100590"
string(1) "x"
string(7) " 100590"
string(7) "100590 "
string(7) "	100590"
string(7) "
100590"
string(6) "100590"
string(30) "                        100590"
string(10) "0-9A-Fa-f]"
string(1) "x"
Done
--UEXPECTF--
*** Testing sprintf() : hexa formats with float values ***

-- Iteration 1 --
unicode(8) "7fffffff"
unicode(9) "7fffffffx"
unicode(8) "7fffffff"
unicode(1) "x"
unicode(9) " 7fffffff"
unicode(9) "7fffffff "
unicode(9) "	7fffffff"
unicode(9) "
7fffffff"
unicode(8) "7fffffff"
unicode(30) "                      7fffffff"
unicode(10) "0-9A-Fa-f]"
unicode(1) "x"

-- Iteration 2 --
unicode(8) "7fffffff"
unicode(9) "7fffffffx"
unicode(8) "7fffffff"
unicode(1) "x"
unicode(9) " 7fffffff"
unicode(9) "7fffffff "
unicode(9) "	7fffffff"
unicode(9) "
7fffffff"
unicode(8) "7fffffff"
unicode(30) "                      7fffffff"
unicode(10) "0-9A-Fa-f]"
unicode(1) "x"

-- Iteration 3 --
unicode(8) "80000001"
unicode(9) "80000001x"
unicode(8) "80000001"
unicode(1) "x"
unicode(9) " 80000001"
unicode(9) "80000001 "
unicode(9) "	80000001"
unicode(9) "
80000001"
unicode(8) "80000001"
unicode(30) "                      80000001"
unicode(10) "0-9A-Fa-f]"
unicode(1) "x"

-- Iteration 4 --
unicode(1) "0"
unicode(2) "0x"
unicode(1) "0"
unicode(1) "x"
unicode(2) " 0"
unicode(2) "0 "
unicode(2) "	0"
unicode(2) "
0"
unicode(4) "   0"
unicode(30) "                             0"
unicode(10) "0-9A-Fa-f]"
unicode(1) "x"

-- Iteration 5 --
unicode(1) "0"
unicode(2) "0x"
unicode(1) "0"
unicode(1) "x"
unicode(2) " 0"
unicode(2) "0 "
unicode(2) "	0"
unicode(2) "
0"
unicode(4) "   0"
unicode(30) "                             0"
unicode(10) "0-9A-Fa-f]"
unicode(1) "x"

-- Iteration 6 --
unicode(1) "1"
unicode(2) "1x"
unicode(1) "1"
unicode(1) "x"
unicode(2) " 1"
unicode(2) "1 "
unicode(2) "	1"
unicode(2) "
1"
unicode(4) "   1"
unicode(30) "                             1"
unicode(10) "0-9A-Fa-f]"
unicode(1) "x"

-- Iteration 7 --
unicode(5) "186a0"
unicode(6) "186a0x"
unicode(5) "186a0"
unicode(1) "x"
unicode(6) " 186a0"
unicode(6) "186a0 "
unicode(6) "	186a0"
unicode(6) "
186a0"
unicode(5) "186a0"
unicode(30) "                         186a0"
unicode(10) "0-9A-Fa-f]"
unicode(1) "x"

-- Iteration 8 --
unicode(8) "fff0bdc0"
unicode(9) "fff0bdc0x"
unicode(8) "fff0bdc0"
unicode(1) "x"
unicode(9) " fff0bdc0"
unicode(9) "fff0bdc0 "
unicode(9) "	fff0bdc0"
unicode(9) "
fff0bdc0"
unicode(8) "fff0bdc0"
unicode(30) "                      fff0bdc0"
unicode(10) "0-9A-Fa-f]"
unicode(1) "x"

-- Iteration 9 --
unicode(7) "5f5e100"
unicode(8) "5f5e100x"
unicode(7) "5f5e100"
unicode(1) "x"
unicode(8) " 5f5e100"
unicode(8) "5f5e100 "
unicode(8) "	5f5e100"
unicode(8) "
5f5e100"
unicode(7) "5f5e100"
unicode(30) "                       5f5e100"
unicode(10) "0-9A-Fa-f]"
unicode(1) "x"

-- Iteration 10 --
unicode(8) "c4653600"
unicode(9) "c4653600x"
unicode(8) "c4653600"
unicode(1) "x"
unicode(9) " c4653600"
unicode(9) "c4653600 "
unicode(9) "	c4653600"
unicode(9) "
c4653600"
unicode(8) "c4653600"
unicode(30) "                      c4653600"
unicode(10) "0-9A-Fa-f]"
unicode(1) "x"

-- Iteration 11 --
unicode(1) "a"
unicode(2) "ax"
unicode(1) "a"
unicode(1) "x"
unicode(2) " a"
unicode(2) "a "
unicode(2) "	a"
unicode(2) "
a"
unicode(4) "   a"
unicode(30) "                             a"
unicode(10) "0-9A-Fa-f]"
unicode(1) "x"

-- Iteration 12 --
unicode(6) "100590"
unicode(7) "100590x"
unicode(6) "100590"
unicode(1) "x"
unicode(7) " 100590"
unicode(7) "100590 "
unicode(7) "	100590"
unicode(7) "
100590"
unicode(6) "100590"
unicode(30) "                        100590"
unicode(10) "0-9A-Fa-f]"
unicode(1) "x"
Done
