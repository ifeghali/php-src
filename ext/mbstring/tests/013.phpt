--TEST--
mb_strcut()
--SKIPIF--
<?php include('skipif.inc'); ?>
--POST--
--GET--
--FILE--
<?php include('013.inc'); ?>
--EXPECT--
��ʸ
0123����ʸ��������ܸ�Ǥ���EUC-JP��ȤäƤ��ޤ������ܸ�����ݽ�����
OK
OK: 0123����ʸ


