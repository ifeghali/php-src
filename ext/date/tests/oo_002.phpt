--TEST--
date OO cloning
--INI--
date.timezone=Europe/Berlin
--FILE--
<?php
class _d extends DateTime {}
class _t extends DateTimeZone {}
$d = new _d("1pm Aug 1 GMT");
var_dump($d->format(DateTime::RFC822));
$c = clone $d;
var_dump($c->format(DateTime::RFC822));
$d->modify("1 hour after");
$c->modify("1 second ago");
var_dump($d->format(DateTime::RFC822));
var_dump($c->format(DateTime::RFC822));
$t = new _t("Asia/Tokyo");
var_dump($t->getName());
$c = clone $t;
var_dump($c->getName());
?>
--EXPECTF--
string(29) "Tue, 01 Aug %d 13:00:00 +0000"
string(29) "Tue, 01 Aug %d 13:00:00 +0000"
string(29) "Tue, 01 Aug %d 14:00:00 +0000"
string(29) "Tue, 01 Aug %d 12:59:59 +0000"
string(10) "Asia/Tokyo"
string(10) "Asia/Tokyo"
--UEXPECTF--
unicode(29) "Tue, 01 Aug %d 13:00:00 +0000"
unicode(29) "Tue, 01 Aug %d 13:00:00 +0000"
unicode(29) "Tue, 01 Aug %d 14:00:00 +0000"
unicode(29) "Tue, 01 Aug %d 12:59:59 +0000"
unicode(10) "Asia/Tokyo"
unicode(10) "Asia/Tokyo"
