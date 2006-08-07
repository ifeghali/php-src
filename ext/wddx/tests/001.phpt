--TEST--
wddx unserialization test
--SKIPIF--
<?php if (!extension_loaded("wddx")) print "skip"; ?>
--FILE--
<?php
	$path = dirname(__FILE__);
	var_dump(wddx_unserialize(file_get_contents("{$path}/wddx.xml")));
?>
--EXPECT--
array(11) {
  ["aNull"]=>
  NULL
  ["aString"]=>
  string(8) "a string"
  ["aNumber"]=>
  float(-12.456)
  ["aDateTime"]=>
  int(897625932)
  ["aDateTime2"]=>
  int(329632332)
  ["aDateTime3"]=>
  string(22) "2040-06-12T04:32:12+00"
  ["aBoolean"]=>
  bool(true)
  ["anArray"]=>
  array(2) {
    [0]=>
    int(10)
    [1]=>
    string(14) "second element"
  }
  ["aBinary"]=>
  string(11) "binary data"
  ["anObject"]=>
  array(2) {
    ["s"]=>
    string(8) "a string"
    ["n"]=>
    float(-12.456)
  }
  ["aRecordset"]=>
  array(2) {
    ["NAME"]=>
    array(2) {
      [0]=>
      string(8) "John Doe"
      [1]=>
      string(8) "Jane Doe"
    }
    ["AGE"]=>
    array(2) {
      [0]=>
      int(34)
      [1]=>
      int(31)
    }
  }
}
--UEXPECT--
array(11) {
  [u"aNull"]=>
  NULL
  [u"aString"]=>
  string(8) "a string"
  [u"aNumber"]=>
  float(-12.456)
  [u"aDateTime"]=>
  int(897625932)
  [u"aDateTime2"]=>
  int(329632332)
  [u"aDateTime3"]=>
  string(22) "2040-06-12T04:32:12+00"
  [u"aBoolean"]=>
  bool(true)
  [u"anArray"]=>
  array(2) {
    [0]=>
    int(10)
    [1]=>
    string(14) "second element"
  }
  [u"aBinary"]=>
  string(11) "binary data"
  [u"anObject"]=>
  array(2) {
    [u"s"]=>
    string(8) "a string"
    [u"n"]=>
    float(-12.456)
  }
  [u"aRecordset"]=>
  array(2) {
    [u"NAME"]=>
    array(2) {
      [0]=>
      string(8) "John Doe"
      [1]=>
      string(8) "Jane Doe"
    }
    [u"AGE"]=>
    array(2) {
      [0]=>
      int(34)
      [1]=>
      int(31)
    }
  }
}
