FBC ?= fbc

ECHO := echo
RM ?= rm -f

ifeq ($(OS),Windows_NT)
	EXEEXT := .exe
else
	EXEEXT :=
endif

LIBRARY := lib/libfbcunit.a
SRCS    := src/fbcunit.bas
SRCS    += src/fbcunit_qb.bas
SRCS    += src/fbcunit_console.bas
SRCS    += src/fbcunit_report.bas
LIBRARY_OBJS := $(patsubst %.bas,%.o,$(SRCS))

HDRS    := inc/fbcunit.bi
HDRS    += src/fbcunit_types.bi
HDRS    += src/fbcunit_console.bi
HDRS    += src/fbcunit_report.bi

TEST_SRCS := tests/tests.bas
TEST_SRCS += tests/fbcu_sanity.bas
TEST_SRCS += tests/fbcu_multiple.bas
TEST_SRCS += tests/fbcu_global.bas
TEST_SRCS += tests/fbcu_many_tests.bas
TEST_SRCS += tests/fbcu_many_suites.bas
TEST_SRCS += tests/fbcu_append.bas
TEST_SRCS += tests/fbcu_append2.bas
TEST_SRCS += tests/fbcu_float.bas
TEST_SRCS += tests/fbcu_namespace.bas
TEST_SRCS += tests/fbcu_default.bas
TEST_SRCS += tests/fbcu_order.bas
TEST_SRCS += tests/fbcu_cases.bas
TEST_SRCS += tests/fbcu_console.bas

TEST_OBJS := $(patsubst %.bas,%.o,$(TEST_SRCS))

TEST_EXE  := tests/tests$(EXEEXT)

EXAMPLES := examples/ex01$(EXEEXT)
EXAMPLES += examples/ex02$(EXEEXT)
EXAMPLES += examples/ex03$(EXEEXT)
EXAMPLES += examples/ex04$(EXEEXT)
EXAMPLES += examples/ex05$(EXEEXT)
EXAMPLES += examples/ex06$(EXEEXT)
EXAMPLES += examples/ex07$(EXEEXT)
EXAMPLES += examples/ex08$(EXEEXT)
EXAMPLES += examples/ex09$(EXEEXT)
EXAMPLES += examples/ex10$(EXEEXT)
EXAMPLES += examples/ex11$(EXEEXT)

ifneq ($(ARCH),)
	FBCFLAGS += -arch $(ARCH)
endif
ifneq ($(TARGET),)
	FBCFLAGS += -target $(TARGET)
endif
ifneq ($(findstring riscos,$(TARGET)),)
	FBCFLAGS += -i ../../inc/riscos
	SRCS := src/riscos/fbcunit.bas $(filter-out src/fbcunit.bas,$(SRCS))
endif
ifneq ($(findstring aros,$(TARGET)),)
	FBCFLAGS += -i ../../inc/aros
endif
ifneq ($(findstring wince,$(TARGET)),)
	FBCFLAGS += -i ../../inc/wince
endif
ifneq ($(FPU),)
	FBCFLAGS += -fpu $(FPU)
endif

ifneq ($(TARGET),dos)
ifneq ($(TARGET),wince-mips)
FBCFLAGS += -mt
endif
endif
ifneq ($(TARGET),wince-mips)
FBCFLAGS += -g
endif
FBCFLAGS += -i ../../inc -i inc
FBCFLAGS += -i src

.SUFFIXES: .bas

VPATH = .

.PHONY: all
all: library

.PHONY: help
help:
	@$(ECHO) "usage: make target [options]"
	@$(ECHO) ""
	@$(ECHO) "Targets:"
	@$(ECHO) "   help           - displays this information"
	@$(ECHO) "   library        - builds $(LIBRARY)"
	@$(ECHO) "   tests          - builds tests for fbcunit"
	@$(ECHO) "   examples       - builds all the examples"
	@$(ECHO) "   everything     - builds library, tests, examples"
	@$(ECHO) "   clean          - cleans up all built files"
	@$(ECHO) ""
	@$(ECHO) "Options:"
	@$(ECHO) "   FBC=/path/fbc  - set path to FBC compiler"
	@$(ECHO) "   TARGET=target"
	@$(ECHO) "   ARCH=arch (default is 486)"
	@$(ECHO) "   FPU=fpu | sse"

.PHONY: everything
everything: library tests examples

.PHONY: library
library: $(LIBRARY)

.PHONY: tests
tests: $(TEST_EXE) 

.PHONY: examples
examples: $(EXAMPLES)

$(LIBRARY): $(SRCS) $(HDRS)
ifeq ($(TARGET),dos)
	$(FBC) $(FBCFLAGS) -d FBCU_NO_INCLIB -c src/fbcunit.bas -o src/fbcunit.o
	$(FBC) $(FBCFLAGS) -d FBCU_NO_INCLIB -c src/fbcunit_qb.bas -o src/fbcunit_qb.o
	$(FBC) $(FBCFLAGS) -d FBCU_NO_INCLIB -c src/fbcunit_console.bas -o src/fbcunit_console.o
	$(FBC) $(FBCFLAGS) -d FBCU_NO_INCLIB -c src/fbcunit_report.bas -o src/fbcunit_report.o
	ar rcs $@ $(LIBRARY_OBJS)
else
	$(FBC) $(FBCFLAGS) -d FBCU_NO_INCLIB -lib $(SRCS) -x $@
endif

tests/%.o: tests/%.bas $(HDRS)
	$(FBC) $(FBCFLAGS) -m tests -c $< -o $@

examples/%$(EXEEXT): examples/%.bas $(HDRS) $(LIBRARY)
	$(FBC) $(FBCFLAGS) $< -p $(abspath lib) -x $@

$(TEST_EXE): $(TEST_OBJS) $(LIBRARY)
	$(FBC) $(FBCFLAGS) $(TEST_OBJS) -p $(abspath lib) -x $@

.PHONY: clean
clean:
	-$(RM) $(LIBRARY)
	-$(RM) $(LIBRARY_OBJS)
	-$(RM) $(TEST_OBJS) $(TEST_EXE)
	-$(RM) $(EXAMPLES)
