@ECHO OFF

REM ----------------------------------------------------------------------
REM PHP version 4.0                                                     
REM ----------------------------------------------------------------------
REM Copyright (c) 1997-2002 The PHP Group                               
REM ----------------------------------------------------------------------
REM  This source file is subject to version 2.02 of the PHP license,    
REM  that is bundled with this package in the file LICENSE, and is      
REM  available at through the world-wide-web at                         
REM  http://www.php.net/license/2_02.txt.                               
REM  If you did not receive a copy of the PHP license and are unable to 
REM  obtain it through the world-wide-web, please send a note to        
REM  license@php.net so we can mail you a copy immediately.             
REM ----------------------------------------------------------------------
REM  Authors:     Alexander Merz (alexmerz@php.net)                         
REM ----------------------------------------------------------------------
REM
REM  $Id: pear.bat,v 1.9 2002/07/26 10:48:20 cox Exp $

REM change this lines to match the paths of your system
REM -------------------

set PHP_BIN=c:\php\php.exe
set PEAR_PATH=c:\php\pear

%PHP_BIN% -C -d include_path=%PEAR_PATH% -f %PEAR_PATH%\scripts\pear.in -- %1 %2 %3 %4 %5 %6 %7 %8 %9
@ECHO ON
