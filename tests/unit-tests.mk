# unit-tests.mk
# This file is part of the FreeBASIC test suite
#
# make file for building fbcunit tests
#

# ------------------------------------------------------------------------

include common.mk

ifndef UNITTEST_RUN_ARGS
UNITTEST_RUN_ARGS :=
endif

ifndef ENABLE_CHECK_BUGS
ENABLE_CHECK_BUGS :=
endif

ifndef ENABLE_CONSOLE_OUTPUT
ENABLE_CONSOLE_OUTPUT :=
endif

FIND := find
XARGS := xargs
GREP := grep
SED := sed
ECHO := echo
PRINTF := printf
CAT := cat
AR ?= ar

ifndef FBC
FBC := $(TESTS_DEFAULT_FBC)
endif

DIRLIST_INC ?= dirlist.mk

include $(DIRLIST_INC)
DIRLIST := $(DIRLIST_FB)
ifeq ($(TARGET_OS),dos)
DIRLIST := $(filter-out interactive threads,$(DIRLIST))
endif

ifeq ($(DIRLIST),)
$(error No directories specified in $(DIRLIST_INC))
endif

# ------------------------------------------------------------------------

UNIT_TESTS_INC := unit-tests.inc
UNIT_TESTS_OBJ_LST := unit-tests-obj.lst
UNIT_TESTS_OBJ_LIB := unit-tests-obj.a
UNIT_TESTS_EXPLICIT_SRCS :=
ifeq ($(filter console,$(DIRLIST)),)
UNIT_TESTS_EXPLICIT_SRCS += ./console/common.bas
endif
UNIT_TESTS_EXPLICIT_SRCS += \
./pp/macro-arg-listexpand-utf16le.bas \
./pp/macro-eval-str-utf16le.bas \
./pp/quote-utf16be.bas \
./pp/quote-utf16le.bas \
./string/asc-utf16le.bas
ifeq ($(TESTS_TARGET_OS),dos)
UNIT_TESTS_EXPLICIT_SRCS += \
./dos/compound/select_const2-part1.bas \
./dos/compound/select_const2-part2.bas \
./dos/compound/select_const2-part3.bas \
./dos/compound/select_const2-part4.bas
endif
UNIT_TESTS_EXPLICIT_OBJS := $(UNIT_TESTS_EXPLICIT_SRCS:%.bas=%.o)

SRCLIST :=
ifeq ($(MAKECMDGOALS),mostlyclean)
-include $(UNIT_TESTS_INC)
else
include $(UNIT_TESTS_INC)
endif
SRCLIST += $(UNIT_TESTS_EXPLICIT_SRCS)
SRCLIST := $(sort $(SRCLIST))
SRCLIST := $(patsubst .bmk,.bas,$(SRCLIST))
SRCLIST_DOS_FILTER_OUT :=
ifeq ($(TARGET_OS),dos)
# The monolithic select_const2 test produces a huge DOS assembly unit.
# DOS runs the same coverage through smaller tests/dos/compound shards.
SRCLIST_DOS_FILTER_OUT := \
./interactive/% interactive/% \
./threads/% threads/% \
./compound/select_const2.bas compound/select_const2.bas
SRCLIST := $(filter-out $(SRCLIST_DOS_FILTER_OUT),$(SRCLIST))
endif

# ------------------------------------------------------------------------

MAINBAS := fbc-tests
MAINEXE := fbc-tests$(TARGET_EXEEXT)

SRCLIST += ./$(MAINBAS).bas

FBCU_DIR := fbcunit
FBCU_INC := $(FBCU_DIR)/inc
FBCU_LIB := $(FBCU_DIR)/lib
FBCU_BIN := $(FBCU_LIB)/libfbcunit.a
FBCU_MAKE := $(MAKE)
ifeq ($(HOST),dos)
	FBCU_MAKE := make.exe
endif

FBCU_LIBS := -l fbcunit

ifeq ($(TARGET_OS),win32)
    FBCU_LIBS += -l user32
endif

# Unit sources are compiled separately, so a THREADCALL source's automatic
# libffi dependency is not retained for the final aggregate link. RISC OS has
# a target libffi build and enables that ARM coverage explicitly.
ifeq ($(TESTS_TARGET_OS),riscos)
ifneq ($(filter threads,$(DIRLIST)),)
    FBCU_LIBS += -l ffi
endif
endif

FBC_CFLAGS := -c -w 3 -i $(FBCU_INC) -m $(MAINBAS)
ifeq ($(TESTS_TARGET_OS),riscos)
FBC_CFLAGS += -i $(abspath ../inc/riscos)
endif
FBC_CFLAGS += -i $(abspath ../inc)
ifneq ($(TARGET_OS),dos)
	FBC_CFLAGS += -Wc -Wno-tautological-compare
endif
ifneq ($(TARGET_OS),dos)
	FBC_CFLAGS += -mt
