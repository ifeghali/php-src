--TEST--
iconv test
--FILE--
<?php

$test = "Stig S�ther Bakken";
print "$test\n";
printf("%s\n", iconv("iso-8859-1", "utf-8", $test));

?>
--EXPECT--
Stig S�ther Bakken
Stig Sæther Bakken
