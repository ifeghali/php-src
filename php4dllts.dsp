# Microsoft Developer Studio Project File - Name="php4dllts" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=php4dllts - Win32 Debug_TS
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "php4dllts.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "php4dllts.mak" CFG="php4dllts - Win32 Debug_TS"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "php4dllts - Win32 Debug_TS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "php4dllts - Win32 Release_TS" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "php4dllts - Win32 Release_TS_inline" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "php4dllts - Win32 Debug_TS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug_TS"
# PROP BASE Intermediate_Dir "Debug_TS"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_TS"
# PROP Intermediate_Dir "Debug_TS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PHP4DLLTS_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "regex" /I "..\bindlib_w32" /I "Zend" /I "tsrm" /I "ext\mysql\libmysql" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "PHP4DLLTS_EXPORTS" /D "PHP_EXPORTS" /D "LIBZEND_EXPORTS" /D "TSRM_EXPORTS" /D "SAPI_EXPORTS" /D ZEND_DEBUG=1 /D "MSVC5" /D "ZTS" /D "WIN32" /D "_MBCS" /D "ZEND_WIN32" /D "PHP_WIN32" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "_DEBUG"
# ADD RSC /l 0x40d /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib wsock32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ZendTS.lib TSRM.lib resolv.lib libmysql.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"libcmt" /nodefaultlib:"libc" /out:"Debug_TS/php4ts_debug.dll" /pdbtype:sept /libpath:"TSRM\Debug_TS" /libpath:"Zend\Debug_TS" /libpath:"..\bindlib_w32\Debug" /libpath:"ext\mysql\libmysql\Debug_TS"

!ELSEIF  "$(CFG)" == "php4dllts - Win32 Release_TS"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release_TS"
# PROP BASE Intermediate_Dir "Release_TS"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_TS"
# PROP Intermediate_Dir "Release_TS"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PHP4DLLTS_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "regex" /I "..\bindlib_w32" /I "Zend" /I "tsrm" /I "ext\mysql\libmysql" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "PHP4DLLTS_EXPORTS" /D "PHP_EXPORTS" /D "LIBZEND_EXPORTS" /D "TSRM_EXPORTS" /D "SAPI_EXPORTS" /D ZEND_DEBUG=0 /D "MSVC5" /D "ZTS" /D "WIN32" /D "_MBCS" /D "ZEND_WIN32" /D "PHP_WIN32" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "NDEBUG"
# ADD RSC /l 0x40d /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib wsock32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ZendTS.lib TSRM.lib resolv.lib libmysql.lib /nologo /dll /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /out:"Release_TS/php4ts.dll" /libpath:"TSRM\Release_TS" /libpath:"Zend\Release_TS" /libpath:"..\bindlib_w32\Release" /libpath:"ext\mysql\libmysql\Release_TS"

!ELSEIF  "$(CFG)" == "php4dllts - Win32 Release_TS_inline"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "php4dllts___Win32_Release_TS_inline"
# PROP BASE Intermediate_Dir "php4dllts___Win32_Release_TS_inline"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_TS_inline"
# PROP Intermediate_Dir "Release_TS_inline"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "." /I "regex" /I "..\bindlib_w32" /I "Zend" /I "tsrm" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "PHP4DLLTS_EXPORTS" /D "MSVC5" /D "PHP_EXPORTS" /D "LIBZEND_EXPORTS" /D "TSRM_EXPORTS" /D "SAPI_EXPORTS" /D "ZTS" /D "WIN32" /D "_MBCS" /D ZEND_DEBUG=0 /FR /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "regex" /I "..\bindlib_w32" /I "Zend" /I "tsrm" /I "ext\mysql\libmysql" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "PHP4DLLTS_EXPORTS" /D "PHP_EXPORTS" /D "LIBZEND_EXPORTS" /D "TSRM_EXPORTS" /D "SAPI_EXPORTS" /D ZEND_DEBUG=0 /D "ZEND_WIN32_FORCE_INLINE" /D "MSVC5" /D "ZTS" /D "ZEND_WIN32" /D "PHP_WIN32" /D "WIN32" /D "_MBCS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "NDEBUG"
# ADD RSC /l 0x40d /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib wsock32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ZendTS.lib TSRM.lib resolv.lib /nologo /dll /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /out:"Release_TS/php4ts.dll" /libpath:"TSRM\Release_TS" /libpath:"Zend\Release_TS" /libpath:"..\bindlib_w32\Release"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib wsock32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ZendTS.lib TSRM.lib resolv.lib libmysql.lib /nologo /dll /machine:I386 /nodefaultlib:"libc.lib" /nodefaultlib:"libcmt.lib" /out:"Release_TS_inline/php4ts.dll" /libpath:"TSRM\Release_TS_inline" /libpath:"Zend\Release_TS_inline" /libpath:"..\bindlib_w32\Release" /libpath:"ext\mysql\libmysql\Release_TS_inline"

