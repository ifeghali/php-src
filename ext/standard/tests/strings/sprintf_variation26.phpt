--TEST--
Test sprintf() function : usage variations - char formats with boolean values
--FILE--
<?php
/* Prototype  : string sprintf(string $format [, mixed $arg1 [, mixed ...]])
 * Description: Return a formatted string 
 * Source code: ext/standard/formatted_print.c
*/

echo "*** Testing sprintf() : char formats with boolean values ***\n";

// array of boolean values 
$boolean_values = array(
  true,
  false,
  TRUE,
  FALSE,
);

// array of char formats
$char_formats = array( 
  "%c", "%hc", "%lc", 
  "%Lc", " %c", "%c ",
  "\t%c", "\n%c", "%4c",
  "%30c", "%[a-bA-B@#$&]", "%*c"
);

$count = 1;
foreach($boolean_values as $boolean_value) {
  echo "\n-- Iteration $count --\n";
  
  foreach($char_formats as $format) {
    var_dump( sprintf($format, $boolean_value) );
  }
  $count++;
};

echo "Done";
?>
--EXPECT--
*** Testing sprintf() : char formats with boolean values ***

-- Iteration 1 --
unicode(1) ""
unicode(1) "c"
unicode(1) ""
unicode(1) "c"
unicode(2) " "
unicode(2) " "
unicode(2) "	"
unicode(2) "
"
unicode(1) ""
unicode(1) ""
unicode(11) "a-bA-B@#$&]"
unicode(1) "c"

-- Iteration 2 --
unicode(1) " "
unicode(1) "c"
unicode(1) " "
unicode(1) "c"
unicode(2) "  "
unicode(2) "  "
unicode(2) "	 "
unicode(2) "
 "
unicode(1) " "
unicode(1) " "
unicode(11) "a-bA-B@#$&]"
unicode(1) "c"

-- Iteration 3 --
unicode(1) ""
unicode(1) "c"
unicode(1) ""
unicode(1) "c"
unicode(2) " "
unicode(2) " "
unicode(2) "	"
unicode(2) "
"
unicode(1) ""
unicode(1) ""
unicode(11) "a-bA-B@#$&]"
unicode(1) "c"

-- Iteration 4 --
unicode(1) " "
unicode(1) "c"
unicode(1) " "
unicode(1) "c"
unicode(2) "  "
unicode(2) "  "
unicode(2) "	 "
unicode(2) "
 "
unicode(1) " "
unicode(1) " "
unicode(11) "a-bA-B@#$&]"
unicode(1) "c"
Done
