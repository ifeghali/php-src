--TEST--
mysqli_stmt_execute()
--SKIPIF--
<?php require_once('skipif.inc'); ?>
<?php require_once('skipifemb.inc'); ?>
--FILE--
<?php
	include "connect.inc";

	$tmp    = NULL;
	$link   = NULL;

	if (!is_null($tmp = @mysqli_stmt_execute()))
		printf("[001] Expecting NULL, got %s/%s\n", gettype($tmp), $tmp);

	if (!is_null($tmp = @mysqli_stmt_execute($link)))
		printf("[002] Expecting NULL, got %s/%s\n", gettype($tmp), $tmp);

	require('table.inc');

	if (!$stmt = mysqli_stmt_init($link))
		printf("[003] [%d] %s\n", mysqli_errno($link), mysqli_error($link));

	// stmt object status test
	if (NULL !== ($tmp = mysqli_stmt_execute($stmt)))
		printf("[004] Expecting NULL, got %s/%s\n", gettype($tmp), $tmp);

	if (mysqli_stmt_prepare($stmt, "SELECT i_do_not_exist_believe_me FROM test ORDER BY id"))
		printf("[005] Statement should have failed!\n");

	// stmt object status test
	if (NULL !== ($tmp = mysqli_stmt_execute($stmt)))
		printf("[006] Expecting NULL, got %s/%s\n", gettype($tmp), $tmp);

	if (!mysqli_stmt_prepare($stmt, "SELECT id FROM test ORDER BY id LIMIT 1"))
		printf("[007] [%d] %s\n", mysqli_stmt_errno($stmt), mysqli_stmt_error($stmt));

	if (true !== ($tmp = mysqli_stmt_execute($stmt)))
		printf("[008] Expecting boolean/true, got %s/%s. [%d] %s\n",
			gettype($tmp), $tmp, mysqli_stmt_errno($stmt), mysqli_stmt_error($stmt));

	if (!mysqli_stmt_prepare($stmt, "INSERT INTO test(id, label) VALUES (?, ?)"))
		printf("[009] [%d] %s\n", mysqli_stmt_execute($stmt), mysqli_stmt_execute($stmt));

	// no input variables bound
	if (false !== ($tmp = mysqli_stmt_execute($stmt)))
		printf("[010] Expecting boolean/false, got %s/%s\n", gettype($tmp), $tmp);

	$id = 100;
	$label = "z";
	if (!mysqli_stmt_bind_param($stmt, "is", $id, $label))
		printf("[011] [%d] %s\n", mysqli_stmt_errno($stmt), mysqli_stmt_error($stmt));

	if (true !== ($tmp = mysqli_stmt_execute($stmt)))
		printf("[012] [%d] %s\n", mysqli_stmt_errno($stmt), mysqli_stmt_error($stmt));

	mysqli_kill($link, mysqli_thread_id($link));

	if (false !== ($tmp = mysqli_stmt_execute($stmt)))
		printf("[014] Expecting boolean/false, got %s/%s\n", gettype($tmp), $tmp);

	mysqli_stmt_close($stmt);

	if (NULL !== ($tmp = mysqli_stmt_execute($stmt)))
		printf("[015] Expecting NULL, got %s/%s\n", gettype($tmp), $tmp);

	mysqli_close($link);
	print "done!";
?>
--EXPECTF--
Warning: mysqli_stmt_execute(): invalid object or resource mysqli_stmt
 in %s on line %d

Warning: mysqli_stmt_execute(): invalid object or resource mysqli_stmt
 in %s on line %d

Warning: mysqli_stmt_execute(): Couldn't fetch mysqli_stmt in %s on line %d
done!