<?php

if (!class_exists('DirectoryTreeIterator', false)) {
	require_once(dirname(__FILE__). '/directorytreeiterator.inc');
}

/** tree view example
 *
 * Usage: php directorytree.php <path> [<start> [<count>]]
 *
 * Simply specify the path to tree with parameter <path>.
 *
 * (c) Marcus Boerger, 2003
 */

if ($argc < 2) {
	echo <<<EOF
Usage: php ${_SERVER['PHP_SELF']} <path>

Displays a graphical directory tree for the given <path>.

<path> The directory for which to generate the directory tree graph.


EOF;
	exit(1);
}

$length = $argc > 3 ? $argv[3] : -1;

echo $argv[1]."\n";
foreach(new LimitIterator(new DirectoryTreeIterator($argv[1]), @$argv[2], $length) as $key=>$file) {
//foreach(new DirectoryTreeIterator($argv[1]) as $file) {
	echo $file . "\n";
}

?>