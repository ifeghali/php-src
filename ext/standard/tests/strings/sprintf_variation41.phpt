--TEST--
Test sprintf() function : usage variations - unsigned formats with float values
--FILE--
<?php
/* Prototype  : string sprintf(string $format [, mixed $arg1 [, mixed ...]])
 * Description: Return a formatted string 
 * Source code: ext/standard/formatted_print.c
*/

echo "*** Testing sprintf() : unsigned formats with float values ***\n";

// array of float values 
$float_values = array(
  2147483647,
  0x800000001, // float value, beyond max positive int
  020000000001, // float value, beyond max positive int
  0.0,
  -0.1,
  1.0,
  +1e5,
  +5e+5,
  +0e-5,
  -1e5,
  -1e+5,
  -1e-2,
  1e5,
  1E8,
  -1E9,
  10.0000000000000000005,
  10.5e+5
);

// array of unsigned formats
$unsigned_formats = array(
  "%u", "%hu", "%lu",
  "%Lu", " %u", "%u ", 
  "\t%u", "\n%u", "%4u",
  "%30u", "%[0-9]", "%*u"
);

$count = 1;
foreach($float_values as $float_value) {
  echo "\n-- Iteration $count --\n";
  
  foreach($unsigned_formats as $format) {
    var_dump( sprintf($format, $float_value) );
  }
  $count++;
};

echo "Done";
?>
--EXPECTF--
*** Testing sprintf() : unsigned formats with float values ***

-- Iteration 1 --
string(10) "2147483647"
string(1) "u"
string(10) "2147483647"
string(1) "u"
string(11) " 2147483647"
string(11) "2147483647 "
string(11) "	2147483647"
string(11) "
2147483647"
string(10) "2147483647"
string(30) "                    2147483647"
string(4) "0-9]"
string(1) "u"

-- Iteration 2 --
string(1) "1"
string(1) "u"
string(1) "1"
string(1) "u"
string(2) " 1"
string(2) "1 "
string(2) "	1"
string(2) "
1"
string(4) "   1"
string(30) "                             1"
string(4) "0-9]"
string(1) "u"

-- Iteration 3 --
string(10) "2147483649"
string(1) "u"
string(10) "2147483649"
string(1) "u"
string(11) " 2147483649"
string(11) "2147483649 "
string(11) "	2147483649"
string(11) "
2147483649"
string(10) "2147483649"
string(30) "                    2147483649"
string(4) "0-9]"
string(1) "u"

-- Iteration 4 --
string(1) "0"
string(1) "u"
string(1) "0"
string(1) "u"
string(2) " 0"
string(2) "0 "
string(2) "	0"
string(2) "
0"
string(4) "   0"
string(30) "                             0"
string(4) "0-9]"
string(1) "u"

-- Iteration 5 --
string(1) "0"
string(1) "u"
string(1) "0"
string(1) "u"
string(2) " 0"
string(2) "0 "
string(2) "	0"
string(2) "
0"
string(4) "   0"
string(30) "                             0"
string(4) "0-9]"
string(1) "u"

-- Iteration 6 --
string(1) "1"
string(1) "u"
string(1) "1"
string(1) "u"
string(2) " 1"
string(2) "1 "
string(2) "	1"
string(2) "
1"
string(4) "   1"
string(30) "                             1"
string(4) "0-9]"
string(1) "u"

-- Iteration 7 --
string(6) "100000"
string(1) "u"
string(6) "100000"
string(1) "u"
string(7) " 100000"
string(7) "100000 "
string(7) "	100000"
string(7) "
100000"
string(6) "100000"
string(30) "                        100000"
string(4) "0-9]"
string(1) "u"

-- Iteration 8 --
string(6) "500000"
string(1) "u"
string(6) "500000"
string(1) "u"
string(7) " 500000"
string(7) "500000 "
string(7) "	500000"
string(7) "
500000"
string(6) "500000"
string(30) "                        500000"
string(4) "0-9]"
string(1) "u"

