--TEST--
Test strrchr() function : usage variations - binary safe
--FILE--
<?php
/* Prototype  : string strrchr(string $haystack, string $needle);
 * Description: Finds the last occurrence of a character in a string.
 * Source code: ext/standard/string.c
*/

/* Test strrchr() function: with binary values & null terminated strings passed to 'str1' & 'str2' */

echo "*** Test strrchr() function: binary safe ***\n";
$haystacks = array(
  "Hello".chr(0)."World",
  chr(0)."Hello World",
  "Hello World".chr(0),
  chr(0).chr(0).chr(0),
  b"Hello\0world",
  "\0Hello",
  "Hello\0"
);

for($index = 0; $index < count($haystacks); $index++ ) {
  //needle as null string
  var_dump( strrchr($haystacks[$index], "\0") );
  //needle as NULL
  var_dump( strrchr($haystacks[$index], NULL) );
}
echo "*** Done ***";
?>
--EXPECT--
*** Test strrchr() function: binary safe ***
unicode(6) " World"
unicode(6) " World"
unicode(12) " Hello World"
unicode(12) " Hello World"
unicode(1) " "
unicode(1) " "
unicode(1) " "
unicode(1) " "
unicode(6) " world"
unicode(6) " world"
unicode(6) " Hello"
unicode(6) " Hello"
unicode(1) " "
unicode(1) " "
*** Done ***
