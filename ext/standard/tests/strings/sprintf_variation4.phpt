--TEST--
Test sprintf() function : usage variations - int formats with float values
--FILE--
<?php
/* Prototype  : string sprintf(string $format [, mixed $arg1 [, mixed ...]])
 * Description: Return a formatted string 
 * Source code: ext/standard/formatted_print.c
*/

echo "*** Testing sprintf() : integer formats with float values ***\n";

// array of float type values

$float_values = array (
  2147483648,
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

// various integer formats
$int_formats = array(
  "%d", "%hd", "%ld",
  "%Ld", " %d", "%d ",
  "\t%d", "\n%d", "%4d",
  "%30d", "%[0-9]", "%*d"
);
 
$count = 1;
foreach($float_values as $float_value) {
  echo "\n-- Iteration $count --\n";
  
  foreach($int_formats as $format) {
    var_dump( sprintf($format, $float_value) );
  }
  $count++;
};

echo "Done";
?>
--EXPECTF--
*** Testing sprintf() : integer formats with float values ***

-- Iteration 1 --
string(11) "-2147483648"
string(1) "d"
string(11) "-2147483648"
string(1) "d"
string(12) " -2147483648"
string(12) "-2147483648 "
string(12) "	-2147483648"
string(12) "
-2147483648"
string(11) "-2147483648"
string(30) "                   -2147483648"
string(4) "0-9]"
string(1) "d"

-- Iteration 2 --
string(10) "2147483647"
string(1) "d"
string(10) "2147483647"
string(1) "d"
string(11) " 2147483647"
string(11) "2147483647 "
string(11) "	2147483647"
string(11) "
2147483647"
string(10) "2147483647"
string(30) "                    2147483647"
string(4) "0-9]"
string(1) "d"

-- Iteration 3 --
string(11) "-2147483647"
string(1) "d"
string(11) "-2147483647"
string(1) "d"
string(12) " -2147483647"
string(12) "-2147483647 "
string(12) "	-2147483647"
string(12) "
-2147483647"
string(11) "-2147483647"
string(30) "                   -2147483647"
string(4) "0-9]"
string(1) "d"

-- Iteration 4 --
string(1) "0"
string(1) "d"
string(1) "0"
string(1) "d"
string(2) " 0"
string(2) "0 "
string(2) "	0"
string(2) "
0"
string(4) "   0"
string(30) "                             0"
string(4) "0-9]"
string(1) "d"

-- Iteration 5 --
string(1) "0"
string(1) "d"
string(1) "0"
string(1) "d"
string(2) " 0"
string(2) "0 "
string(2) "	0"
string(2) "
0"
string(4) "   0"
string(30) "                             0"
string(4) "0-9]"
string(1) "d"

-- Iteration 6 --
string(1) "1"
string(1) "d"
string(1) "1"
string(1) "d"
string(2) " 1"
string(2) "1 "
string(2) "	1"
string(2) "
1"
string(4) "   1"
string(30) "                             1"
string(4) "0-9]"
string(1) "d"

-- Iteration 7 --
string(6) "100000"
string(1) "d"
string(6) "100000"
string(1) "d"
string(7) " 100000"
string(7) "100000 "
string(7) "	100000"
string(7) "
100000"
string(6) "100000"
string(30) "                        100000"
string(4) "0-9]"
string(1) "d"

-- Iteration 8 --
string(8) "-1000000"
string(1) "d"
string(8) "-1000000"
string(1) "d"
string(9) " -1000000"
string(9) "-1000000 "
string(9) "	-1000000"
string(9) "
-1000000"
string(8) "-1000000"
string(30) "                      -1000000"
string(4) "0-9]"
string(1) "d"

-- Iteration 9 --
string(9) "100000000"
string(1) "d"
string(9) "100000000"
string(1) "d"
string(10) " 100000000"
string(10) "100000000 "
string(10) "	100000000"
string(10) "
100000000"
string(9) "100000000"
string(30) "                     100000000"
string(4) "0-9]"
string(1) "d"

-- Iteration 10 --
string(11) "-1000000000"
string(1) "d"
string(11) "-1000000000"
string(1) "d"
string(12) " -1000000000"
string(12) "-1000000000 "
string(12) "	-1000000000"
string(12) "
-1000000000"
string(11) "-1000000000"
string(30) "                   -1000000000"
string(4) "0-9]"
string(1) "d"

-- Iteration 11 --
string(2) "10"
string(1) "d"
string(2) "10"
string(1) "d"
string(3) " 10"
string(3) "10 "
string(3) "	10"
string(3) "
10"
string(4) "  10"
string(30) "                            10"
string(4) "0-9]"
string(1) "d"

-- Iteration 12 --
string(7) "1050000"
string(1) "d"
string(7) "1050000"
string(1) "d"
string(8) " 1050000"
string(8) "1050000 "
string(8) "	1050000"
string(8) "
1050000"
string(7) "1050000"
string(30) "                       1050000"
string(4) "0-9]"
string(1) "d"
Done
--UEXPECTF--
*** Testing sprintf() : integer formats with float values ***

-- Iteration 1 --
unicode(11) "-2147483648"
unicode(1) "d"
unicode(11) "-2147483648"
unicode(1) "d"
unicode(12) " -2147483648"
unicode(12) "-2147483648 "
unicode(12) "	-2147483648"
unicode(12) "
-2147483648"
unicode(11) "-2147483648"
unicode(30) "                   -2147483648"
unicode(4) "0-9]"
unicode(1) "d"

-- Iteration 2 --
unicode(10) "2147483647"
unicode(1) "d"
unicode(10) "2147483647"
unicode(1) "d"
unicode(11) " 2147483647"
unicode(11) "2147483647 "
unicode(11) "	2147483647"
unicode(11) "
2147483647"
unicode(10) "2147483647"
unicode(30) "                    2147483647"
unicode(4) "0-9]"
unicode(1) "d"

-- Iteration 3 --
unicode(11) "-2147483647"
unicode(1) "d"
unicode(11) "-2147483647"
unicode(1) "d"
unicode(12) " -2147483647"
unicode(12) "-2147483647 "
unicode(12) "	-2147483647"
unicode(12) "
-2147483647"
unicode(11) "-2147483647"
unicode(30) "                   -2147483647"
unicode(4) "0-9]"
unicode(1) "d"

-- Iteration 4 --
unicode(1) "0"
unicode(1) "d"
unicode(1) "0"
unicode(1) "d"
unicode(2) " 0"
unicode(2) "0 "
unicode(2) "	0"
unicode(2) "
0"
unicode(4) "   0"
unicode(30) "                             0"
unicode(4) "0-9]"
unicode(1) "d"

-- Iteration 5 --
unicode(1) "0"
unicode(1) "d"
unicode(1) "0"
unicode(1) "d"
unicode(2) " 0"
unicode(2) "0 "
unicode(2) "	0"
unicode(2) "
0"
unicode(4) "   0"
unicode(30) "                             0"
unicode(4) "0-9]"
unicode(1) "d"

-- Iteration 6 --
unicode(1) "1"
unicode(1) "d"
unicode(1) "1"
unicode(1) "d"
unicode(2) " 1"
unicode(2) "1 "
unicode(2) "	1"
unicode(2) "
1"
unicode(4) "   1"
unicode(30) "                             1"
unicode(4) "0-9]"
unicode(1) "d"

-- Iteration 7 --
unicode(6) "100000"
unicode(1) "d"
unicode(6) "100000"
unicode(1) "d"
unicode(7) " 100000"
unicode(7) "100000 "
unicode(7) "	100000"
unicode(7) "
100000"
unicode(6) "100000"
unicode(30) "                        100000"
unicode(4) "0-9]"
unicode(1) "d"

-- Iteration 8 --
unicode(8) "-1000000"
unicode(1) "d"
unicode(8) "-1000000"
unicode(1) "d"
unicode(9) " -1000000"
unicode(9) "-1000000 "
unicode(9) "	-1000000"
unicode(9) "
-1000000"
unicode(8) "-1000000"
unicode(30) "                      -1000000"
unicode(4) "0-9]"
unicode(1) "d"

-- Iteration 9 --
unicode(9) "100000000"
unicode(1) "d"
unicode(9) "100000000"
unicode(1) "d"
unicode(10) " 100000000"
unicode(10) "100000000 "
unicode(10) "	100000000"
unicode(10) "
100000000"
unicode(9) "100000000"
unicode(30) "                     100000000"
unicode(4) "0-9]"
unicode(1) "d"

-- Iteration 10 --
unicode(11) "-1000000000"
unicode(1) "d"
unicode(11) "-1000000000"
unicode(1) "d"
unicode(12) " -1000000000"
unicode(12) "-1000000000 "
unicode(12) "	-1000000000"
unicode(12) "
-1000000000"
unicode(11) "-1000000000"
unicode(30) "                   -1000000000"
unicode(4) "0-9]"
unicode(1) "d"

-- Iteration 11 --
unicode(2) "10"
unicode(1) "d"
unicode(2) "10"
unicode(1) "d"
unicode(3) " 10"
unicode(3) "10 "
unicode(3) "	10"
unicode(3) "
10"
unicode(4) "  10"
unicode(30) "                            10"
unicode(4) "0-9]"
unicode(1) "d"

-- Iteration 12 --
unicode(7) "1050000"
unicode(1) "d"
unicode(7) "1050000"
unicode(1) "d"
unicode(8) " 1050000"
unicode(8) "1050000 "
unicode(8) "	1050000"
unicode(8) "
1050000"
unicode(7) "1050000"
unicode(30) "                       1050000"
unicode(4) "0-9]"
unicode(1) "d"
Done
