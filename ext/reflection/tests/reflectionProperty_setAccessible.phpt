--TEST--
Test ReflectionProperty::setAccessible().
--FILE--
<?php

class TestClass {
    public $pub;
    public $pub2 = 5;
    static public $stat = "static property";
    protected $prot = 4;
    private $priv = "keepOut";
}

class AnotherClass {
}

$instance = new TestClass();

echo "\nProtected property:\n";
$propInfo = new ReflectionProperty('TestClass', 'prot');
try {
    var_dump($propInfo->getValue($instance));
}
catch(Exception $exc) {
    echo $exc->getMessage(), "\n";
}

$propInfo->setAccessible(true);
var_dump($propInfo->getValue($instance));

$propInfo->setAccessible(false);
try {
    var_dump($propInfo->getValue($instance));
}
catch(Exception $exc) {
    echo $exc->getMessage(), "\n";
}
?>
--EXPECTF--

Protected property:
Cannot access non-public member TestClass::prot
int(4)
Cannot access non-public member TestClass::prot
