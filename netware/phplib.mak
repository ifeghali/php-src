# Temporarily here -- later may go into some batch file
# which will set this as an environment variable
PROJECT_ROOT = .

# Module details
MODULE_NAME = phplib
MODULE_DESC = "PHP 5 - Script Interpreter and Library"
VMAJ = 3
VMIN = 0
VREV = 0

#include the common settings
include $(PROJECT_ROOT)/netware/common.mif


# Extensions of all input and output files
.SUFFIXES:
.SUFFIXES: .nlm .lib .obj .cpp .c .msg .mlc .mdb .xdc .d

# Source files
C_SRC = ext/bcmath/bcmath.c \
        ext/bcmath/libbcmath/src/add.c \
        ext/bcmath/libbcmath/src/compare.c \
        ext/bcmath/libbcmath/src/debug.c \
        ext/bcmath/libbcmath/src/div.c \
        ext/bcmath/libbcmath/src/divmod.c \
        ext/bcmath/libbcmath/src/doaddsub.c \
        ext/bcmath/libbcmath/src/init.c \
        ext/bcmath/libbcmath/src/int2num.c \
        ext/bcmath/libbcmath/src/nearzero.c \
        ext/bcmath/libbcmath/src/neg.c \
        ext/bcmath/libbcmath/src/num2long.c \
        ext/bcmath/libbcmath/src/num2str.c \
        ext/bcmath/libbcmath/src/outofmem.c \
        ext/bcmath/libbcmath/src/output.c \
        ext/bcmath/libbcmath/src/raise.c \
        ext/bcmath/libbcmath/src/raisemod.c \
        ext/bcmath/libbcmath/src/recmul.c \
        ext/bcmath/libbcmath/src/rmzero.c \
        ext/bcmath/libbcmath/src/rt.c \
        ext/bcmath/libbcmath/src/sqrt.c \
        ext/bcmath/libbcmath/src/str2num.c \
        ext/bcmath/libbcmath/src/sub.c \
        ext/bcmath/libbcmath/src/zero.c \
        ext/calendar/cal_unix.c \
        ext/calendar/calendar.c \
        ext/calendar/dow.c \
        ext/calendar/easter.c \
        ext/calendar/french.c \
        ext/calendar/gregor.c \
        ext/calendar/jewish.c \
        ext/calendar/julian.c \
        ext/ftp/ftp.c \
        ext/ftp/php_ftp.c \
        ext/mcal/php_mcal.c \
        ext/pcre/php_pcre.c \
        ext/pcre/pcrelib/chartables.c \
        ext/pcre/pcrelib/get.c \
        ext/pcre/pcrelib/maketables.c \
        ext/pcre/pcrelib/pcre.c \
        ext/pcre/pcrelib/pcreposix.c \
        ext/pcre/pcrelib/study.c \
        ext/session/mod_files.c \
        ext/session/mod_mm.c \
        ext/session/mod_user.c \
        ext/session/session.c \
        ext/snmp/snmp.c \
        ext/standard/aggregation.c \
        ext/standard/array.c \
        ext/standard/assert.c \
        ext/standard/base64.c \
        ext/standard/basic_functions.c \
        ext/standard/browscap.c \
        ext/standard/crc32.c \
        ext/standard/credits.c \
        ext/standard/css.c \
        ext/standard/cyr_convert.c \
        ext/standard/datetime.c \
        ext/standard/dir.c \
        ext/standard/dl.c \
        ext/standard/dns.c \
        ext/standard/exec.c \
        ext/standard/file.c \
        ext/standard/filestat.c \
        ext/standard/filters.c \
        ext/standard/flock_compat.c \
        ext/standard/formatted_print.c \
        ext/standard/fsock.c \
        ext/standard/ftok.c \
        ext/standard/ftp_fopen_wrapper.c \
        ext/standard/head.c \
        ext/standard/html.c \
        ext/standard/http_fopen_wrapper.c \
        ext/standard/image.c \
        ext/standard/incomplete_class.c \
        ext/standard/info.c \
        ext/standard/iptc.c \
        ext/standard/lcg.c \
        ext/standard/levenshtein.c \
        ext/standard/link.c \
        ext/standard/mail.c \
        ext/standard/math.c \
        ext/standard/md5.c \
        ext/standard/metaphone.c \
        ext/standard/microtime.c \
        ext/standard/pack.c \
        ext/standard/pageinfo.c \
        ext/standard/parsedate.c \
        ext/standard/php_fopen_wrapper.c \
        ext/standard/quot_print.c \
        ext/standard/rand.c \
        ext/standard/reg.c \
        ext/standard/scanf.c \
        ext/standard/sha1.c \
        ext/standard/soundex.c \
        ext/standard/string.c \
        ext/standard/strnatcmp.c \
        ext/standard/type.c \
        ext/standard/uniqid.c \
        ext/standard/url.c \
        ext/standard/url_scanner.c \
        ext/standard/url_scanner_ex.c \
        ext/standard/user_filters.c \
        ext/standard/var.c \
        ext/standard/var_unserializer.c \
        ext/standard/versioning.c \
        main/fopen_wrappers.c \
        main/internal_functions_nw.c \
        main/main.c \
        main/memory_streams.c \
        main/mergesort.c \
        main/network.c \
        main/output.c \
        main/php_content_types.c \
        main/php_ini.c \
        main/php_logos.c \
        main/php_open_temporary_file.c \
        main/php_sprintf.c \
        main/php_ticks.c \
        main/php_variables.c \
        main/reentrancy.c \
        main/rfc1867.c \
        main/safe_mode.c \
        main/SAPI.c \
        main/snprintf.c \
        main/spprintf.c \
        main/streams.c \
        main/strlcat.c \
        main/strlcpy.c \
        main/user_streams.c \
        netware/geterrnoptr.c \
        netware/mktemp.c \
        netware/pipe.c \
        netware/pwd.c \
        netware/start.c \
        netware/time_nw.c \
        netware/wfile.c \
        regex/regcomp.c \
        regex/regerror.c \
        regex/regexec.c \
        regex/regfree.c \
