--TEST--
mysqli_enable_reads_from_master()
--SKIPIF--
<?php require_once('skipif.inc'); ?>
<?php require_once('skipifemb.inc'); ?>
<?PHP
if (!function_exists('mysqli_enable_reads_from_master')) {
	die("skip function mysqli_enable_reads_from_master() not available\n");
}
?>
--FILE--
<?php
	include "connect.inc";

	$tmp    = NULL;
	$link   = NULL;

	if (NULL !== ($tmp = @mysqli_enable_reads_from_master()))
		printf("[001] Expecting NULL/NULL, got %s/%s\n", gettype($tmp), $tmp);

	if (NULL !== ($tmp = @mysqli_enable_reads_from_master($link)))
		printf("[002] Expecting NULL/NULL, got %s/%s\n", gettype($tmp), $tmp);

	if (!$link = mysqli_connect($host, $user, $passwd, $db, $port, $socket)) {
		printf("[003] Cannot connect to the server using host=%s, user=%s, passwd=***, dbname=%s, port=%s, socket=%s\n",
			$host, $user, $db, $port, $socket);
	}

	if (!is_bool($tmp = mysqli_enable_reads_from_master($link)))
		printf("[004] Expecting boolean/[true|false] value, got %s/%s\n", gettype($tmp), $tmp);

	mysqli_close($link);

	if (NULL !== ($tmp = mysqli_enable_reads_from_master($link)))
		printf("[005] Expecting NULL, got %s/%s\n", gettype($tmp), $tmp);

	print "done!";
?>
--EXPECTF--
Warning: mysqli_enable_reads_from_master(): Couldn't fetch mysqli in %s on line %d
done!