--TEST--
Test array_multisort() function : usage variation - test sort order of all types
--FILE--
<?php
/* Prototype  : bool array_multisort(array ar1 [, SORT_ASC|SORT_DESC [, SORT_REGULAR|SORT_NUMERIC|SORT_STRING]] [, array ar2 [, SORT_ASC|SORT_DESC [, SORT_REGULAR|SORT_NUMERIC|SORT_STRING]], ...])
 * Description: Sort multiple arrays at once similar to how ORDER BY clause works in SQL 
 * Source code: ext/standard/array.c
 * Alias to functions: 
 */

echo "*** Testing array_multisort() : usage variation  - test sort order of all types***\n";

// define some classes
class classWithToString {
	public function __toString() {
		return "Class A object";
	}
}

class classWithoutToString { }

$inputs = array(
      'int 0' => 0,
      'float -10.5' => -10.5,
      array(),
      'uppercase NULL' => NULL,
      'lowercase true' => true,
      'empty string DQ' => "",
      'string DQ' => "string",
      'instance of classWithToString' => new classWithToString(),
      'instance of classWithoutToString' => new classWithoutToString(),
      'undefined var' => @$undefined_var,
);

var_dump(array_multisort($inputs, SORT_NUMERIC));
var_dump($inputs);

?>
===DONE===
--EXPECTF--
*** Testing array_multisort() : usage variation  - test sort order of all types***

Notice: Object of class classWithToString could not be converted to double in %sarray_multisort_variation9.php on line %d

Notice: Object of class classWithoutToString could not be converted to double in %sarray_multisort_variation9.php on line %d

Notice: Object of class classWithoutToString could not be converted to double in %sarray_multisort_variation9.php on line %d

Notice: Object of class classWithoutToString could not be converted to double in %sarray_multisort_variation9.php on line %d

Notice: Object of class classWithoutToString could not be converted to double in %sarray_multisort_variation9.php on line %d
bool(true)
array(10) {
  [u"float -10.5"]=>
  float(-10.5)
  [u"string DQ"]=>
  unicode(6) "string"
  [u"undefined var"]=>
  NULL
  [u"empty string DQ"]=>
  unicode(0) ""
  [u"uppercase NULL"]=>
  NULL
  [u"int 0"]=>
  int(0)
  [0]=>
  array(0) {
  }
  [u"instance of classWithoutToString"]=>
  object(classWithoutToString)#2 (0) {
  }
  [u"lowercase true"]=>
  bool(true)
  [u"instance of classWithToString"]=>
  object(classWithToString)#1 (0) {
  }
}
===DONE===