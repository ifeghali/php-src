--TEST--
mb_output_handler() (EUC-JP)
--SKIPIF--
<?php include('skipif.inc'); ?>
--POST--
--GET--
--FILE--
<?php include('007.inc'); ?>
--EXPECT--
string(73) "�ƥ��������ܸ�ʸ���󡣤��Υ⥸�塼���PHP�˥ޥ���Х��ȴؿ����󶡤��ޤ���"
