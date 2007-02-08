--TEST--
Test getParent()
--SKIPIF--
<?php if (!extension_loaded("tidy")) print "skip"; ?>
--FILE--
<?php
$x = tidy_parse_string("<body><div>Content</div></body>");
var_dump($x->body()->child[0]->name);
var_dump($x->body()->child[0]->getParent()->name);
?>
--EXPECT--
string(3) "div"
string(4) "body"
--UEXPECT--
unicode(3) "div"
unicode(4) "body"
