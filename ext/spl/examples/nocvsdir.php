<?php

/** @file   nocvsdir.php
 * @brief   Program Dir without CVS subdirs
 * @ingroup Examples
 * @author  Marcus Boerger
 * @date    2003 - 2005
 *
 * Usage: php nocvsdir.php \<path\>
 *
 * Simply specify the path to tree with parameter \<path\>.
 */

if ($argc < 2) {
	echo <<<EOF
Usage: php ${_SERVER['PHP_SELF']} <path>

Show the directory and all it's contents without any CVS directory in <path>.

<path> The directory for which to generate the directory.


EOF;
	exit(1);
}

if (!class_exists("RecursiveFilterIterator")) require_once("recursivefilteriterator.inc");

class NoCvsDirectory extends RecursiveFilterIterator
{
	function accept()
	{
		return $this->getInnerIterator()->getFilename() != 'CVS';
	}
}

$it = new RecursiveIteratorIterator(new NoCvsDirectory(new RecursiveDirectoryIterator($argv[1])));

foreach($it as $pathname => $file)
{
	echo $pathname."\n";
}

?>