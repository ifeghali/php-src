--TEST--
Test sort() function : usage variations - sort mixed values, 'sort_flag' as default/SORT_REGULAR (OK to fail as result is unpredectable)
--FILE--
<?php
/* Prototype  : bool sort ( array &$array [, int $sort_flags] )
 * Description: This function sorts an array. 
                Elements will be arranged from lowest to highest when this function has completed.
 * Source code: ext/standard/array.c
*/

/*
 * testing sort() by providing mixed value array for $array argument with following flag values.
 * flag value as default
 * SORT_REGULAR - compare items normally
*/

echo "*** Testing sort() : usage variations ***\n";

// mixed value array
$mixed_values = array (
  array(), 
  array(array(33, -5, 6), array(11), array(22, -55), array() ),
  -4, "4", 4.00, "b", "5", -2, -2.0, -2.98989, "-.9", "True", "",
  NULL, "ab", "abcd", 0.0, -0, "abcd\x00abcd\x00abcd", '', true, false
);

echo "\n-- Testing sort() by supplying mixed value array, 'flag' value is default --\n";
$temp_array = $mixed_values;
var_dump(sort($temp_array) );
var_dump($temp_array);

echo "\n-- Testing sort() by supplying mixed value array, 'flag' value is SORT_REGULAR --\n";
$temp_array = $mixed_values;
var_dump(sort($temp_array, SORT_REGULAR) );
var_dump($temp_array);

echo "Done\n";
?>
--EXPECT--
*** Testing sort() : usage variations ***

-- Testing sort() by supplying mixed value array, 'flag' value is default --
bool(true)
array(22) {
  [0]=>
  int(-4)
  [1]=>
  float(-2.98989)
  [2]=>
  int(-2)
  [3]=>
  float(-2)
  [4]=>
  unicode(0) ""
  [5]=>
  unicode(0) ""
  [6]=>
  NULL
  [7]=>
  bool(false)
  [8]=>
  bool(true)
  [9]=>
  unicode(3) "-.9"
  [10]=>
  float(0)
  [11]=>
  int(0)
  [12]=>
  unicode(1) "4"
  [13]=>
  unicode(1) "5"
  [14]=>
  unicode(4) "True"
  [15]=>
  unicode(2) "ab"
  [16]=>
  unicode(4) "abcd"
  [17]=>
  unicode(14) "abcd abcd abcd"
  [18]=>
  unicode(1) "b"
  [19]=>
  float(4)
  [20]=>
  array(0) {
  }
  [21]=>
  array(4) {
    [0]=>
    array(3) {
      [0]=>
      int(33)
      [1]=>
      int(-5)
      [2]=>
      int(6)
    }
    [1]=>
    array(1) {
      [0]=>
      int(11)
    }
    [2]=>
    array(2) {
      [0]=>
      int(22)
      [1]=>
      int(-55)
    }
    [3]=>
    array(0) {
    }
  }
}

-- Testing sort() by supplying mixed value array, 'flag' value is SORT_REGULAR --
bool(true)
array(22) {
  [0]=>
  int(-4)
  [1]=>
  float(-2.98989)
  [2]=>
  int(-2)
  [3]=>
  float(-2)
  [4]=>
  unicode(0) ""
  [5]=>
  unicode(0) ""
  [6]=>
  NULL
  [7]=>
  bool(false)
  [8]=>
  bool(true)
  [9]=>
  unicode(3) "-.9"
  [10]=>
  float(0)
  [11]=>
  int(0)
  [12]=>
  unicode(1) "4"
  [13]=>
  unicode(1) "5"
  [14]=>
  unicode(4) "True"
  [15]=>
  unicode(2) "ab"
  [16]=>
  unicode(4) "abcd"
  [17]=>
  unicode(14) "abcd abcd abcd"
  [18]=>
  unicode(1) "b"
  [19]=>
  float(4)
  [20]=>
  array(0) {
  }
  [21]=>
  array(4) {
    [0]=>
    array(3) {
      [0]=>
      int(33)
      [1]=>
      int(-5)
      [2]=>
      int(6)
    }
    [1]=>
    array(1) {
      [0]=>
      int(11)
    }
    [2]=>
    array(2) {
      [0]=>
      int(22)
      [1]=>
      int(-55)
    }
    [3]=>
    array(0) {
    }
  }
}
Done
