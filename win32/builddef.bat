rem Generate phpts.def file, which exports symbols from our dll that
rem are present in some of the libraries which are compiled statically
rem into PHP
rem $Id:$
@echo off
type ..\ext\sqlite\php_sqlite.def
