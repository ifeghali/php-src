--TEST--
Gettext basic test
--SKIPIF--
<?php
	error_reporting(0);
	if (!extension_loaded("gettext")) {
		die("skip\n");
	}
	if (!setlocale(LC_ALL, 'fi_FI')) {
		die("skip fi_FI locale not supported.");
	}
?>
--FILE--
<?php // $Id: gettext_basic.phpt,v 1.4 2008/03/22 14:58:29 felipe Exp $

chdir(dirname(__FILE__));
putenv("LANGUAGE=fi");
setlocale(LC_ALL, 'fi_FI');
bindtextdomain ("messages", "./locale");
textdomain ("messages");
echo gettext("Basic test"), "\n";
echo _("Basic test"), "\n";

?>
--EXPECTF--
Deprecated: setlocale(): deprecated in Unicode mode, please use ICU locale functions in %s on line %d
Perustesti
Perustesti