endif
ifeq ($(TARGET_OS),js)
# Need to do some optimisations to reduce the number of local variables,
# or else linking fails
	FBC_CFLAGS += -O 1
endif
ifdef DEBUG
	FBC_CFLAGS += -g
endif
ifdef EXTRAERR
	FBC_CFLAGS += -exx
endif
ifdef ARCH
	FBC_CFLAGS += -arch $(ARCH)
endif
ifneq ($(TARGET),)
	FBC_CFLAGS += -target $(TARGET)
endif
ifneq ($(FPU),)
	FBC_CFLAGS += -fpu $(FPU)
endif
ifneq ($(FPMODE),)
	FBC_CFLAGS += -fpmode $(FPMODE)
endif
ifneq ($(GEN),)
	FBC_CFLAGS += -gen $(GEN)
endif

FBC_LFLAGS := $(FBCU_LIBS) -p $(FBCU_LIB) -fbgfx -x $(MAINEXE) -v
ifneq ($(TARGET_OS),dos)
	FBC_LFLAGS += -mt
endif
ifdef DEBUG
	FBC_LFLAGS += -g
endif
ifdef ARCH
	FBC_LFLAGS += -arch $(ARCH)
endif
ifdef TARGET
	FBC_LFLAGS += -target $(TARGET)
endif
ifeq ($(TARGET_OS),js)
# Copy the file/ and data/ directories into the in-memory FS
	FBC_LFLAGS += -Wl -sEXIT_RUNTIME -Wl --preload-file,boolean,--preload-file,data,--preload-file,file,--preload-file,wstring
endif
ifeq ($(TARGET_OS),wii)
	AR := powerpc-eabi-ar
	WII_CC ?= powerpc-eabi-gcc
	WII_ELF2DOL ?= elf2dol
	WII_DEVKITPRO ?= $(DEVKITPRO)
	WII_FB_PREFIX ?= $(FBWII_PREFIX)
	ifeq ($(WII_DEVKITPRO),)
		WII_DEVKITPRO := /opt/devkitpro
	endif
	ifeq ($(WII_FB_PREFIX),)
		WII_FB_PREFIX := ../build/wii-sdk
	endif
	WII_FB_LIBDIR := $(WII_FB_PREFIX)/lib/freebasic-wii/wii-powerpc
endif

ifeq ($(ENABLE_CHECK_BUGS),1)
	FBC_CFLAGS += -d ENABLE_CHECK_BUGS=$(ENABLE_CHECK_BUGS)
endif
ifeq ($(ENABLE_CONSOLE_OUTPUT),1)
	FBC_CFLAGS += -d ENABLE_CONSOLE_OUTPUT=$(ENABLE_CONSOLE_OUTPUT)
endif

OBJLIST := $(SRCLIST:%.bas=%.o)

TESTS_CC_IS_CLANG := $(strip $(shell $(CC) -dM -E -x c /dev/null 2>/dev/null | grep -q __clang__ && echo yes || true))
ifeq ($(TESTS_CC_IS_CLANG),yes)
./functions/var_args-gcc.o functions/var_args-gcc.o: FBC_CFLAGS += -Wc -Wno-varargs
./optimizations/consteval.o optimizations/consteval.o: FBC_CFLAGS += -Wc -Wno-absolute-value
endif

#
#: rules
#

%.o : %.bas
	$(FBC) $(FBC_CFLAGS) $^

ifeq ($(TESTS_TARGET_OS),riscos)
./numbers/infnan.o: ./riscos/numbers/infnan.bas
	$(FBC) $(FBC_CFLAGS) $< -o $@
./numbers/limits.o: ./riscos/numbers/limits.bas
	$(FBC) $(FBC_CFLAGS) $< -o $@
./structs/anon-align.o: ./riscos/structs/anon-align.bas
	$(FBC) $(FBC_CFLAGS) $< -o $@
./structs/padding.o: ./riscos/structs/padding.bas
	$(FBC) $(FBC_CFLAGS) $< -o $@
./threads/threadcall.o: ./riscos/threads/threadcall.bas
	$(FBC) $(FBC_CFLAGS) $< -o $@
endif

#
#: targets
#

.PHONY: all
all : make_fbcunit $(UNIT_TESTS_OBJ_LST) build_tests run_tests

.PHONY: make_fbcunit
make_fbcunit : $(FBCU_BIN)

$(FBCU_BIN) :
	+$(FBCU_MAKE) -C $(FBCU_DIR) FBC="$(FBC)" FPU=$(FPU) ARCH=$(ARCH) TARGET=$(TARGET)

# ------------------------------------------------------------------------
# Auto-generate the file UNIT_TESTS_INC - needed by this makefile
# Find all *.bas files containing '# include [once] "fbcu.bi"'
# from all dirs listed in DIRLIST from DIRLIST_INC
#
#
$(UNIT_TESTS_INC) : $(DIRLIST_INC)
	@$(PRINTF) "Generating $(UNIT_TESTS_INC) : "
	@$(ECHO) "# This file automatically generated - DO NOT EDIT" > $(UNIT_TESTS_INC)
	@$(ECHO) "#" >> $(UNIT_TESTS_INC)
	@$(FIND) $(DIRLIST) -type f -name '*.bas' -or -name '*.bmk' \
