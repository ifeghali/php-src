--TEST--
Test strrev() function : usage variations - double quoted strings
--FILE--
<?php
/* Prototype  : string strrev(string $str);
 * Description: Reverse a string 
 * Source code: ext/standard/string.c
*/

/* Testing strrev() function with various double quoted strings */

echo "*** Testing strrev() : with various double quoted strings ***\n";
$str = "Hiiii";

$strings = array(
  //strings containing escape chars
  "hello\\world",
  "hello\$world",
  "\ttesting\ttesting\tstrrev",
  "testing\rstrrev testing strrev",
  "testing\fstrrev \f testing \nstrrev",
  "\ntesting\nstrrev\n testing \n strrev",
  "using\vvertical\vtab",

  //polyndrome strings
  "HelloolleH",
  "ababababababa",

  //numeric + strings  
  "Hello123",
  "123Hello",
  "123.34Hello",
  "Hello1.234",
  "123Hello1.234",

  //concatenated strings
  "Hello".chr(0)."World",
  "Hello"."world",
  "Hello".$str,

  //strings containing white spaces
  "              h",
  "h              ",
  "      h        ",
  "h  e  l  l  o  ",

  //strings containing quotes
  "\hello\'world",
  "hello\"world",

  //special chars in strings 
  "t@@#$% %test ^test &test *test +test -test",
  "!test ~test `test` =test= @test@test.com",
  "/test/r\test\strrev\t\u12ab /uu/",

  //only special chars
  "!@#$%^&*()_+=-`~"
);

$count = 1;
for( $index = 0; $index < count($strings); $index++ ) {
  echo "\n-- Iteration $count --\n";
  var_dump( strrev($strings[$index]) );
  $count ++;
}

echo "*** Done ***";
?>
--EXPECT--
*** Testing strrev() : with various double quoted strings ***

-- Iteration 1 --
unicode(11) "dlrow\olleh"

-- Iteration 2 --
unicode(11) "dlrow$olleh"

-- Iteration 3 --
unicode(23) "verrts	gnitset	gnitset	"

-- Iteration 4 --
unicode(29) "verrts gnitset verrtsgnitset"

-- Iteration 5 --
unicode(32) "verrts
 gnitset  verrtsgnitset"

-- Iteration 6 --
unicode(33) "verrts 
 gnitset 
verrts
gnitset
"

-- Iteration 7 --
unicode(18) "batlacitrevgnisu"

-- Iteration 8 --
unicode(10) "HelloolleH"

-- Iteration 9 --
unicode(13) "ababababababa"

-- Iteration 10 --
unicode(8) "321olleH"

-- Iteration 11 --
unicode(8) "olleH321"

-- Iteration 12 --
unicode(11) "olleH43.321"

-- Iteration 13 --
unicode(10) "432.1olleH"

-- Iteration 14 --
unicode(13) "432.1olleH321"

-- Iteration 15 --
unicode(11) "dlroW olleH"

-- Iteration 16 --
unicode(10) "dlrowolleH"

-- Iteration 17 --
unicode(10) "iiiiHolleH"

-- Iteration 18 --
unicode(15) "h              "

-- Iteration 19 --
unicode(15) "              h"

-- Iteration 20 --
unicode(15) "        h      "

-- Iteration 21 --
unicode(15) "  o  l  l  e  h"

-- Iteration 22 --
unicode(13) "dlrow'\olleh\"

-- Iteration 23 --
unicode(11) "dlrow"olleh"

-- Iteration 24 --
unicode(42) "tset- tset+ tset* tset& tset^ tset% %$#@@t"

-- Iteration 25 --
unicode(40) "moc.tset@tset@ =tset= `tset` tset~ tset!"

-- Iteration 26 --
unicode(25) "/uu/ ካ	verrts\tse	r/tset/"

-- Iteration 27 --
unicode(16) "~`-=+_)(*&^%$#@!"
*** Done ***
