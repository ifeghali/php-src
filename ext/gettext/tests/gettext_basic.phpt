--TEST--
Gettext basic test
--SKIPIF--
<?php if (!extension_loaded("gettext")) print "skip"; ?>
--FILE--
<?php // $Id: with_files.phpt,v 1.1 2003/05/17 14:27:07 sterling Exp $

chdir(dirname(__FILE__));
setlocale(LC_ALL, 'fi_FI');
bindtextdomain ("messages", "./locale");
textdomain ("messages");
echo gettext("Basic test"), "\n";
echo _("Basic test"), "\n";

?>
--EXPECT--
Perustesti
Perustesti
