--TEST--
mb_substr()
--SKIPIF--
<?php include('skipif.inc'); ?>
--POST--
--GET--
--FILE--
<?php include('012.inc'); ?>
--EXPECT--
1: ���ܸ�Ǥ���EUC-
2: 0123����ʸ��������ܸ�Ǥ���EUC-JP��ȤäƤ��ޤ������ܸ�����ݽ�����
3 OK
4 OK: 0123����ʸ�����


