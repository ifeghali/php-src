--TEST--
Test md5() function with ASCII output and raw binary output
--FILE--
<?php

/* Prototype: string md5( string str[, bool raw_output] )
 * Description: Calculates the MD5 hash os string str and returns that hash as string.
 */

echo md5("")."\n";
echo md5("a")."\n";
echo md5("abc")."\n";
echo md5("message digest")."\n";
echo md5("abcdefghijklmnopqrstuvwxyz")."\n";
echo md5("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")."\n";
echo md5("12345678901234567890123456789012345678901234567890123456789012345678901234567890")."\n";

/* Testing Error Conditions */
echo "\n*** Testing Error Conditions ***\n";
/* zero arguments */
var_dump ( md5() );

/* More than valid number of arguments ( valid is 2)  */
var_dump ( md5("a", true, NULL) );

$sample_string = "strings";
$counter = 1;

/* Variations of input strings in an array */
$mixed_array = array( NULL, 1234, "12345%", "abcd#123XYZ", "abc !@#$",
                      'string1', "$sample_string",
                      "qwertyuioplkjhgfdsazxcvbnm0123654789mnbvcxzasdfghjklpoiuytrewq7896541230mnbvcxzlkjhgfdsaqwertyuiop3210456789qwertyuioplkjhgfdsazxcvbnm1236547890123654789mnbvcxzlkjhgfdsapoiuytrewq");


/* looping to test md5() with different variations in its parameters */
foreach( $mixed_array as $input_str)
{
 echo "\n--Iteration ".$counter." --\n";
 var_dump( md5($input_str) );
 var_dump( md5($sample_string.$input_str."a") );
 var_dump( md5($input_str, true) );
 var_dump( md5($input_str, false) );
 var_dump( md5($input_str, 12) );
 var_dump( md5($input_str, 0) );
 var_dump( md5($input_str, -12) );
 var_dump( md5($input_str, NULL) );
 var_dump( md5($input_str, "STRING") );
 $counter++;
}

/* Use of class and objects */
echo "\n*** Testing with OBJECTS ***\n";
class string1
{
  public function __toString() {
    return "Object_for_md5_function";
  }
}
$obj = new string1;
var_dump( md5($obj) );

/* String with embedded NULL */
echo "\n*** Testing with String with embedded NULL ***\n";
var_dump( md5("1234\x0005678\x0000efgh\xijkl") );

/* heredoc string */
$str = <<<EOD
us
ing heredoc string
EOD;

echo "\n*** Testing with heredoc string ***\n";
var_dump( md5($str) );

echo "\nDone";

?>
--EXPECTF--
d41d8cd98f00b204e9800998ecf8427e
0cc175b9c0f1b6a831c399e269772661
900150983cd24fb0d6963f7d28e17f72
f96b697d7cb7938d525a2f31aaf161d0
c3fcd3d76192e4007dfb496cca67e13b
d174ab98d277d9f5a5611c2c9f419d9f
57edf4a22be3c955ac49da2e2107b67a

*** Testing Error Conditions ***

Warning: md5() expects at least 1 parameter, 0 given in %s on line %d
NULL

Warning: md5() expects at most 2 parameters, 3 given in %s on line %d
NULL

--Iteration 1 --
unicode(32) "d41d8cd98f00b204e9800998ecf8427e"
unicode(32) "48ea99a5b9577fecc55e990f8cb78acf"
string(16) "��ُ ��	���B~"
unicode(32) "d41d8cd98f00b204e9800998ecf8427e"
string(16) "��ُ ��	���B~"
unicode(32) "d41d8cd98f00b204e9800998ecf8427e"
string(16) "��ُ ��	���B~"
unicode(32) "d41d8cd98f00b204e9800998ecf8427e"
string(16) "��ُ ��	���B~"

--Iteration 2 --
unicode(32) "81dc9bdb52d04dc20036dbd8313ed055"
unicode(32) "9c41887e44db0be6c6d6a444b2b9dfe1"
string(16) "�ܛ�R�M� 6��1>�U"
unicode(32) "81dc9bdb52d04dc20036dbd8313ed055"
string(16) "�ܛ�R�M� 6��1>�U"
unicode(32) "81dc9bdb52d04dc20036dbd8313ed055"
string(16) "�ܛ�R�M� 6��1>�U"
unicode(32) "81dc9bdb52d04dc20036dbd8313ed055"
string(16) "�ܛ�R�M� 6��1>�U"