| $(XARGS) $(GREP) -l -i -E \
"(#[[:space:]]*include[[:space:]](once)*[[:space:]]*\"fbcu(nit)?\.bi\")|([[:space:]]*.[[:space:]]*TEST_MODE[[:space:]]*:[[:space:]]*FBCUNIT_COMPATIBLE)" \
| $(SED) -e 's/\(^.*\)/\SRCLIST \+\= \.\/\1/g' \
>> $(UNIT_TESTS_INC)
	@$(ECHO) "Done"

# hack: generate the file UNIT_TESTS_OBJ_LST from UNIT_TESTS_INC
# Use the auto-generated list of tests to create a temporary file
# containing a list of all the object files.  The command line can be
# very long and some shells (like cmd.exe) won't handle it.

$(UNIT_TESTS_OBJ_LST) : $(UNIT_TESTS_INC)
	@$(GREP) -i -e 'SRCLIST +=' $(UNIT_TESTS_INC) \
| $(SED) -e 's/^SRCLIST += \(.*\)\(\.b.*\)/\1\.o/g' \
$(if $(filter dos,$(TARGET_OS)),-e '/^\.\/interactive\//d' -e '/^\.\/threads\//d' -e '/^\.\/compound\/select_const2\.o/d') \
> $(UNIT_TESTS_OBJ_LST)
	@$(PRINTF) '%s\n' $(UNIT_TESTS_EXPLICIT_OBJS) >> $(UNIT_TESTS_OBJ_LST)

# ------------------------------------------------------------------------

.PHONY: build_tests
ifeq ($(TARGET_OS),wii)
build_tests : $(FBCU_BIN) ./$(MAINBAS).o $(OBJLIST) $(UNIT_TESTS_OBJ_LST) $(UNIT_TESTS_OBJ_LIB)
	$(WII_CC) -o $(MAINBAS).elf -L "$(WII_FB_LIBDIR)" -L "$(FBCU_LIB)" -L "." -L "$(WII_DEVKITPRO)/libogc/lib/wii" "$(WII_FB_LIBDIR)/fbrt0.o" ./$(MAINBAS).o -Wl,--whole-archive $(UNIT_TESTS_OBJ_LIB) -Wl,--no-whole-archive -Wl,--start-group -lfbcunit -lfbmt -lfbgfx -lfat -lwiiuse -lbte -lasnd -logc -lm -lc -lgcc -Wl,--end-group -mrvl -mcpu=750 -meabi -mhard-float
	$(WII_ELF2DOL) $(MAINBAS).elf $(MAINEXE)
else
build_tests : $(FBCU_BIN) ./$(MAINBAS).o $(OBJLIST) $(UNIT_TESTS_OBJ_LST)
	$(FBC) $(FBC_LFLAGS) @$(UNIT_TESTS_OBJ_LST) ./$(MAINBAS).o
endif

$(UNIT_TESTS_OBJ_LIB) : $(UNIT_TESTS_OBJ_LST) $(OBJLIST)
	$(RM) $@
	$(CAT) $(UNIT_TESTS_OBJ_LST) | $(XARGS) $(AR) rcs $@

.PHONY: run_tests
run_tests : build_tests
ifeq ($(HOST),dos)
	$(MAINEXE) $(UNITTEST_RUN_ARGS)
else ifneq ($(TARGET_OS),js)
	./$(MAINEXE) $(UNITTEST_RUN_ARGS)
else ifneq ($(NODEJS),)
	$(NODEJS) ./$(MAINEXE) $(UNITTEST_RUN_ARGS)
endif

.PHONY: clean
clean : clean_main_exe clean_tests clean_fbcu clean_include

.PHONY: mostlyclean
mostlyclean : clean_main_exe clean_tests clean_fbcu

.PHONY: clean_main_exe
clean_main_exe :
	$(RM) $(MAINEXE) $(MAINBAS).elf

.PHONY: clean_tests
clean_tests : $(UNIT_TESTS_OBJ_LST)
	@$(ECHO) Cleaning unit-tests files ...
	@$(RM) ./$(MAINBAS).o $(UNIT_TESTS_OBJ_LIB)
	@if [ -s $(UNIT_TESTS_OBJ_LST) ]; then $(CAT) $(UNIT_TESTS_OBJ_LST) | $(XARGS) $(RM) ; fi

.PHONY: clean_fbcu
clean_fbcu :
	cd $(FBCU_DIR) && $(MAKE) clean

.PHONY: clean_include
clean_include : clean_tests
	$(RM) $(UNIT_TESTS_INC) $(UNIT_TESTS_OBJ_LST)