!ENDIF 

# Begin Target

# Name "php4dllts - Win32 Debug_TS"
# Name "php4dllts - Win32 Release_TS"
# Name "php4dllts - Win32 Release_TS_inline"
# Begin Group "Core"

# PROP Default_Filter ""
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\configuration-parser.c"
# End Source File
# Begin Source File

SOURCE=".\configuration-scanner.c"
# End Source File
# Begin Source File

SOURCE=.\ext\standard\cyr_convert.c
# End Source File
# Begin Source File

SOURCE=".\fopen-wrappers.c"
# End Source File
# Begin Source File

SOURCE=.\internal_functions_win32.c
# ADD CPP /I "ext\xml\expat\xmltok" /I "ext\xml\expat\xmlparse"
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=.\mergesort.c
# End Source File
# Begin Source File

SOURCE=.\php_content_types.c
# End Source File
# Begin Source File

SOURCE=.\php_ini.c
# End Source File
# Begin Source File

SOURCE=.\php_realpath.c
# End Source File
# Begin Source File

SOURCE=.\php_ticks.c
# End Source File
# Begin Source File

SOURCE=.\php_variables.c
# End Source File
# Begin Source File

SOURCE=.\php_virtual_cwd.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\quot_print.c
# End Source File
# Begin Source File

SOURCE=.\reentrancy.c
# End Source File
# Begin Source File

SOURCE=.\rfc1867.c
# End Source File
# Begin Source File

SOURCE=.\safe_mode.c
# End Source File
# Begin Source File

SOURCE=.\SAPI.c
# End Source File
# Begin Source File

SOURCE=.\strlcat.c
# End Source File
# Begin Source File

SOURCE=.\strlcpy.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\config.w32.h
# End Source File
# Begin Source File

SOURCE=".\configuration-parser.h"
# End Source File
# Begin Source File

SOURCE=".\configuration-parser.tab.h"
# End Source File
# Begin Source File

SOURCE=.\ext\standard\cyr_convert.h
# End Source File
# Begin Source File

SOURCE=.\getopt.h
# End Source File
# Begin Source File

SOURCE=.\internal_functions_registry.h
# End Source File
# Begin Source File

SOURCE=.\logos.h
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\output.h
# End Source File
# Begin Source File

SOURCE=.\php.h
# End Source File
# Begin Source File

SOURCE=.\php3_compat.h
# End Source File
# Begin Source File

SOURCE=.\php_compat.h
# End Source File
# Begin Source File

SOURCE=.\php_content_types.h
# End Source File
# Begin Source File

SOURCE=.\php_globals.h
# End Source File
# Begin Source File

SOURCE=.\php_ini.h
# End Source File
# Begin Source File

SOURCE=.\php_realpath.h
# End Source File
# Begin Source File

SOURCE=.\php_regex.h
# End Source File
# Begin Source File

SOURCE=.\php_variables.h
# End Source File
# Begin Source File

SOURCE=.\php_virtual_cwd.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\quot_print.h
# End Source File
# Begin Source File

SOURCE=.\rfc1867.h
# End Source File
# Begin Source File

SOURCE=.\safe_mode.h
# End Source File
# Begin Source File

SOURCE=.\SAPI.h
# End Source File
# Begin Source File

SOURCE=.\win95nt.h
# End Source File
# End Group
# End Group
# Begin Group "Function Modules"

# PROP Default_Filter ""
# Begin Group "Source Files No. 1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ext\standard\array.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\assert.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\base64.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\basic_functions.c
# End Source File
# Begin Source File

