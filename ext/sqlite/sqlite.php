<?php
dl("sqlite.so");

debug_zval_dump(sqlite_libversion());
debug_zval_dump(sqlite_libencoding());

$s = sqlite_open("weztest.sqlite", 0666, $err);

debug_zval_dump($err);
debug_zval_dump($s);

$r = sqlite_query("create table foo (a INTEGER PRIMARY KEY, b INTEGER )", $s);
$r = sqlite_query("select * from sqlite_master", $s);
debug_zval_dump($r);
debug_zval_dump(sqlite_num_rows($r));
debug_zval_dump(sqlite_num_fields($r));

for ($j = 0; $j < sqlite_num_fields($r); $j++) {
	echo "Field $j is " . sqlite_field_name($r, $j) . "\n";
}

while ($row = sqlite_fetch_array($r, SQLITE_ASSOC)) {
	print_r($row);
}

sqlite_close($s);

?>
