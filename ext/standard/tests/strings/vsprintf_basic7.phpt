--TEST--
Test vsprintf() function : basic functionality - unsigned format
--FILE--
<?php
/* Prototype  : string vsprintf(string $format , aaray $args)
 * Description: Return a formatted string 
 * Source code: ext/standard/formatted_print.c
*/

echo "*** Testing vsprintf() : basic functionality - using unsigned format ***\n";

// Initialise all required variables
$format = "format";
$format1 = "%u";
$format2 = "%u %u";
$format3 = "%u %u %u";
$arg1 = array(-1111);
$arg2 = array(-1111,-1234567);
$arg3 = array(-1111,-1234567,-2345432);

var_dump( vsprintf($format1,$arg1) );
var_dump( vsprintf($format2,$arg2) );
var_dump( vsprintf($format3,$arg3) );

echo "Done";
?>
--EXPECTF--
*** Testing vsprintf() : basic functionality - using unsigned format ***
string(10) "4294966185"
string(21) "4294966185 4293732729"
string(32) "4294966185 4293732729 4292621864"
Done

--UEXPECTF--
*** Testing vsprintf() : basic functionality - using unsigned format ***
unicode(10) "4294966185"
unicode(21) "4294966185 4293732729"
unicode(32) "4294966185 4293732729 4292621864"
Done