--TEST--
Phar: test a zip archive created by openoffice
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
<?php if (!extension_loaded("spl")) die("skip SPL not available"); ?>
--FILE--
<?php
$a = new PharData(dirname(__FILE__) . '/files/odt.odt');
foreach (new RecursiveIteratorIterator($a, RecursiveIteratorIterator::LEAVES_ONLY) as $b) {
	if ($b->isDir()) {
		echo "dir " . $b->getPathName() . "\n";
	} else {
		echo $b->getPathName() . "\n";
	}
}
?>
===DONE===
--EXPECTF--
phar://%sodt.odt%cConfigurations2%caccelerator%ccurrent.xml
phar://%sodt.odt%cMETA-INF%cmanifest.xml
phar://%sodt.odt%cThumbnails%cthumbnail.png
phar://%sodt.odt%ccontent.xml
phar://%sodt.odt%cmeta.xml
phar://%sodt.odt%cmimetype
phar://%sodt.odt%csettings.xml
phar://%sodt.odt%cstyles.xml
===DONE===