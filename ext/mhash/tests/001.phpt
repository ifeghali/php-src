--TEST--
mhash() test
--SKIPIF--
<?php
	include "skip.inc";
?>
--FILE--
<?php

$supported_hash_al = array(
"MHASH_MD5",
"MHASH_SHA1",
"MHASH_HAVAL256",
"MHASH_HAVAL192",
"MHASH_HAVAL224",
"MHASH_HAVAL160",
"MHASH_RIPEMD160",
"MHASH_GOST",
"MHASH_TIGER",
"MHASH_CRC32",
"MHASH_CRC32B"
);

	$data = "This is the test of the mhash extension...";

	foreach ($supported_hash_al as $hash) {
		echo $hash . "\n";
		var_dump(mhash(constant($hash), $data));
		echo "\n";
	}
?>
--EXPECT--
MHASH_MD5
string(16) "-�ۑ�N����S*̓j"

MHASH_SHA1
string(20) "/�A�Z����I{�;�ہ*}�"

MHASH_HAVAL256
string(32) "�U���d'5�ǐ�ƕ���;���� �u����"

MHASH_HAVAL192
string(24) "L�7�H0	*��p�Ɉ����"

MHASH_HAVAL224
string(28) "SbхgR�,�����r����^�&�&K��"

MHASH_HAVAL160
string(20) "Ƴo�uWi����"q�{��"

MHASH_RIPEMD160
string(20) "lGCZ��YķƯF4�>XX="

MHASH_GOST
string(32) "
%Rν�|��QG�U�C)5��,���-�"

MHASH_TIGER
string(24) "�:�y���둮�� ~g9\�T0�"

MHASH_CRC32
string(4) "��"

MHASH_CRC32B
string(4) "��Z�"

