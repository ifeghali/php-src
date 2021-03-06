--TEST--
Bug #39322 (proc_terminate() loosing process resource)
--SKIPIF--
<?php
if (!is_executable('/bin/sleep')) echo 'skip sleep not found';
?>
--FILE--
<?php
$descriptors = array(
    0 => array('pipe', 'r'),
    1 => array('pipe', 'w'),
    2 => array('pipe', 'w'));

$pipes = array();

$process = proc_open('/bin/sleep 120', $descriptors, $pipes);

proc_terminate($process);
sleep(1); // wait a bit to let the process finish
var_dump(proc_get_status($process));

echo "Done!\n";

?>
--EXPECTF--
array(8) {
  [u"command"]=>
  unicode(14) "/bin/sleep 120"
  [u"pid"]=>
  int(%d)
  [u"running"]=>
  bool(false)
  [u"signaled"]=>
  bool(true)
  [u"stopped"]=>
  bool(false)
  [u"exitcode"]=>
  int(-1)
  [u"termsig"]=>
  int(15)
  [u"stopsig"]=>
  int(0)
}
Done!