#        ext/standard/crypt.c \
#        ext/com/COM.c \
#        ext/com/conversion.c \
#        ext/odbc/php_odbc.c \
#        ext/snmp/winsnmp.c \
#        netware/winutil.c \
#        netware/registry.c \
#        main/alloca.c \
#        ext/wddx/wddx.c \
#        netware/readdir.c \

WIN32_C_SRC = sendmail.c \


CPP_SRC_NODIR = $(notdir $(CPP_SRC))
C_SRC_NODIR = $(notdir $(C_SRC))
SRC_DIR = $(dir $(CPP_SRC) $(C_SRC))

# Library files
LIBRARY  = $(PROJECT_ROOT)/Zend/$(BUILD)/Zend.lib $(PROJECT_ROOT)/TSRM/$(BUILD)/TSRM.lib

# Destination directories and files
OBJ_DIR = $(BUILD)
FINAL_DIR = $(BUILD)
MAP_FILE = $(FINAL_DIR)\$(MODULE_NAME).map
OBJECTS  = $(join $(SRC_DIR), $(addprefix $(OBJ_DIR)/,$(CPP_SRC_NODIR:.c=.obj) $(C_SRC_NODIR:.c=.obj)))
WIN32_OBJECTS  = $(addprefix $(OBJ_DIR)/,$(WIN32_C_SRC:.c=.obj))
DEPDS  = $(join $(SRC_DIR), $(addprefix $(OBJ_DIR)/,$(CPP_SRC_NODIR:.c=.d) $(C_SRC_NODIR:.c=.d)))

# Binary file
ifndef BINARY
	BINARY=$(FINAL_DIR)\$(MODULE_NAME).nlm
