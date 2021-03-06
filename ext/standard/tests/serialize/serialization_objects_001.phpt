--TEST--
Test serialize() & unserialize() functions: objects
--FILE--
<?php 
/* Prototype  : proto string serialize(mixed variable)
 * Description: Returns a string representation of variable (which can later be unserialized) 
 * Source code: ext/standard/var.c
 * Alias to functions: 
 */
/* Prototype  : proto mixed unserialize(string variable_representation)
 * Description: Takes a string representation of variable and recreates it 
 * Source code: ext/standard/var.c
 * Alias to functions: 
 */

echo "\n--- Testing objects ---\n";

class members 
{
  private $var_private = 10;
  protected $var_protected = "string";
  public $var_public = array(-100.123, "string", TRUE);
}

$members_obj = new members();
var_dump( $members_obj );
$serialize_data = serialize( $members_obj );
var_dump( $serialize_data );
$members_obj = unserialize( $serialize_data );
var_dump( $members_obj );

echo "\n--- testing reference to an obj ---\n";
$ref_members_obj = &$members_obj;
$serialize_data = serialize( $ref_members_obj );
var_dump( $serialize_data );
$ref_members_obj = unserialize( $serialize_data );
var_dump( $ref_members_obj );

echo "\nDone";
?>
--EXPECTF--

--- Testing objects ---
object(members)#%d (3) {
  [u"var_private":u"members":private]=>
  int(10)
  [u"var_protected":protected]=>
  unicode(6) "string"
  [u"var_public"]=>
  array(3) {
    [0]=>
    float(-100.123)
    [1]=>
    unicode(6) "string"
    [2]=>
    bool(true)
  }
}
unicode(195) "O:7:"members":3:{U:20:" members var_private";i:10;U:16:" * var_protected";U:6:"string";U:10:"var_public";a:3:{i:0;d:-100.1230000000000046611603465862572193145751953125;i:1;U:6:"string";i:2;b:1;}}"
object(members)#%d (3) {
  [u"var_private":u"members":private]=>
  int(10)
  [u"var_protected":protected]=>
  unicode(6) "string"
  [u"var_public"]=>
  array(3) {
    [0]=>
    float(-100.123)
    [1]=>
    unicode(6) "string"
    [2]=>
    bool(true)
  }
}

--- testing reference to an obj ---
unicode(195) "O:7:"members":3:{U:20:" members var_private";i:10;U:16:" * var_protected";U:6:"string";U:10:"var_public";a:3:{i:0;d:-100.1230000000000046611603465862572193145751953125;i:1;U:6:"string";i:2;b:1;}}"
object(members)#%d (3) {
  [u"var_private":u"members":private]=>
  int(10)
  [u"var_protected":protected]=>
  unicode(6) "string"
  [u"var_public"]=>
  array(3) {
    [0]=>
    float(-100.123)
    [1]=>
    unicode(6) "string"
    [2]=>
    bool(true)
  }
}

Done