--Iteration 3 --
unicode(32) "702c3965a2d1be8a5b63bc758d554155"
unicode(32) "d637e55d925927d18d180d64c49ab088"
string(16) "p,9e�Ѿ�[c�u�UAU"
unicode(32) "702c3965a2d1be8a5b63bc758d554155"
string(16) "p,9e�Ѿ�[c�u�UAU"
unicode(32) "702c3965a2d1be8a5b63bc758d554155"
string(16) "p,9e�Ѿ�[c�u�UAU"
unicode(32) "702c3965a2d1be8a5b63bc758d554155"
string(16) "p,9e�Ѿ�[c�u�UAU"

--Iteration 4 --
unicode(32) "b1b64c7e52d11f209e74453e58b2cbd4"
unicode(32) "dcfc5b5870c11660e0b0b1b30e956423"
string(16) "��L~R� �tE>X���"
unicode(32) "b1b64c7e52d11f209e74453e58b2cbd4"
string(16) "��L~R� �tE>X���"
unicode(32) "b1b64c7e52d11f209e74453e58b2cbd4"
string(16) "��L~R� �tE>X���"
unicode(32) "b1b64c7e52d11f209e74453e58b2cbd4"
string(16) "��L~R� �tE>X���"

--Iteration 5 --
unicode(32) "0b392a56bf5827d6cee3c1925f4369ca"
unicode(32) "913a4525342633c4e32fd0a44cc8ab9e"
string(16) "9*V�X'�����_Ci�"
unicode(32) "0b392a56bf5827d6cee3c1925f4369ca"
string(16) "9*V�X'�����_Ci�"
unicode(32) "0b392a56bf5827d6cee3c1925f4369ca"
string(16) "9*V�X'�����_Ci�"
unicode(32) "0b392a56bf5827d6cee3c1925f4369ca"
string(16) "9*V�X'�����_Ci�"

--Iteration 6 --
unicode(32) "34b577be20fbc15477aadb9a08101ff9"
unicode(32) "26e3e41d0a96cb64e5b9351f48a1f215"
string(16) "4�w� ��Tw�ۚ�"
unicode(32) "34b577be20fbc15477aadb9a08101ff9"
string(16) "4�w� ��Tw�ۚ�"
unicode(32) "34b577be20fbc15477aadb9a08101ff9"
string(16) "4�w� ��Tw�ۚ�"
unicode(32) "34b577be20fbc15477aadb9a08101ff9"
string(16) "4�w� ��Tw�ۚ�"

--Iteration 7 --
unicode(32) "8bcf6629759bd278a5c6266bd9c054f8"
unicode(32) "3305dfd10eb830480b79dd94be5d49a0"
string(16) "��f)u��x��&k��T�"
unicode(32) "8bcf6629759bd278a5c6266bd9c054f8"
string(16) "��f)u��x��&k��T�"
unicode(32) "8bcf6629759bd278a5c6266bd9c054f8"
string(16) "��f)u��x��&k��T�"
unicode(32) "8bcf6629759bd278a5c6266bd9c054f8"
string(16) "��f)u��x��&k��T�"

--Iteration 8 --
unicode(32) "5c6a272108124da0ec38a7bc4f602a70"
unicode(32) "f74e0f11c941017cfb53bee1ba6a802e"
string(16) "\j'!M��8��O`*p"
unicode(32) "5c6a272108124da0ec38a7bc4f602a70"
string(16) "\j'!M��8��O`*p"
unicode(32) "5c6a272108124da0ec38a7bc4f602a70"
string(16) "\j'!M��8��O`*p"
unicode(32) "5c6a272108124da0ec38a7bc4f602a70"
string(16) "\j'!M��8��O`*p"

*** Testing with OBJECTS ***
unicode(32) "e52eb782329f9f684fddfc60ed462fc3"

*** Testing with String with embedded NULL ***
unicode(32) "3e2c064efcf5282f79f4d5c4ffe7c1fd"

*** Testing with heredoc string ***
unicode(32) "0b79c6be30e31d0d28afaf62a9521b80"

Done