-- Iteration 9 --
string(1) "0"
string(1) "u"
string(1) "0"
string(1) "u"
string(2) " 0"
string(2) "0 "
string(2) "	0"
string(2) "
0"
string(4) "   0"
string(30) "                             0"
string(4) "0-9]"
string(1) "u"

-- Iteration 10 --
string(10) "4294867296"
string(1) "u"
string(10) "4294867296"
string(1) "u"
string(11) " 4294867296"
string(11) "4294867296 "
string(11) "	4294867296"
string(11) "
4294867296"
string(10) "4294867296"
string(30) "                    4294867296"
string(4) "0-9]"
string(1) "u"

-- Iteration 11 --
string(10) "4294867296"
string(1) "u"
string(10) "4294867296"
string(1) "u"
string(11) " 4294867296"
string(11) "4294867296 "
string(11) "	4294867296"
string(11) "
4294867296"
string(10) "4294867296"
string(30) "                    4294867296"
string(4) "0-9]"
string(1) "u"

-- Iteration 12 --
string(1) "0"
string(1) "u"
string(1) "0"
string(1) "u"
string(2) " 0"
string(2) "0 "
string(2) "	0"
string(2) "
0"
string(4) "   0"
string(30) "                             0"
string(4) "0-9]"
string(1) "u"

-- Iteration 13 --
string(6) "100000"
string(1) "u"
string(6) "100000"
string(1) "u"
string(7) " 100000"
string(7) "100000 "
string(7) "	100000"
string(7) "
100000"
string(6) "100000"
string(30) "                        100000"
string(4) "0-9]"
string(1) "u"

-- Iteration 14 --
string(9) "100000000"
string(1) "u"
string(9) "100000000"
string(1) "u"
string(10) " 100000000"
string(10) "100000000 "
string(10) "	100000000"
string(10) "
100000000"
string(9) "100000000"
string(30) "                     100000000"
string(4) "0-9]"
string(1) "u"

-- Iteration 15 --
string(10) "3294967296"
string(1) "u"
string(10) "3294967296"
string(1) "u"
string(11) " 3294967296"
string(11) "3294967296 "
string(11) "	3294967296"
string(11) "
3294967296"
string(10) "3294967296"
string(30) "                    3294967296"
string(4) "0-9]"
string(1) "u"

-- Iteration 16 --
string(2) "10"
string(1) "u"
string(2) "10"
string(1) "u"
string(3) " 10"
string(3) "10 "
string(3) "	10"
string(3) "
10"
string(4) "  10"
string(30) "                            10"
string(4) "0-9]"
string(1) "u"

-- Iteration 17 --
string(7) "1050000"
string(1) "u"
string(7) "1050000"
string(1) "u"
string(8) " 1050000"
string(8) "1050000 "
string(8) "	1050000"
string(8) "
1050000"
string(7) "1050000"
string(30) "                       1050000"
string(4) "0-9]"
string(1) "u"
Done

--UEXPECTF--
*** Testing sprintf() : unsigned formats with float values ***

