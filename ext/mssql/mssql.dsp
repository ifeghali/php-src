# Microsoft Developer Studio Project File - Name="mssql" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=mssql - Win32 MSSQL 70 Release_TS
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mssql.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mssql.mak" CFG="mssql - Win32 MSSQL 70 Release_TS"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mssql - Win32 MSSQL 65 Release_TS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mssql - Win32 MSSQL 65 Debug_TS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mssql - Win32 MSSQL 70 Release_TS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mssql - Win32 MSSQL 70 Debug_TS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mssql - Win32 MSSQL 65 Release_TS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MSSQL_65_Release_TS"
# PROP BASE Intermediate_Dir "MSSQL_65_Release_TS"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MSSQL_65_Release_TS"
# PROP Intermediate_Dir "MSSQL_65_Release_TS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\.." /I "..\..\..\Zend" /I "..\..\..\bindlib_w32" /I "..\..\..\TSRM" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSSQL_EXPORTS" /D "COMPILE_DL_MSSQL" /D "DBNTWIN32" /D ZTS=1 /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\.." /I "..\..\main" /I "..\..\Zend" /I "..\..\..\bindlib_w32" /I "..\..\TSRM" /D MSSQL65=1 /D ZEND_DEBUG=0 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSSQL_EXPORTS" /D "COMPILE_DL_MSSQL" /D "DBNTWIN32" /D ZTS=1 /D "ZEND_WIN32" /D "PHP_WIN32" /D HAVE_MSSQL=1 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x406 /d "NDEBUG"
# ADD RSC /l 0x406 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib php4ts.lib ntwdblib.lib /nologo /dll /machine:I386 /out:"MSSQL_65_Release/php_mssql.dll" /libpath:"..\..\Release_TS"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib php4ts.lib ntwdblib.lib /nologo /dll /machine:I386 /out:"..\..\Release_TS/php_mssql65.dll" /libpath:"..\..\Release_TS" /libpath:"..\..\..\php_build\mssql-65\lib"

!ELSEIF  "$(CFG)" == "mssql - Win32 MSSQL 65 Debug_TS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MSSQL_65_Debug_TS"
# PROP BASE Intermediate_Dir "MSSQL_65_Debug_TS"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MSSQL_65_Debug_TS"
# PROP Intermediate_Dir "MSSQL_65_Debug_TS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\.." /I "..\..\Zend" /I "..\..\..\bindlib_w32" /I "..\..\TSRM" /I "mssql-70" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSSQL_EXPORTS" /D "COMPILE_DL_MSSQL" /D "DBNTWIN32" /D ZTS=1 /D MSSQL65=1 /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /O2 /I "..\.." /I "..\..\main" /I "..\..\Zend" /I "..\..\..\bindlib_w32" /I "..\..\TSRM" /D MSSQL65=1 /D ZEND_DEBUG=1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSSQL_EXPORTS" /D "COMPILE_DL_MSSQL" /D "DBNTWIN32" /D ZTS=1 /D "ZEND_WIN32" /D "PHP_WIN32" /D HAVE_MSSQL=1 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x406 /d "NDEBUG"
# ADD RSC /l 0x406 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib php4ts.lib ntwdblib.lib /nologo /dll /machine:I386 /out:"MSSQL_65_Release/php_mssql.dll" /libpath:"..\..\Release_TS" /libpath:"mssql-70"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib php4ts_debug.lib ntwdblib.lib /nologo /dll /machine:I386 /out:"..\..\Debug_TS/php_mssql65.dll" /libpath:"..\..\Debug_TS" /libpath:"..\..\..\php_build\mssql-65\lib"

!ELSEIF  "$(CFG)" == "mssql - Win32 MSSQL 70 Release_TS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MSSQL_70_Release_TS"
# PROP BASE Intermediate_Dir "MSSQL_70_Release_TS"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MSSQL_70_Release_TS"
# PROP Intermediate_Dir "MSSQL_70_Release_TS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\.." /I "..\..\..\Zend" /I "..\..\..\bindlib_w32" /I "..\..\..\TSRM" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSSQL_EXPORTS" /D "COMPILE_DL_MSSQL" /D "DBNTWIN32" /D ZTS=1 /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\.." /I "..\..\main" /I "..\..\Zend" /I "..\..\..\bindlib_w32" /I "..\..\TSRM" /D MSSQL70=1 /D ZEND_DEBUG=0 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSSQL_EXPORTS" /D "COMPILE_DL_MSSQL" /D "DBNTWIN32" /D ZTS=1 /D "ZEND_WIN32" /D "PHP_WIN32" /D HAVE_MSSQL=1 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x406 /d "NDEBUG"
# ADD RSC /l 0x406 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib php4ts.lib ntwdblib.lib /nologo /dll /machine:I386 /out:"MSSQL_65_Release/php_mssql.dll" /libpath:"..\..\Release_TS"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib php4ts.lib ntwdblib.lib /nologo /dll /machine:I386 /out:"..\..\Release_TS/php_mssql70.dll" /libpath:"..\..\Release_TS" /libpath:"..\..\..\php_build\mssql-70\lib"

!ELSEIF  "$(CFG)" == "mssql - Win32 MSSQL 70 Debug_TS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MSSQL_70_Debug_TS"
# PROP BASE Intermediate_Dir "MSSQL_70_Debug_TS"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MSSQL_70_Debug_TS"
# PROP Intermediate_Dir "MSSQL_70_Debug_TS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\.." /I "..\..\Zend" /I "..\..\..\bindlib_w32" /I "..\..\TSRM" /I "mssql-70" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSSQL_EXPORTS" /D "COMPILE_DL_MSSQL" /D "DBNTWIN32" /D ZTS=1 /D MSSQL70=1 /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /O2 /I "..\.." /I "..\..\main" /I "..\..\Zend" /I "..\..\..\bindlib_w32" /I "..\..\TSRM" /D MSSQL70=1 /D ZEND_DEBUG=1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSSQL_EXPORTS" /D "COMPILE_DL_MSSQL" /D "DBNTWIN32" /D ZTS=1 /D "ZEND_WIN32" /D "PHP_WIN32" /D HAVE_MSSQL=1 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x406 /d "NDEBUG"
# ADD RSC /l 0x406 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib php4ts.lib ntwdblib.lib /nologo /dll /machine:I386 /out:"MSSQL_65_Release/php_mssql.dll" /libpath:"..\..\Release_TS" /libpath:"mssql-70"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib php4ts_debug.lib ntwdblib.lib /nologo /dll /machine:I386 /out:"..\..\Debug_TS/php_mssql70.dll" /libpath:"..\..\Debug_TS" /libpath:"..\..\..\php_build\mssql-70\lib"

!ENDIF 

# Begin Target

# Name "mssql - Win32 MSSQL 65 Release_TS"
# Name "mssql - Win32 MSSQL 65 Debug_TS"
# Name "mssql - Win32 MSSQL 70 Release_TS"
# Name "mssql - Win32 MSSQL 70 Debug_TS"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\php_mssql.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\php_mssql.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
