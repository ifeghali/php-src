--TEST--
mb_strimwidth()
--SKIPIF--
<?php include('skipif.inc'); ?>
--POST--
--GET--
--FILE--
<?php include('014.inc'); ?>
--EXPECT--
1: 0123����ʸ��...
2: 0123����ʸ��������ܸ�Ǥ���EUC-JP��ȤäƤ��ޤ������ܸ�����ݽ�����
3: ��EUC-JP��ȤäƤ��ޤ������ܸ�����ݽ�����
ERR: Warning
4 OK
ERR: Warning
5 OK
ERR: Warning
6 OK


