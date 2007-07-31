--TEST--
builtin functions tests
--FILE--
<?php

var_dump(get_resource_type());
var_dump(get_resource_type(""));
$fp = fopen(__FILE__, "r");
var_dump(get_resource_type($fp));
fclose($fp);
var_dump(get_resource_type($fp));

var_dump(gettype(get_loaded_extensions()));
var_dump(count(get_loaded_extensions()));
var_dump(count(get_loaded_extensions(true)));

define("USER_CONSTANT", "test");

var_dump(get_defined_constants(true, true));
var_dump(gettype(get_defined_constants(true)));
var_dump(gettype(get_defined_constants()));
var_dump(count(get_defined_constants()));

function test () {
}

var_dump(get_defined_functions(true));
var_dump(gettype(get_defined_functions()));
var_dump(count(get_defined_functions()));

var_dump(get_declared_interfaces(true));
var_dump(gettype(get_declared_interfaces()));
var_dump(count(get_declared_interfaces()));

var_dump(get_extension_funcs());
var_dump(get_extension_funcs(true));
var_dump(gettype(get_extension_funcs("standard")));
var_dump(count(get_extension_funcs("standard")));
var_dump(gettype(get_extension_funcs("zend")));
var_dump(count(get_extension_funcs("zend")));


echo "Done\n";
?>
--EXPECTF--	
Warning: Wrong parameter count for get_resource_type() in %s on line %d
NULL

Warning: Supplied argument is not a valid resource handle in %s on line %d
bool(false)
string(6) "stream"
string(7) "Unknown"
string(5) "array"
int(%d)
int(2)

Warning: Wrong parameter count for get_defined_constants() in %s on line %d
NULL
string(5) "array"
string(5) "array"
int(%d)

Warning: Wrong parameter count for get_defined_functions() in %s on line %d
NULL
string(5) "array"
int(%d)

Warning: Wrong parameter count for get_declared_interfaces() in %s on line %d
NULL
string(5) "array"
int(%d)

Warning: get_extension_funcs() expects exactly 1 parameter, 0 given in %s on line %d
NULL
bool(false)
string(5) "array"
int(%d)
string(5) "array"
int(%d)
Done
--UEXPECTF--
Warning: Wrong parameter count for get_resource_type() in %s on line %d
NULL

Warning: Supplied argument is not a valid resource handle in %s on line %d
bool(false)
unicode(6) "stream"
unicode(7) "Unknown"
unicode(5) "array"
int(%d)
int(2)

Warning: Wrong parameter count for get_defined_constants() in %s on line %d
NULL
unicode(5) "array"
unicode(5) "array"
int(%d)

Warning: Wrong parameter count for get_defined_functions() in %s on line %d
NULL
unicode(5) "array"
int(%d)

Warning: Wrong parameter count for get_declared_interfaces() in %s on line %d
NULL
unicode(5) "array"
int(%d)

Warning: get_extension_funcs() expects exactly 1 parameter, 0 given in %s on line %d
NULL
bool(false)
unicode(5) "array"
int(%d)
unicode(5) "array"
int(%d)
Done