endif

# Compile flags
C_FLAGS += -c -maxerrors 25 -msgstyle gcc
C_FLAGS += -wchar_t on -bool on
C_FLAGS += -relax_pointers		# To remove type-casting errors
C_FLAGS += -processor Pentium
C_FLAGS += -nostdinc -nosyspath
C_FLAGS  += -DNETWARE -D__BIT_TYPES_DEFINED__ -DZTS
C_FLAGS  += -DPHP5DLLTS_EXPORTS -DPHP_EXPORTS -DLIBZEND_EXPORTS -DTSRM_EXPORTS -DSAPI_EXPORTS
C_FLAGS  += -DHAVE_SYS_TIME_H -DHAVE_STRUCT_FLOCK -DVIRTUAL_DIR -DHAVE_TZNAME
C_FLAGS  += -DHAVE_DLFCN_H -DHAVE_LIBDL
C_FLAGS  += -DCLIB_STAT_PATCH		# stat patch
C_FLAGS  += -DNEW_LIBC -DHAVE_ARPA_INET_H=1 -DHAVE_NETINET_IN_H=1 -DHAVE_INET_ATON=1

C_FLAGS  += -DLINK_SIZE=2		# Added for the new definition in ext\pcre\pcrelib\internal.h
# Below added for the file ext\pcre\pcrelib\pcreposix.c and
# the default value is what is defined in the changelog file
C_FLAGS  += -DPOSIX_MALLOC_THRESHOLD=5

C_FLAGS  += -I. -Imain -Inetware -Iregex -IZend -ITSRM
C_FLAGS  += -Iext/standard -Iext/mcal -Iext/pcre -Iext/pcre/pcrelib
C_FLAGS  += -Iext/bcmath -Iext/bcmath/libbcmath/src
C_FLAGS  += -Iext/odbc -Iext/session -Iext/ftp -Iext/wddx -Iext/calendar -Iext/snmp
C_FLAGS  += -I- -Inetware -I$(SDK_DIR)/include -I$(MWCIncludes)	# netware added for special SYS/STAT.H : Venkat(6/2/02)
C_FLAGS  += -Iwin32
C_FLAGS  += -I$(SDK_DIR)/include/winsock	# Added for socket calls


# Extra stuff based on debug / release builds
ifeq '$(BUILD)' 'debug'
	SYM_FILE = $(FINAL_DIR)\$(MODULE_NAME).sym
	C_FLAGS  += -inline smart -sym on -sym codeview4 -sym internal -opt off -opt intrinsics 
	C_FLAGS += -D_DEBUG -DZEND_DEBUG=1
	LD_FLAGS += -sym codeview4 -sym internal -osym $(SYM_FILE) 
	export MWLibraryFiles=$(SDK_DIR)/imports/libcpre.o;mwcrtl.lib
else
	C_FLAGS  += -opt speed -inline on -inline auto -sym off -DZEND_DEBUG=0
	LD_FLAGS += -sym off
	export MWLibraryFiles=$(SDK_DIR)/imports/libcpre.o;mwcrtl.lib
endif

# Dependencies
MODULE = LibC
IMPORT = @$(SDK_DIR)/imports/libc.imp   \
         @$(SDK_DIR)/imports/ws2nlm.imp \
         @$(MPK_DIR)/import/mpkOrg.imp
EXPORT = @$(PROJECT_ROOT)/netware/phplib.imp
API = NXGetEnvCount \
      NXCopyEnv \
      OutputToScreen \
      bsd_fd_set


# Virtual paths
vpath %.cpp .
vpath %.c . win32
vpath %.obj $(OBJ_DIR)


all: prebuild project

.PHONY: all

