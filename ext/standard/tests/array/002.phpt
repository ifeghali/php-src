--TEST--
Test arsort, asort, krsort, ksort, rsort, and sort
--POST--
--GET--
--FILE--
<?php
require('ext/standard/tests/array/data.inc');

function test_sort ($sort_function, $data) {
    echo "\n -- Testing $sort_function() -- \n";
	echo "No second argument:\n";
    $sort_function ($data);
    var_dump ($data);
	echo "Using SORT_REGULAR:\n";
    $sort_function ($data, SORT_REGULAR);
    var_dump ($data);
	echo "Using SORT_NUMERIC:\n";
    $sort_function ($data, SORT_NUMERIC);
    var_dump ($data);
	echo "Using SORT_STRING\n";
    $sort_function ($data, SORT_STRING);
    var_dump ($data);
}

foreach (array ('arsort', 'asort', 'krsort', 'ksort', 'rsort', 'sort') as $test_function) {
    test_sort ($test_function, $data);
}

?>
--EXPECT--

 -- Testing arsort() -- 
No second argument:
array(7) {
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  ["test"]=>
  int(27)
  [2147483647]=>
  string(4) "test"
  [5]=>
  string(4) "Test"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [0]=>
  string(3) "PHP"
  [-2147483648]=>
  float(-0.33333333333333)
}
Using SORT_REGULAR:
array(7) {
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  ["test"]=>
  int(27)
  [2147483647]=>
  string(4) "test"
  [5]=>
  string(4) "Test"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [0]=>
  string(3) "PHP"
  [-2147483648]=>
  float(-0.33333333333333)
}
Using SORT_NUMERIC:
array(7) {
  ["test"]=>
  int(27)
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [0]=>
  string(3) "PHP"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [5]=>
  string(4) "Test"
  [2147483647]=>
  string(4) "test"
  [-2147483648]=>
  float(-0.33333333333333)
}
Using SORT_STRING
array(7) {
  [2147483647]=>
  string(4) "test"
  [5]=>
  string(4) "Test"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [0]=>
  string(3) "PHP"
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  ["test"]=>
  int(27)
  [-2147483648]=>
  float(-0.33333333333333)
}

 -- Testing asort() -- 
No second argument:
array(7) {
  [-2147483648]=>
  float(-0.33333333333333)
  [0]=>
  string(3) "PHP"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [5]=>
  string(4) "Test"
  [2147483647]=>
  string(4) "test"
  ["test"]=>
  int(27)
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
}
Using SORT_REGULAR:
array(7) {
  [-2147483648]=>
  float(-0.33333333333333)
  [0]=>
  string(3) "PHP"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [5]=>
  string(4) "Test"
  [2147483647]=>
  string(4) "test"
  ["test"]=>
  int(27)
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
}
Using SORT_NUMERIC:
array(7) {
  [-2147483648]=>
  float(-0.33333333333333)
  [2147483647]=>
  string(4) "test"
  [5]=>
  string(4) "Test"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [0]=>
  string(3) "PHP"
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  ["test"]=>
  int(27)
}
Using SORT_STRING
array(7) {
  [-2147483648]=>
  float(-0.33333333333333)
  ["test"]=>
  int(27)
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [0]=>
  string(3) "PHP"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [5]=>
  string(4) "Test"
  [2147483647]=>
  string(4) "test"
}

 -- Testing krsort() -- 
No second argument:
array(7) {
  [2147483647]=>
  string(4) "test"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [5]=>
  string(4) "Test"
  ["test"]=>
  int(27)
  [0]=>
  string(3) "PHP"
  [-2147483648]=>
  float(-0.33333333333333)
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
}
Using SORT_REGULAR:
array(7) {
  [2147483647]=>
  string(4) "test"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [5]=>
  string(4) "Test"
  [0]=>
  string(3) "PHP"
  ["test"]=>
  int(27)
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [-2147483648]=>
  float(-0.33333333333333)
}
Using SORT_NUMERIC:
array(7) {
  [2147483647]=>
  string(4) "test"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [5]=>
  string(4) "Test"
  ["test"]=>
  int(27)
  [0]=>
  string(3) "PHP"
  [-2147483648]=>
  float(-0.33333333333333)
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
}
Using SORT_STRING
array(7) {
  ["test"]=>
  int(27)
  [5]=>
  string(4) "Test"
  [2147483647]=>
  string(4) "test"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [0]=>
  string(3) "PHP"
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [-2147483648]=>
  float(-0.33333333333333)
}

 -- Testing ksort() -- 
