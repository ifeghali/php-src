<?php
@unlink(dirname(__FILE__) . '/frontcontroller4.phar.zip');
$a = new Phar(dirname(__FILE__) . '/frontcontroller4.phar.zip');
$a['a.php'] = 'hio';
$a['a.jpg'] = 'hio';
$a['a.phps'] = '<?php function hio(){}';
$a->setStub('<?php
Phar::webPhar("whatever", "index.php", null, array(), array("/hi" => true));
echo "oops did not run\n";
var_dump($_ENV, $_SERVER);
__HALT_COMPILER();');
?>