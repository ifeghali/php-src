--TEST--
Phar front controller mime type override, Phar::PHPS
--SKIPIF--
<?php if (!extension_loaded("phar")) die("skip"); ?>
--ENV--
SCRIPT_NAME=/frontcontroller15.php/a.php
REQUEST_URI=/frontcontroller15.php/a.php
--FILE_EXTERNAL--
frontcontroller8.phar
--EXPECTHEADERS--
Content-type: text/html
--EXPECT--
<code><span style="color: #000000">
<span style="color: #0000BB">&lt;?php&nbsp;</span><span style="color: #007700">function&nbsp;</span><span style="color: #0000BB">hio</span><span style="color: #007700">(){}</span>
</span>
</code>