SOURCE=.\ext\bcmath\bcmath.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\browscap.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\datetime.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\dir.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\dl.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\dns.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\exec.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\file.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\filestat.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\flock_compat.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\formatted_print.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\fsock.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\head.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\html.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\image.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\info.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\iptc.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\lcg.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\link.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\mail.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\math.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\md5.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\metaphone.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\microtime.c
# End Source File
# Begin Source File

SOURCE=.\ext\session\mod_files.c
# End Source File
# Begin Source File

SOURCE=.\ext\session\mod_user.c
# End Source File
# Begin Source File

SOURCE=.\ext\bcmath\number.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\output.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\pack.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\pageinfo.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\parsedate.c
# End Source File
# Begin Source File

SOURCE=.\ext\mcal\php_mcal.c
# End Source File
# Begin Source File

SOURCE=.\ext\mysql\php_mysql.c
# End Source File
# Begin Source File

SOURCE=.\ext\odbc\php_odbc.c
# End Source File
# Begin Source File

SOURCE=.\ext\pcre\php_pcre.c
# ADD CPP /D "STATIC"
# End Source File
# Begin Source File

SOURCE=.\ext\standard\rand.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\reg.c
# End Source File
# Begin Source File

SOURCE=.\ext\session\session.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\soundex.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\string.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\strnatcmp.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\syslog.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\type.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\uniqid.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\url.c
# End Source File
# Begin Source File

SOURCE=.\ext\standard\var.c
# End Source File
# End Group
# Begin Group "Header Files No. 1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ext\standard\base64.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\basic_functions.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\datetime.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\dl.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\dns.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\exec.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\file.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\flock_compat.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\fsock.h
# End Source File
# Begin Source File

SOURCE=.\functions\global.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\head.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\html.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\info.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\md5.h
# End Source File
# Begin Source File

SOURCE=.\ext\session\mod_user.h
# End Source File
# Begin Source File

SOURCE=.\ext\bcmath\number.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\pageinfo.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\php_array.h
# End Source File
# Begin Source File

SOURCE=.\ext\bcmath\php_bcmath.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\php_dir.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\php_filestat.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\php_lcg.h
# End Source File
# Begin Source File

SOURCE=.\ext\ldap\php_ldap.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\php_mail.h
# End Source File
# Begin Source File

SOURCE=.\ext\mcal\php_mcal.h
# End Source File
# Begin Source File

SOURCE=.\ext\mysql\php_mysql.h
# End Source File
# Begin Source File

SOURCE=.\ext\odbc\php_odbc.h
# End Source File
# Begin Source File

SOURCE=.\ext\pcre\php_pcre.h
# End Source File
# Begin Source File

SOURCE=.\ext\session\php_session.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\php_string.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\php_syslog.h
# End Source File
# Begin Source File

SOURCE=.\functions\phpdir.h
# End Source File
# Begin Source File

SOURCE=.\functions\phpmath.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\reg.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\type.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\uniqid.h
# End Source File
# Begin Source File

SOURCE=.\ext\standard\url.h
# End Source File
# End Group
# Begin Group "PCRE"

# PROP Default_Filter ""
# Begin Group "Source Files No. 3"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=.\ext\pcre\pcrelib\chartables.c
# ADD CPP /D "STATIC"
# End Source File
# Begin Source File

SOURCE=.\ext\pcre\pcrelib\get.c
# ADD CPP /D "STATIC"
# End Source File
# Begin Source File

SOURCE=.\ext\pcre\pcrelib\maketables.c
# ADD CPP /D "STATIC"
# End Source File
# Begin Source File

SOURCE=.\ext\pcre\pcrelib\pcre.c
# ADD CPP /D "STATIC"
# End Source File
# Begin Source File

SOURCE=.\ext\pcre\pcrelib\study.c
# ADD CPP /D "STATIC"
# End Source File
# End Group
# Begin Group "Header Files No. 3"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\ext\pcre\pcrelib\internal.h
# End Source File
# Begin Source File

SOURCE=.\ext\pcre\pcrelib\pcre.h
# End Source File
# End Group
# End Group
# Begin Group "Regular Expressions"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\regex\regcomp.c
# End Source File
# Begin Source File