-- Iteration 1 --
unicode(10) "2147483647"
unicode(1) "u"
unicode(10) "2147483647"
unicode(1) "u"
unicode(11) " 2147483647"
unicode(11) "2147483647 "
unicode(11) "	2147483647"
unicode(11) "
2147483647"
unicode(10) "2147483647"
unicode(30) "                    2147483647"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 2 --
unicode(1) "1"
unicode(1) "u"
unicode(1) "1"
unicode(1) "u"
unicode(2) " 1"
unicode(2) "1 "
unicode(2) "	1"
unicode(2) "
1"
unicode(4) "   1"
unicode(30) "                             1"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 3 --
unicode(10) "2147483649"
unicode(1) "u"
unicode(10) "2147483649"
unicode(1) "u"
unicode(11) " 2147483649"
unicode(11) "2147483649 "
unicode(11) "	2147483649"
unicode(11) "
2147483649"
unicode(10) "2147483649"
unicode(30) "                    2147483649"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 4 --
unicode(1) "0"
unicode(1) "u"
unicode(1) "0"
unicode(1) "u"
unicode(2) " 0"
unicode(2) "0 "
unicode(2) "	0"
unicode(2) "
0"
unicode(4) "   0"
unicode(30) "                             0"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 5 --
unicode(1) "0"
unicode(1) "u"
unicode(1) "0"
unicode(1) "u"
unicode(2) " 0"
unicode(2) "0 "
unicode(2) "	0"
unicode(2) "
0"
unicode(4) "   0"
unicode(30) "                             0"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 6 --
unicode(1) "1"
unicode(1) "u"
unicode(1) "1"
unicode(1) "u"
unicode(2) " 1"
unicode(2) "1 "
unicode(2) "	1"
unicode(2) "
1"
unicode(4) "   1"
unicode(30) "                             1"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 7 --
unicode(6) "100000"
unicode(1) "u"
unicode(6) "100000"
unicode(1) "u"
unicode(7) " 100000"
unicode(7) "100000 "
unicode(7) "	100000"
unicode(7) "
100000"
unicode(6) "100000"
unicode(30) "                        100000"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 8 --
unicode(6) "500000"
unicode(1) "u"
unicode(6) "500000"
unicode(1) "u"
unicode(7) " 500000"
unicode(7) "500000 "
unicode(7) "	500000"
unicode(7) "
500000"
unicode(6) "500000"
unicode(30) "                        500000"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 9 --
unicode(1) "0"
unicode(1) "u"
unicode(1) "0"
unicode(1) "u"
unicode(2) " 0"
unicode(2) "0 "
unicode(2) "	0"
unicode(2) "
0"
unicode(4) "   0"
unicode(30) "                             0"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 10 --
unicode(10) "4294867296"
unicode(1) "u"
unicode(10) "4294867296"
unicode(1) "u"
unicode(11) " 4294867296"
unicode(11) "4294867296 "
unicode(11) "	4294867296"
unicode(11) "
4294867296"
unicode(10) "4294867296"
unicode(30) "                    4294867296"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 11 --
unicode(10) "4294867296"
unicode(1) "u"
unicode(10) "4294867296"
unicode(1) "u"
unicode(11) " 4294867296"
unicode(11) "4294867296 "
unicode(11) "	4294867296"
unicode(11) "
4294867296"
unicode(10) "4294867296"
unicode(30) "                    4294867296"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 12 --
unicode(1) "0"
unicode(1) "u"
unicode(1) "0"
unicode(1) "u"
unicode(2) " 0"
unicode(2) "0 "
unicode(2) "	0"
unicode(2) "
0"
unicode(4) "   0"
unicode(30) "                             0"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 13 --
unicode(6) "100000"
unicode(1) "u"
unicode(6) "100000"
unicode(1) "u"
unicode(7) " 100000"
unicode(7) "100000 "
unicode(7) "	100000"
unicode(7) "
100000"
unicode(6) "100000"
unicode(30) "                        100000"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 14 --
unicode(9) "100000000"
unicode(1) "u"
unicode(9) "100000000"
unicode(1) "u"
unicode(10) " 100000000"
unicode(10) "100000000 "
unicode(10) "	100000000"
unicode(10) "
100000000"
unicode(9) "100000000"
unicode(30) "                     100000000"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 15 --
unicode(10) "3294967296"
unicode(1) "u"
unicode(10) "3294967296"
unicode(1) "u"
unicode(11) " 3294967296"
unicode(11) "3294967296 "
unicode(11) "	3294967296"
unicode(11) "
3294967296"
unicode(10) "3294967296"
unicode(30) "                    3294967296"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 16 --
unicode(2) "10"
unicode(1) "u"
unicode(2) "10"
unicode(1) "u"
unicode(3) " 10"
unicode(3) "10 "
unicode(3) "	10"
unicode(3) "
10"
unicode(4) "  10"
unicode(30) "                            10"
unicode(4) "0-9]"
unicode(1) "u"

-- Iteration 17 --
unicode(7) "1050000"
unicode(1) "u"
unicode(7) "1050000"
unicode(1) "u"
unicode(8) " 1050000"
unicode(8) "1050000 "
unicode(8) "	1050000"
unicode(8) "
1050000"
unicode(7) "1050000"
unicode(30) "                       1050000"
unicode(4) "0-9]"
unicode(1) "u"
Done