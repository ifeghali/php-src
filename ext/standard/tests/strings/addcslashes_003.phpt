--TEST--
Test addcslashes() function (variation 3)
--INI--
precision=14
--FILE--
<?php

/* Miscellaneous input */
echo "\n*** Testing addcslashes() with miscellaneous input arguments ***\n";
var_dump( addcslashes(b"", b"") );
var_dump( addcslashes(b"", b"burp") );
var_dump( addcslashes(b"kaboemkara!", b"") );
var_dump( addcslashes(b"foobarbaz", b'bar') );
var_dump( addcslashes(b'foo[ ]', b'A..z') );
var_dump( @addcslashes(b"zoo['.']", b'z..A') );
var_dump( addcslashes(b'abcdefghijklmnopqrstuvwxyz', b"a\145..\160z") );
var_dump( addcslashes( 123, 123 ) );
var_dump( addcslashes( 123, NULL) );
var_dump( addcslashes( NULL, 123) );
var_dump( addcslashes( 0, 0 ) );
var_dump( addcslashes( b"\0" , 0 ) );
var_dump( addcslashes( NULL, NULL) );
var_dump( addcslashes( -1.234578, 3 ) );
var_dump( addcslashes( b" ", b" ") );
var_dump( addcslashes( b"string\x00with\x00NULL", b"\0") );

echo "Done\n"; 

?>
--EXPECT--
*** Testing addcslashes() with miscellaneous input arguments ***
string(0) ""
string(0) ""
string(11) "kaboemkara!"
string(14) "foo\b\a\r\b\az"
string(11) "\f\o\o\[ \]"
string(10) "\zoo['\.']"
string(40) "\abcd\e\f\g\h\i\j\k\l\m\n\o\pqrstuvwxy\z"
string(6) "\1\2\3"
string(3) "123"
string(0) ""
string(2) "\0"
string(1) " "
string(0) ""
string(10) "-1.2\34578"
string(2) "\ "
string(22) "string\000with\000NULL"
Done