No second argument:
array(7) {
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [-2147483648]=>
  float(-0.33333333333333)
  [0]=>
  string(3) "PHP"
  ["test"]=>
  int(27)
  [5]=>
  string(4) "Test"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [2147483647]=>
  string(4) "test"
}
Using SORT_REGULAR:
array(7) {
  [-2147483648]=>
  float(-0.33333333333333)
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  ["test"]=>
  int(27)
  [0]=>
  string(3) "PHP"
  [5]=>
  string(4) "Test"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [2147483647]=>
  string(4) "test"
}
Using SORT_NUMERIC:
array(7) {
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [-2147483648]=>
  float(-0.33333333333333)
  [0]=>
  string(3) "PHP"
  ["test"]=>
  int(27)
  [5]=>
  string(4) "Test"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [2147483647]=>
  string(4) "test"
}
Using SORT_STRING
array(7) {
  [-2147483648]=>
  float(-0.33333333333333)
  ["-2147483648"]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [0]=>
  string(3) "PHP"
  [17]=>
  string(27) "PHP: Hypertext Preprocessor"
  [2147483647]=>
  string(4) "test"
  [5]=>
  string(4) "Test"
  ["test"]=>
  int(27)
}

 -- Testing rsort() -- 
No second argument:
array(7) {
  [0]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [1]=>
  int(27)
  [2]=>
  string(4) "test"
  [3]=>
  string(4) "Test"
  [4]=>
  string(27) "PHP: Hypertext Preprocessor"
  [5]=>
  string(3) "PHP"
  [6]=>
  float(-0.33333333333333)
}
Using SORT_REGULAR:
array(7) {
  [0]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [1]=>
  int(27)
  [2]=>
  string(4) "test"
  [3]=>
  string(4) "Test"
  [4]=>
  string(27) "PHP: Hypertext Preprocessor"
  [5]=>
  string(3) "PHP"
  [6]=>
  float(-0.33333333333333)
}
Using SORT_NUMERIC:
array(7) {
  [0]=>
  int(27)
  [1]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [2]=>
  string(3) "PHP"
  [3]=>
  string(27) "PHP: Hypertext Preprocessor"
  [4]=>
  string(4) "Test"
  [5]=>
  string(4) "test"
  [6]=>
  float(-0.33333333333333)
}
Using SORT_STRING
array(7) {
  [0]=>
  string(4) "test"
  [1]=>
  string(4) "Test"
  [2]=>
  string(27) "PHP: Hypertext Preprocessor"
  [3]=>
  string(3) "PHP"
  [4]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [5]=>
  int(27)
  [6]=>
  float(-0.33333333333333)
}

 -- Testing sort() -- 
No second argument:
array(7) {
  [0]=>
  float(-0.33333333333333)
  [1]=>
  string(3) "PHP"
  [2]=>
  string(27) "PHP: Hypertext Preprocessor"
  [3]=>
  string(4) "Test"
  [4]=>
  string(4) "test"
  [5]=>
  int(27)
  [6]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
}
Using SORT_REGULAR:
array(7) {
  [0]=>
  float(-0.33333333333333)
  [1]=>
  string(3) "PHP"
  [2]=>
  string(27) "PHP: Hypertext Preprocessor"
  [3]=>
  string(4) "Test"
  [4]=>
  string(4) "test"
  [5]=>
  int(27)
  [6]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
}
Using SORT_NUMERIC:
array(7) {
  [0]=>
  float(-0.33333333333333)
  [1]=>
  string(4) "test"
  [2]=>
  string(4) "Test"
  [3]=>
  string(27) "PHP: Hypertext Preprocessor"
  [4]=>
  string(3) "PHP"
  [5]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [6]=>
  int(27)
}
Using SORT_STRING
array(7) {
  [0]=>
  float(-0.33333333333333)
  [1]=>
  int(27)
  [2]=>
  array(2) {
    [0]=>
    string(6) "banana"
    [1]=>
    string(6) "orange"
  }
  [3]=>
  string(3) "PHP"
  [4]=>
  string(27) "PHP: Hypertext Preprocessor"
  [5]=>
  string(4) "Test"
  [6]=>
  string(4) "test"
}
