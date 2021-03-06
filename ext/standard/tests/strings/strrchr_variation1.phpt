--TEST--
Test strrchr() function : usage variations - double quoted strings
--FILE--
<?php
/* Prototype  : string strrchr(string $haystack, string $needle);
 * Description: Finds the last occurrence of a character in a string.
 * Source code: ext/standard/string.c
*/

/* Test strrchr() function by passing various double quoted strings for 'haystack' & 'needle' */

echo "*** Testing strrchr() function: with various double quoted strings ***";
$haystack = "Hello,\t\n\0\n  $&!#%\o,()*+-./:;<=>?@hello123456he \x234 \101 ";
$needle = array(
  //regular strings
  "l",
  "L",
  "HELLO", 
  "hEllo",

  //escape characters
  "\t",
  "\T",
  "	",
  "\n",
  "\N",
  "
",  //new line

  //nulls
  "\0",
  NULL,
  null,
  
  //boolean false
  FALSE,
  false,

  //empty string
  "",  

  //special chars
  " ",
  "$",
  " $",
  "&",
  "!#",
  "%\o",
  "\o,",
  "()",
  "*+",
  "+",
  "-",
  ".",
  ".;",
  ":;",
  ";",
  "<=>",
  ">",
  "=>",
  "?",
  "@", 
  "@hEllo", 

  "12345", //decimal numeric string
  "\x23",  //hexadecimal numeric string
  "#",  //respective ASCII char of \x23
  "\101",  //octal numeric string
  "A",  //respective ASCII char of \101
  "456HEE",  //numerics + chars
  42, //needle as int(ASCII value of "*")
  $haystack  //haystack as needle
);

/* loop through to get the position of the needle in haystack string */
$count = 1;
for($index=0; $index<count($needle); $index++) {
  echo "\n-- Iteration $count --\n";
  var_dump( strrchr($haystack, $needle[$index]) );
  $count++;
}
echo "*** Done ***";
?>
--EXPECT--
*** Testing strrchr() function: with various double quoted strings ***
-- Iteration 1 --
unicode(16) "lo123456he #4 A "

-- Iteration 2 --
bool(false)

-- Iteration 3 --
unicode(53) "Hello,	
 
  $&!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 4 --
unicode(8) "he #4 A "

-- Iteration 5 --
unicode(47) "	
 
  $&!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 6 --
unicode(36) "\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 7 --
unicode(47) "	
 
  $&!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 8 --
unicode(44) "
  $&!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 9 --
unicode(36) "\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 10 --
unicode(44) "
  $&!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 11 --
unicode(45) " 
  $&!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 12 --
unicode(45) " 
  $&!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 13 --
unicode(45) " 
  $&!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 14 --
unicode(45) " 
  $&!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 15 --
unicode(45) " 
  $&!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 16 --
bool(false)

-- Iteration 17 --
unicode(1) " "

-- Iteration 18 --
unicode(41) "$&!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 19 --
unicode(1) " "

-- Iteration 20 --
unicode(40) "&!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 21 --
unicode(39) "!#%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 22 --
unicode(37) "%\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 23 --
unicode(36) "\o,()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 24 --
unicode(33) "()*+-./:;<=>?@hello123456he #4 A "

-- Iteration 25 --
unicode(31) "*+-./:;<=>?@hello123456he #4 A "

-- Iteration 26 --
unicode(30) "+-./:;<=>?@hello123456he #4 A "

-- Iteration 27 --
unicode(29) "-./:;<=>?@hello123456he #4 A "

-- Iteration 28 --
unicode(28) "./:;<=>?@hello123456he #4 A "

-- Iteration 29 --
unicode(28) "./:;<=>?@hello123456he #4 A "

-- Iteration 30 --
unicode(26) ":;<=>?@hello123456he #4 A "

-- Iteration 31 --
unicode(25) ";<=>?@hello123456he #4 A "

-- Iteration 32 --
unicode(24) "<=>?@hello123456he #4 A "

-- Iteration 33 --
unicode(22) ">?@hello123456he #4 A "

-- Iteration 34 --
unicode(23) "=>?@hello123456he #4 A "

-- Iteration 35 --
unicode(21) "?@hello123456he #4 A "

-- Iteration 36 --
unicode(20) "@hello123456he #4 A "

-- Iteration 37 --
unicode(20) "@hello123456he #4 A "

-- Iteration 38 --
unicode(14) "123456he #4 A "

-- Iteration 39 --
unicode(5) "#4 A "

-- Iteration 40 --
unicode(5) "#4 A "

-- Iteration 41 --
unicode(2) "A "

-- Iteration 42 --
unicode(2) "A "

-- Iteration 43 --
unicode(4) "4 A "

-- Iteration 44 --
unicode(31) "*+-./:;<=>?@hello123456he #4 A "

-- Iteration 45 --
unicode(53) "Hello,	
 
  $&!#%\o,()*+-./:;<=>?@hello123456he #4 A "
*** Done ***
