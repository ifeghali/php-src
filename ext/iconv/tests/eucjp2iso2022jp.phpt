--TEST--
EUC-JP to ISO-2022-JP
--SKIPIF--
<?php /* include('skipif.inc'); */ ?>
--INI--
error_reporting=2039
--FILE--
<?php
/* include('test.inc'); */
/* charset=EUC-JP */

$str = "
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
���ܸ�ƥ����Ȥ�English Text
";

$str = iconv("EUC-JP", "ISO-2022-JP", $str);
$str = base64_encode($str);
echo $str."\n";

?>
--EXPECT--
ChskQkZ8S1w4bCVGJS0lOSVIJEgbKEJFbmdsaXNoIFRleHQKGyRCRnxLXDhsJUYlLSU5JUgkSBsoQkVuZ2xpc2ggVGV4dAobJEJGfEtcOGwlRiUtJTklSCRIGyhCRW5nbGlzaCBUZXh0ChskQkZ8S1w4bCVGJS0lOSVIJEgbKEJFbmdsaXNoIFRleHQKGyRCRnxLXDhsJUYlLSU5JUgkSBsoQkVuZ2xpc2ggVGV4dAobJEJGfEtcOGwlRiUtJTklSCRIGyhCRW5nbGlzaCBUZXh0ChskQkZ8S1w4bCVGJS0lOSVIJEgbKEJFbmdsaXNoIFRleHQKGyRCRnxLXDhsJUYlLSU5JUgkSBsoQkVuZ2xpc2ggVGV4dAobJEJGfEtcOGwlRiUtJTklSCRIGyhCRW5nbGlzaCBUZXh0ChskQkZ8S1w4bCVGJS0lOSVIJEgbKEJFbmdsaXNoIFRleHQKGyRCRnxLXDhsJUYlLSU5JUgkSBsoQkVuZ2xpc2ggVGV4dAobJEJGfEtcOGwlRiUtJTklSCRIGyhCRW5nbGlzaCBUZXh0ChskQkZ8S1w4bCVGJS0lOSVIJEgbKEJFbmdsaXNoIFRleHQKGyRCRnxLXDhsJUYlLSU5JUgkSBsoQkVuZ2xpc2ggVGV4dAobJEJGfEtcOGwlRiUtJTklSCRIGyhCRW5nbGlzaCBUZXh0ChskQkZ8S1w4bCVGJS0lOSVIJEgbKEJFbmdsaXNoIFRleHQKGyRCRnxLXDhsJUYlLSU5JUgkSBsoQkVuZ2xpc2ggVGV4dAobJEJGfEtcOGwlRiUtJTklSCRIGyhCRW5nbGlzaCBUZXh0ChskQkZ8S1w4bCVGJS0lOSVIJEgbKEJFbmdsaXNoIFRleHQKGyRCRnxLXDhsJUYlLSU5JUgkSBsoQkVuZ2xpc2ggVGV4dAobJEJGfEtcOGwlRiUtJTklSCRIGyhCRW5nbGlzaCBUZXh0ChskQkZ8S1w4bCVGJS0lOSVIJEgbKEJFbmdsaXNoIFRleHQKGyRCRnxLXDhsJUYlLSU5JUgkSBsoQkVuZ2xpc2ggVGV4dAobJEJGfEtcOGwlRiUtJTklSCRIGyhCRW5nbGlzaCBUZXh0ChskQkZ8S1w4bCVGJS0lOSVIJEgbKEJFbmdsaXNoIFRleHQKGyRCRnxLXDhsJUYlLSU5JUgkSBsoQkVuZ2xpc2ggVGV4dAobJEJGfEtcOGwlRiUtJTklSCRIGyhCRW5nbGlzaCBUZXh0ChskQkZ8S1w4bCVGJS0lOSVIJEgbKEJFbmdsaXNoIFRleHQKGyRCRnxLXDhsJUYlLSU5JUgkSBsoQkVuZ2xpc2ggVGV4dAobJEJGfEtcOGwlRiUtJTklSCRIGyhCRW5nbGlzaCBUZXh0ChskQkZ8S1w4bCVGJS0lOSVIJEgbKEJFbmdsaXNoIFRleHQKGyRCRnxLXDhsJUYlLSU5JUgkSBsoQkVuZ2xpc2ggVGV4dAobJEJGfEtcOGwlRiUtJTklSCRIGyhCRW5nbGlzaCBUZXh0ChskQkZ8S1w4bCVGJS0lOSVIJEgbKEJFbmdsaXNoIFRleHQKGyRCRnxLXDhsJUYlLSU5JUgkSBsoQkVuZ2xpc2ggVGV4dAobJEJGfEtcOGwlRiUtJTklSCRIGyhCRW5nbGlzaCBUZXh0Cg==

