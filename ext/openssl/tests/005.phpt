--TEST--
openSSL: read public key from x.509 resource
--SKIPIF--
<?php 
if (!extension_loaded("openssl")) die("skip"); 
?>
--FILE--
<?php 
$dir = dirname(__FILE__);
$file_pub = $dir . '/bug37820cert.pem';
$file_key = $dir . '/bug37820key.pem';

$priv_key = file_get_contents($file_key);
$priv_key_id = openssl_get_privatekey($priv_key);

$x509 = openssl_x509_read(file_get_contents($file_pub));

$pub_key_id = openssl_get_publickey($x509);
$data = "some custom data";
if (!openssl_sign($data, $signature, $priv_key_id, OPENSSL_ALGO_MD5)) {
	echo "openssl_sign failed.";
}

$ok = openssl_verify($data, $signature, $pub_key_id, OPENSSL_ALGO_MD5);
if ($ok == 1) {
   echo "Ok";
} elseif ($ok == 0) {
   echo "openssl_verify failed.";
}


?>
--EXPECTF--
Ok