prebuild:
	@if not exist main\$(OBJ_DIR)                     md main\$(OBJ_DIR)
	@if not exist regex\$(OBJ_DIR)                    md regex\$(OBJ_DIR)
	@if not exist netware\$(OBJ_DIR)                  md netware\$(OBJ_DIR)
	@if not exist ext\standard\$(OBJ_DIR)             md ext\standard\$(OBJ_DIR)
	@if not exist ext\bcmath\$(OBJ_DIR)               md ext\bcmath\$(OBJ_DIR)
	@if not exist ext\bcmath\libbcmath\src\$(OBJ_DIR) md ext\bcmath\libbcmath\src\$(OBJ_DIR)
	@if not exist ext\mcal\$(OBJ_DIR)                 md ext\mcal\$(OBJ_DIR)
	@if not exist ext\pcre\$(OBJ_DIR)                 md ext\pcre\$(OBJ_DIR)
	@if not exist ext\pcre\pcrelib\$(OBJ_DIR)         md ext\pcre\pcrelib\$(OBJ_DIR)
	@if not exist ext\odbc\$(OBJ_DIR)                 md ext\odbc\$(OBJ_DIR)
	@if not exist ext\session\$(OBJ_DIR)              md ext\session\$(OBJ_DIR)
	@if not exist ext\ftp\$(OBJ_DIR)                  md ext\ftp\$(OBJ_DIR)
	@if not exist ext\wddx\$(OBJ_DIR)                 md ext\wddx\$(OBJ_DIR)
	@if not exist ext\calendar\$(OBJ_DIR)             md ext\calendar\$(OBJ_DIR)
	@if not exist ext\snmp\$(OBJ_DIR)                 md ext\snmp\$(OBJ_DIR)
	@if not exist $(FINAL_DIR)                        md $(FINAL_DIR)

project: $(BINARY) $(MESSAGE)
	@echo Build complete.

%.d: ../%.c
	@echo Building Dependencies for $(<F)
	@$(CC) -M $< $(C_FLAGS) -o $@
	
%.obj: ../%.c
	@echo Compiling $?...
# writing the flags to a file. Otherwise fails on win95 due too many characters in command line.
	@echo $(wordlist 1, 20, $(C_FLAGS)) > $(basename $(OBJ_DIR)\$(notdir $@)).cfg
	@echo $(wordlist 21, 40, $(C_FLAGS)) >> $(basename $(OBJ_DIR)\$(notdir $@)).cfg
	@echo $(wordlist 41, 60, $(C_FLAGS)) >> $(basename $(OBJ_DIR)\$(notdir $@)).cfg
	@echo $(wordlist 61, 80, $(C_FLAGS)) >> $(basename $(OBJ_DIR)\$(notdir $@)).cfg
	@$(CC) $< @$(basename $(OBJ_DIR)\$(notdir $@)).cfg -o $@ 
	@del $(basename $(OBJ_DIR)\$(notdir $@)).cfg


$(BINARY): $(OBJECTS) $(WIN32_OBJECTS) $(LIBRARY)
	@echo Import $(IMPORT) > $(basename $@).def
ifdef API
	@echo Import $(API) >> $(basename $@).def
endif
	@echo Module $(MODULE) >> $(basename $@).def
ifdef EXPORT
	@echo Export $(EXPORT) >> $(basename $@).def
endif
	@echo AutoUnload >> $(basename $@).def
ifeq '$(BUILD)' 'debug'
	@echo Debug >> $(basename $@).def
endif
	@echo Flag_On 0x00000008 >> $(basename $@).def
	@echo Start _LibCPrelude >> $(basename $@).def
	@echo Exit _LibCPostlude >> $(basename $@).def

	$(MPKTOOL) $(XDCFLAGS) $(basename $@).xdc
	@echo xdcdata $(basename $@).xdc >> $(basename $@).def

	@echo Linking $@...
	@echo $(LD_FLAGS) -commandfile $(basename $@).def > $(basename $@).link
ifdef LIBRARY
	@echo $(LIBRARY) >> $(basename $@).link