SOURCE=.\regex\regerror.c
# End Source File
# Begin Source File

SOURCE=.\regex\regexec.c
# End Source File
# Begin Source File

SOURCE=.\regex\regfree.c
# End Source File
# End Group
# Begin Group "XML"

# PROP Default_Filter ""
# Begin Group "Source Files No. 4"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=.\ext\wddx\wddx.c
# ADD CPP /I "ext\xml\expat\xmltok" /I "ext\xml\expat\xmlparse"
# End Source File
# Begin Source File

SOURCE=.\ext\xml\xml.c
# ADD CPP /I "ext\xml\expat\xmltok" /I "ext\xml\expat\xmlparse"
# End Source File
# End Group
# Begin Group "Header Files No. 4"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\ext\wddx\php_wddx.h
# End Source File
# Begin Source File

SOURCE=.\ext\wddx\php_wddx_api.h
# End Source File
# Begin Source File

SOURCE=.\ext\xml\php_xml.h
# End Source File
# End Group
# Begin Group "expat"

# PROP Default_Filter ""
# Begin Group "Source Files No. 5"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\ext\xml\expat\xmlparse\hashtable.c
# ADD CPP /I "ext\xml\expat\xmltok"
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmlparse\xmlparse.c
# ADD CPP /I "ext\xml\expat\xmltok"
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\xmlrole.c
# ADD CPP /I "ext\xml\expat\xmltok"
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\xmltok.c
# ADD CPP /I "ext\xml\expat\xmltok"
# End Source File
# End Group
# Begin Group "Header Files No. 5"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\asciitab.h
# End Source File
# Begin Source File

SOURCE=.\hashtable.h
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\iasciitab.h
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\latin1tab.h
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\nametab.h
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\utf8tab.h
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\xmldef.h
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmlparse\xmlparse.h
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\xmlrole.h
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\xmltok.h
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\xmltok_impl.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\xmltok_impl.h
# End Source File
# Begin Source File

SOURCE=.\ext\xml\expat\xmltok\xmltok_ns.c
# PROP Exclude_From_Build 1
# End Source File
# End Group
# End Group
# End Group
# End Group
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Group "Source Files No. 2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ext\com\COM.c
# End Source File
# Begin Source File

SOURCE=.\win32\pwd.c
# End Source File
# Begin Source File

SOURCE=.\win32\readdir.c
# End Source File
# Begin Source File

SOURCE=.\win32\registry.c
# End Source File
# Begin Source File

SOURCE=.\win32\sendmail.c
# End Source File
# Begin Source File

SOURCE=.\win32\time.c
# End Source File
# Begin Source File

SOURCE=.\win32\wfile.c
# End Source File
# Begin Source File

SOURCE=.\dl\snmp\winsnmp.c
# End Source File
# Begin Source File

SOURCE=.\win32\winutil.c
# End Source File
# Begin Source File

SOURCE=.\win32\wsyslog.c
# End Source File
# End Group
# Begin Group "Header Files No. 2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\win32\grp.h
# End Source File
# Begin Source File

SOURCE=.\win32\param.h
# End Source File
# Begin Source File

SOURCE=.\ext\com\php_COM.h
# End Source File
# Begin Source File

SOURCE=.\win32\php_registry.h
# End Source File
# Begin Source File

SOURCE=.\win32\pwd.h
# End Source File
# Begin Source File

SOURCE=.\win32\readdir.h
# End Source File
# Begin Source File

SOURCE=.\win32\sendmail.h
# End Source File
# Begin Source File

SOURCE=.\win32\syslog.h
# End Source File
# Begin Source File

SOURCE=.\win32\time.h
# End Source File
# Begin Source File

SOURCE=.\win32\unistd.h
# End Source File
# Begin Source File

SOURCE=.\win32\wfile.h
# End Source File
# End Group
# End Group
# Begin Group "Parsers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\configuration-parser.y"

!IF  "$(CFG)" == "php4dllts - Win32 Debug_TS"

# Begin Custom Build
InputPath=".\configuration-parser.y"

BuildCmds= \
	if not "X%CYGWIN%"=="X" bison --output=configuration-parser.c -v -d -S "%CYGWIN%\share\bison.simple" -p cfg configuration-parser.y \
	if "X%CYGWIN%"=="X" bison --output=configuration-parser.c -v -d -S "C:\Program Files\Cygnus\share\bison.simple" -p cfg configuration-parser.y \
	

