--TEST--
SimpleXML [profile]: Accessing an aliased namespaced element
--SKIPIF--
<?php if (!extension_loaded("simplexml")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL & ~E_NOTICE);
$root = simplexml_load_string(b'<?xml version="1.0"?>
<root xmlns:reserved="reserved-ns">
 <reserved:child>Hello</reserved:child>
</root>
');

echo $root->children('reserved')->child;
echo "\n---Done---\n";
?>
--EXPECT--
---Done--- 