endif
	@echo $(wordlist   1,  10, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist  11,  20, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist  21,  30, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist  31,  40, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist  41,  50, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist  51,  60, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist  61, 70, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 71, 80, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 81, 85, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 86, 90, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 91, 95, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 96, 100, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 101, 105, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 106, 110, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 111, 115, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 116, 120, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 121, 125, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 126, 130, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 131, 135, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 136, 140, $(OBJECTS)) >> $(basename $@).link
	@echo $(wordlist 141, 141, $(OBJECTS)) >> $(basename $@).link

	@echo $(wordlist 1, 2, $(WIN32_OBJECTS)) >> $(basename $@).link

	@$(LINK) @$(basename $@).link


.PHONY: clean
clean: cleansrc cleanobj cleanbin

.PHONY: cleand
cleand:
	@echo Deleting all dependency files...
	-@del "main\$(OBJ_DIR)\*.d"
	-@del "regex\$(OBJ_DIR)\*.d"
	-@del "netware\$(OBJ_DIR)\*.d"
	-@del "ext\standard\$(OBJ_DIR)\*.d"
	-@del "ext\bcmath\$(OBJ_DIR)\*.d"
	-@del "ext\bcmath\libbcmath\src\$(OBJ_DIR)\*.d"
	-@del "ext\mcal\$(OBJ_DIR)\*.d"
	-@del "ext\pcre\$(OBJ_DIR)\*.d"
	-@del "ext\pcre\pcrelib\$(OBJ_DIR)\*.d"
	-@del "ext\odbc\$(OBJ_DIR)\*.d"
	-@del "ext\session\$(OBJ_DIR)\*.d"
	-@del "ext\ftp\$(OBJ_DIR)\*.d"
	-@del "ext\wddx\$(OBJ_DIR)\*.d"
	-@del "ext\calendar\$(OBJ_DIR)\*.d"
	-@del "ext\snmp\$(OBJ_DIR)\*.d"

.PHONY: cleansrc
cleansrc:
	@echo Deleting all generated source files...
	-@del "ext\standard\parsedate.c"
	-@del "ext\standard\parsedate.h"
	-@del "ext\standard\parsedate.output"

.PHONY: cleanobj
cleanobj:
	@echo Deleting all object files...
	-@del "main\$(OBJ_DIR)\*.obj"
	-@del "regex\$(OBJ_DIR)\*.obj"
	-@del "netware\$(OBJ_DIR)\*.obj"
	-@del "ext\standard\$(OBJ_DIR)\*.obj"
	-@del "ext\bcmath\$(OBJ_DIR)\*.obj"
	-@del "ext\bcmath\libbcmath\src\$(OBJ_DIR)\*.obj"
	-@del "ext\mcal\$(OBJ_DIR)\*.obj"
	-@del "ext\pcre\$(OBJ_DIR)\*.obj"
	-@del "ext\pcre\pcrelib\$(OBJ_DIR)\*.obj"
	-@del "ext\odbc\$(OBJ_DIR)\*.obj"
	-@del "ext\session\$(OBJ_DIR)\*.obj"
	-@del "ext\ftp\$(OBJ_DIR)\*.obj"
	-@del "ext\wddx\$(OBJ_DIR)\*.obj"
	-@del "ext\calendar\$(OBJ_DIR)\*.obj"
	-@del "ext\snmp\$(OBJ_DIR)\*.obj"

.PHONY: cleanbin
cleanbin:
	@echo Deleting binary files...
	-@del "$(FINAL_DIR)\$(MODULE_NAME).nlm"
	@echo Deleting MAP, DEF files, etc....
	-@del "$(FINAL_DIR)\$(MODULE_NAME).map"
	-@del "$(FINAL_DIR)\$(MODULE_NAME).def"
	-@del "$(FINAL_DIR)\$(MODULE_NAME).link"
ifeq '$(BUILD)' 'debug'
	-@del $(FINAL_DIR)\$(MODULE_NAME).sym
endif
