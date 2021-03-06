--TEST--
Check for exif_read_data, JPEG with IFD and GPS data in Motorola byte-order.
--CREDIT--
Eric Stewart <ericleestewart@gmail.com>
--SKIPIF--
<?php if (!extension_loaded('exif')) print 'skip exif extension not available';?>
--INI--
output_handler=
zlib.output_compression=0
--FILE--
<?php
var_dump(exif_read_data(dirname(__FILE__).'/image013.jpg'));
?>
--EXPECTF--
array(25) {
  [u"FileName"]=>
  unicode(12) "image013.jpg"
  [u"FileDateTime"]=>
  int(%d)
  [u"FileSize"]=>
  int(%d)
  [u"FileType"]=>
  int(2)
  [u"MimeType"]=>
  unicode(10) "image/jpeg"
  [u"SectionsFound"]=>
  unicode(18) "ANY_TAG, IFD0, GPS"
  [u"COMPUTED"]=>
  array(8) {
    [u"html"]=>
    unicode(20) "width="1" height="1""
    [u"Height"]=>
    int(1)
    [u"Width"]=>
    int(1)
    [u"IsColor"]=>
    int(1)
    [u"ByteOrderMotorola"]=>
    int(1)
    [u"Copyright"]=>
    unicode(24) "Eric Stewart, Hex Editor"
    [u"Copyright.Photographer"]=>
    unicode(12) "Eric Stewart"
    [u"Copyright.Editor"]=>
    unicode(10) "Hex Editor"
  }
  [u"ImageDescription"]=>
  unicode(15) "My description."
  [u"Make"]=>
  unicode(11) "OpenShutter"
  [u"Model"]=>
  unicode(8) "OS 1.0.0"
  [u"XResolution"]=>
  unicode(4) "72/1"
  [u"YResolution"]=>
  unicode(4) "72/1"
  [u"ResolutionUnit"]=>
  int(2)
  [u"DateTime"]=>
  unicode(%d) "%s"
  [u"Artist"]=>
  unicode(12) "Eric Stewart"
  [u"Copyright"]=>
  unicode(12) "Eric Stewart"
  [u"GPS_IFD_Pointer"]=>
  int(246)
  [u"GPSVersion"]=>
  unicode(4) "  "
  [u"GPSLatitudeRef"]=>
  unicode(1) "N"
  [u"GPSLatitude"]=>
  array(3) {
    [0]=>
    unicode(4) "33/1"
    [1]=>
    unicode(4) "37/1"
    [2]=>
    unicode(3) "0/1"
  }
  [u"GPSLongitudeRef"]=>
  unicode(1) "W"
  [u"GPSLongitude"]=>
  array(3) {
    [0]=>
    unicode(4) "84/1"
    [1]=>
    unicode(3) "7/1"
    [2]=>
    unicode(3) "0/1"
  }
  [u"GPSAltitudeRef"]=>
  unicode(1) " "
  [u"GPSAltitude"]=>
  unicode(5) "295/1"
  [u"GPSTimeStamp"]=>
  array(3) {
    [0]=>
    unicode(3) "1/1"
    [1]=>
    unicode(4) "47/1"
    [2]=>
    unicode(4) "53/1"
  }
}