"configuration-parser.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"configuration-parser.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "php4dllts - Win32 Release_TS"

# Begin Custom Build
InputPath=".\configuration-parser.y"

BuildCmds= \
	if not "X%CYGWIN%"=="X" bison --output=configuration-parser.c -v -d -S "%CYGWIN%\share\bison.simple" -p cfg configuration-parser.y \
	if "X%CYGWIN%"=="X" bison --output=configuration-parser.c -v -d -S "C:\Program Files\Cygnus\share\bison.simple" -p cfg configuration-parser.y \
	

"configuration-parser.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"configuration-parser.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "php4dllts - Win32 Release_TS_inline"

# Begin Custom Build
InputPath=".\configuration-parser.y"

BuildCmds= \
	if not "X%CYGWIN%"=="X" bison --output=configuration-parser.c -v -d -S "%CYGWIN%\share\bison.simple" -p cfg configuration-parser.y \
	if "X%CYGWIN%"=="X" bison --output=configuration-parser.c -v -d -S "C:\Program Files\Cygnus\share\bison.simple" -p cfg configuration-parser.y \
	

"configuration-parser.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"configuration-parser.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ext\standard\parsedate.y

!IF  "$(CFG)" == "php4dllts - Win32 Debug_TS"

# Begin Custom Build
InputPath=.\ext\standard\parsedate.y

"ext\standard\parsedate.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cd ext\standard 
	if not "X%CYGWIN%"=="X" bison --output=parsedate.c -v -d -S "%CYGWIN%\share\bison.simple" parsedate.y 
	if "X%CYGWIN%"=="X" bison --output=parsedate.c -v -d -S "C:\Program Files\Cygnus\share\bison.simple" parsedate.y 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "php4dllts - Win32 Release_TS"

# Begin Custom Build
InputPath=.\ext\standard\parsedate.y

"ext\standard\parsedate.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cd ext\standard 
	if not "X%CYGWIN%"=="X" bison --output=parsedate.c -v -d -S "%CYGWIN%\share\bison.simple" parsedate.y 
	if "X%CYGWIN%"=="X" bison --output=parsedate.c -v -d -S "C:\Program Files\Cygnus\share\bison.simple" parsedate.y 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "php4dllts - Win32 Release_TS_inline"

# Begin Custom Build
InputPath=.\ext\standard\parsedate.y

"ext\standard\parsedate.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cd ext\standard 
	if not "X%CYGWIN%"=="X" bison --output=parsedate.c -v -d -S "%CYGWIN%\share\bison.simple" parsedate.y 
	if "X%CYGWIN%"=="X" bison --output=parsedate.c -v -d -S "C:\Program Files\Cygnus\share\bison.simple" parsedate.y 
	
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Scanners"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\configuration-scanner.l"

!IF  "$(CFG)" == "php4dllts - Win32 Debug_TS"

# Begin Custom Build
InputPath=".\configuration-scanner.l"

"configuration-scanner.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	flex -i -Pcfg -oconfiguration-scanner.c configuration-scanner.l

# End Custom Build

!ELSEIF  "$(CFG)" == "php4dllts - Win32 Release_TS"

# Begin Custom Build
InputPath=".\configuration-scanner.l"

"configuration-scanner.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	flex -i -Pcfg -oconfiguration-scanner.c configuration-scanner.l

# End Custom Build

!ELSEIF  "$(CFG)" == "php4dllts - Win32 Release_TS_inline"

# Begin Custom Build
InputPath=".\configuration-scanner.l"

"configuration-scanner.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	flex -i -Pcfg -oconfiguration-scanner.c configuration-scanner.l

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Text Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ChangeLog
# End Source File
# Begin Source File

SOURCE=.\LICENSE
# End Source File
# Begin Source File

SOURCE=.\NEWS
# End Source File
# Begin Source File

SOURCE=".\php.ini-dist"
# End Source File
# Begin Source File

SOURCE=".\README.CVS-RULES"
# End Source File
# Begin Source File

SOURCE=.\TODO
# End Source File
# End Group
# End Target
# End Project
