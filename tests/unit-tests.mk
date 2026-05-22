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
UNIT_TESTS_EXPLICIT_SRCS := \
./pp/macro-arg-listexpand-utf16le.bas \
./pp/macro-eval-str-utf16le.bas \
./pp/quote-utf16be.bas \
./pp/quote-utf16le.bas \
./string/asc-utf16le.bas
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
# DOS runs the same coverage through smaller select_const2-dos-*.bas shards.
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
FBCU_INC := $(abspath $(FBCU_DIR)/inc)
FBCU_LIB := $(abspath $(FBCU_DIR)/lib)
FBCU_BIN := $(FBCU_LIB)/libfbcunit.a
FBCU_MAKE := $(MAKE)
ifeq ($(HOST),dos)
	FBCU_MAKE := make.exe
endif

FBCU_LIBS := -l fbcunit

ifeq ($(TARGET_OS),win32)
    FBCU_LIBS += -l user32
endif

FBC_CFLAGS := -c -w 3 -i $(FBCU_INC) -m $(MAINBAS)
ifeq ($(TARGET_OS),dos)
	FBC_CFLAGS += -i $(abspath ../inc)
endif
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

ifeq ($(ENABLE_CHECK_BUGS),1)
	FBC_CFLAGS += -d ENABLE_CHECK_BUGS=$(ENABLE_CHECK_BUGS)
endif
ifeq ($(ENABLE_CONSOLE_OUTPUT),1)
	FBC_CFLAGS += -d ENABLE_CONSOLE_OUTPUT=$(ENABLE_CONSOLE_OUTPUT)
endif

OBJLIST := $(SRCLIST:%.bas=%.o)

ifeq ($(TESTS_HOST_OS),darwin)
./functions/var_args-gcc.o functions/var_args-gcc.o: FBC_CFLAGS += -Wc -Wno-varargs
./optimizations/consteval.o optimizations/consteval.o: FBC_CFLAGS += -Wc -Wno-absolute-value
endif

#
#: rules
#

%.o : %.bas
	$(FBC) $(FBC_CFLAGS) $^

#
#: targets
#

.PHONY: all
all : make_fbcunit $(UNIT_TESTS_OBJ_LST) build_tests run_tests

.PHONY: make_fbcunit
make_fbcunit : $(FBCU_BIN)

$(FBCU_BIN) :
	$(FBCU_MAKE) -C $(FBCU_DIR) FBC="$(FBC)" FPU=$(FPU) ARCH=$(ARCH) TARGET=$(TARGET)

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
| $(SED) 's/^SRCLIST += \(.*\)\(\.b.*\)/\1\.o/g' \
$(if $(filter dos,$(TARGET_OS)),-e '/^\.\/interactive\//d' -e '/^\.\/threads\//d' -e '/^\.\/compound\/select_const2\.o/d') \
> $(UNIT_TESTS_OBJ_LST)
	@$(PRINTF) '%s\n' $(UNIT_TESTS_EXPLICIT_OBJS) >> $(UNIT_TESTS_OBJ_LST)

# ------------------------------------------------------------------------

.PHONY: build_tests
build_tests : $(FBCU_BIN) ./$(MAINBAS).o $(OBJLIST) $(UNIT_TESTS_OBJ_LST)
	$(FBC) $(FBC_LFLAGS) @$(UNIT_TESTS_OBJ_LST) ./$(MAINBAS).o

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
	$(RM) $(MAINEXE)

.PHONY: clean_tests
clean_tests : $(UNIT_TESTS_OBJ_LST)
	@$(ECHO) Cleaning unit-tests files ...
	@$(RM) ./$(MAINBAS).o
	@if [ -s $(UNIT_TESTS_OBJ_LST) ]; then $(CAT) $(UNIT_TESTS_OBJ_LST) | $(XARGS) $(RM) ; fi

.PHONY: clean_fbcu
clean_fbcu :
	cd $(FBCU_DIR) && $(MAKE) clean

.PHONY: clean_include
clean_include : clean_tests
	$(RM) $(UNIT_TESTS_INC) $(UNIT_TESTS_OBJ_LST)
