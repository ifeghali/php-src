--TEST--
Test dir() function : usage variations - unexpected value for 'dir' argument
--FILE--
<?php
/* 
 * Prototype  : object dir(string $directory[, resource $context])
 * Description: Directory class with properties, handle and class and methods read, rewind and close
 * Source code: ext/standard/dir.c
 */

/*
 * Passing non string values to 'directory' argument of dir() and see
 * that the function outputs proper warning messages wherever expected.
 */

echo "*** Testing dir() : unexpected values for \$directory argument ***\n";

// get an unset variable
$unset_var = 10;
unset($unset_var);

class A
{
  public $var;
  public function init() {
    $this->var = 10;
  }
  public function __toString() {
    return "ClassAObject";
  }
}

// get a resource variable
$fp = fopen(__FILE__, "r"); // get a file handle 
$dfp = opendir( dirname(__FILE__) ); // get a dir handle

// unexpected values to be passed to $directory argument
$unexpected_values = array (

       // array data
/*1*/  array(),
       array(0),
       array(1),
       array(1, 2),
       array('color' => 'red', 'item' => 'pen'),

       // null data
/*6*/  NULL,
       null,

       // boolean data
/*8*/  true,
       false,
       TRUE,
       FALSE,

       // empty data
/*12*/ "",
       '',

       // undefined data
/*14*/ @$undefined_var,

       // unset data
/*15*/ @$unset_var,

       // resource variable(dir and file handle)
/*16*/ $fp,
       $dfp,

       // object data
/*18*/ new A()
);

// loop through various elements of $unexpected_values to check the behavior of dir()
$iterator = 1;
foreach( $unexpected_values as $unexpected_value ) {
  echo "\n-- Iteration $iterator --\n";
  var_dump( dir($unexpected_value) );
  $iterator++;
}

fclose($fp);
closedir($dfp);
echo "Done";
?>
--EXPECTF--
*** Testing dir() : unexpected values for $directory argument ***

-- Iteration 1 --

Notice: Array to string conversion in %s on line %d

Warning: dir(Array): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 2 --

Notice: Array to string conversion in %s on line %d

Warning: dir(Array): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 3 --

Notice: Array to string conversion in %s on line %d

Warning: dir(Array): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 4 --

Notice: Array to string conversion in %s on line %d

Warning: dir(Array): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 5 --

Notice: Array to string conversion in %s on line %d

Warning: dir(Array): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 6 --
bool(false)

-- Iteration 7 --
bool(false)

-- Iteration 8 --

Warning: dir(1): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 9 --
bool(false)

-- Iteration 10 --

Warning: dir(1): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 11 --
bool(false)

-- Iteration 12 --
bool(false)

-- Iteration 13 --
bool(false)

-- Iteration 14 --
bool(false)

-- Iteration 15 --
bool(false)

-- Iteration 16 --

Warning: dir(Resource id #%d): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 17 --

Warning: dir(Resource id #%d): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 18 --

Warning: dir(ClassAObject): failed to open dir: %s in %s on line %d
bool(false)
Done
--UEXPECTF--
*** Testing dir() : unexpected values for $directory argument ***

-- Iteration 1 --

Notice: Array to string conversion in %s on line %d

Warning: dir(Array): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 2 --

Notice: Array to string conversion in %s on line %d

Warning: dir(Array): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 3 --

Notice: Array to string conversion in %s on line %d

Warning: dir(Array): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 4 --

Notice: Array to string conversion in %s on line %d

Warning: dir(Array): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 5 --

Notice: Array to string conversion in %s on line %d

Warning: dir(Array): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 6 --
bool(false)

-- Iteration 7 --
bool(false)

-- Iteration 8 --

Warning: dir(1): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 9 --
bool(false)

-- Iteration 10 --

Warning: dir(1): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 11 --
bool(false)

-- Iteration 12 --
bool(false)

-- Iteration 13 --
bool(false)

-- Iteration 14 --
bool(false)

-- Iteration 15 --
bool(false)

-- Iteration 16 --

Warning: dir(Resource id #%d): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 17 --

Warning: dir(Resource id #%d): failed to open dir: %s in %s on line %d
bool(false)

-- Iteration 18 --

Warning: dir(ClassAObject): failed to open dir: %s in %s on line %d
bool(false)
Done