# examples GNUmakefile - attempts to build all examples
#
# Several examples will not build if their libraries are not
# present.
#
# To make all examples:
#   $ make
# To make a subset, use the dirname in lower case
#   $ make unicode
#
# If fbc[.exe] is not in your path, pass it to make
#   $ make FBC=d:/path/to/fbc
#

UNAME_S := $(shell uname -s 2>/dev/null || echo unknown)

ifneq (,$(filter MINGW% MSYS% CYGWIN%,$(UNAME_S)))
	EXEEXT := .exe
	DLLEXT := .dll
	WINDOWS := 1
else ifeq ($(OS),Windows_NT)
	EXEEXT := .exe
	DLLEXT := .dll
	WINDOWS := 1
else ifeq ($(UNAME_S),Darwin)
	EXEEXT :=
	DLLEXT := .dylib
	WINDOWS :=
else
	EXEEXT :=
	DLLEXT := .so
	WINDOWS :=
endif

FBC ?= fbc$(EXEEXT)
CC ?= gcc
LD ?= ld
STOP_ON_ERROR ?= n
RM ?= rm -f
EXAMPLES_FBC_TARGET := $(strip $(shell $(FBC) -print target 2>/dev/null))



BASICS		:= $(subst .bas,,$(wildcard *.bas))
COMPRESSION	:= $(subst .bas,,$(wildcard compression/*.bas))
CONSOLE		:= $(subst .bas,,$(wildcard console/*.bas)) \
			$(subst .bas,,$(wildcard console/curses/*.bas)) \
			$(subst .bas,,$(wildcard console/caca/*.bas))
DATABASE	:= $(subst .bas,,$(wildcard database/*.bas))
DLL		:= dll/dylib dll/libmydll$(DLLEXT) dll/test
DOS		:= $(subst .bas,,$(wildcard DOS/*.bas))
FILES		:= $(subst .bas,,$(wildcard files/*.bas files/*/*.bas))
GRAPHICS	:= $(subst .bas,,$(wildcard graphics/*.bas graphics/*/*.bas))
GUI		:= $(subst .bas,,$(wildcard GUI/*/*.bas GUI/*/*/*.bas))
MANUAL		:= $(subst .bas,,$(wildcard manual/*.bas manual/*/*.bas))
MATH		:= $(subst .bas,,$(wildcard math/*.bas math/*/*.bas))
MISC		:= $(subst .bas,,$(wildcard misc/*/*.bas))
NETWORK		:= $(subst .bas,,$(wildcard network/*.bas network/*/*.bas \
			network/*/*/*.bas))
OPTIMIZE	:= $(subst .bas,,$(wildcard OptimizePureAbstractTypes/*.bas))
REGEX		:= $(subst .bas,,$(wildcard regex/*.bas regex/*/*.bas))
SFXLIB		:= $(subst .bas,,$(wildcard sfxlib/*.bas))
SOUND		:= $(subst .bas,,$(wildcard sound/*.bas)) \
			$(subst .bas,,$(wildcard sound/*/*.bas))
THREADS		:= $(subst .bas,,$(wildcard threads/*.bas)) \
			threads/timer-lib/libtimer$(DLLEXT) threads/timer-lib/test
UNICODE		:= $(subst .bas,,$(wildcard unicode/*.bas))
WIN32		:= $(subst .bas,,$(wildcard win32/*.bas win32/*/*.bas win32/*/*/*.bas))
XML		:= $(subst .bas,,$(wildcard xml/*.bas))

LOCAL_MODULES := \
	misc/trycatch/trycatch \
	network/curl/CHttp/CHttp \
	network/curl/CHttp/CHttpForm \
	network/curl/CHttp/CHttpStream \
	win32/COM/DropTarget/CDropTarget \
	win32/GDIPlus/frmwrk \
	win32/winmm/winmmhlp \
	$(subst .bas,,$(wildcard win32/variants/libvariant/*.bas))

LOCAL_LIBS := \
	network/curl/CHttp/libCHttp.a \
	win32/COM/MoviePlayer/libmovctrl.a \
	win32/COM/WebBrowser/libwebctrl.a \
	win32/variants/libvariant.a

MANUAL_HELPERS := \
	manual/module/common2 \
	manual/module/extern1 \
	manual/procs/mydll

INTENTIONAL_FAILURES := \
	manual/module/import \
	manual/module/opts \
	manual/prepro/inclib \
	manual/prepro/error \
	manual/prepro/pragma_reserve1 \
	manual/prepro/pragma_reserve2 \
	manual/prepro/pragma_reserve3 \
	manual/proguide/linecontinuation2 \
	win32/ddk/driver/driver

MISC := $(filter-out $(LOCAL_MODULES),$(MISC))
MANUAL := $(filter-out $(INTENTIONAL_FAILURES) $(MANUAL_HELPERS),$(MANUAL))
NETWORK := $(filter-out $(LOCAL_MODULES),$(NETWORK))
WIN32 := $(filter-out $(LOCAL_MODULES) $(INTENTIONAL_FAILURES),$(WIN32))


ALL_FILES := \
	$(BASICS) $(COMPRESSION) $(CONSOLE) $(DATABASE) \
	$(DLL) $(DOS) $(FILES) $(GRAPHICS) $(GUI) $(MANUAL) \
	$(MATH) $(MISC) $(NETWORK) $(OPTIMIZE) $(REGEX) $(SFXLIB) $(SOUND) \
	$(THREADS) $(UNICODE) $(WIN32) $(XML) $(LOCAL_LIBS)

all: $(ALL_FILES)

basics: $(BASICS)
compression: $(COMPRESSION)
console: $(CONSOLE)
database: $(DATABASE)
dll: $(DLL)
dos: $(DOS)
files: $(FILES)
graphics: $(GRAPHICS)
gui: $(GUI)
manual: $(MANUAL)
math: $(MATH)
misc: $(MISC)
#network: checklib.curl checklib.CHttp .WAIT $(NETWORK)
network: $(NETWORK)
optimize: $(OPTIMIZE)
regex: $(REGEX)
sfxlib: $(SFXLIB)
sound: $(SOUND)
threads: $(THREADS)
unicode: $(UNICODE)
win32: $(WIN32)
xml: $(XML)


checklib.%:
	@echo check library: "lib$*"
	@if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists "$*"; then \
		true; \
	elif command -v ldconfig >/dev/null 2>&1 && ldconfig -p 2>/dev/null | grep "lib$*\\.so" >/dev/null; then \
		true; \
	else \
		echo "missing library: lib$*"; \
		false; \
	fi


dll/libmydll$(DLLEXT): dll/mydll.bas
	$(FBC) -dylib $<

dll/test: dll/test.bas dll/libmydll$(DLLEXT)
	$(FBC) -p dll dll/test.bas

ifeq ($(WINDOWS),1)
DOS/%: DOS/%.bas
	@echo "SKIP $@ on Windows"
else
DOS/%: DOS/%.bas
	$(FBC) $<
endif

threads/timer-lib/libtimer$(DLLEXT): threads/timer-lib/timer.bas
	$(FBC) -dylib -mt $<

threads/timer-lib/test: threads/timer-lib/test.bas threads/timer-lib/libtimer$(DLLEXT)
	$(FBC) -p $(dir $@) -mt $<

misc/trycatch/test: misc/trycatch/test.bas misc/trycatch/trycatch.bas misc/trycatch/trycatch.bi
	$(FBC) -p $(dir $@) $< misc/trycatch/trycatch.bas -x $@

manual/module/common1: manual/module/common1.bas manual/module/common2.bas
	$(FBC) -p $(dir $@) $< manual/module/common2.bas -x $@

manual/module/extern2: manual/module/extern2.bas manual/module/extern1.bas
	$(FBC) -p $(dir $@) $< manual/module/extern1.bas -x $@

manual/module/option_entry: manual/module/option_entry.bas
	$(FBC) -p $(dir $@) $< -entry custom_main -x $@

manual/procs/mydll$(DLLEXT): manual/procs/mydll.bas
	$(FBC) -dll $< -x $@

manual/procs/lib: manual/procs/lib.bas manual/procs/mydll$(DLLEXT)
	$(FBC) -p $(dir $@) $< -x $@

manual/procs/vararg1: manual/procs/vararg1.bas
	@$(FBC) -p $(dir $@) $< -x $@ >/dev/null 2>&1 || $(FBC) -gen gas64 -p $(dir $@) $< -x $@

manual/procs/vararg2: manual/procs/vararg2.bas
	@$(FBC) -p $(dir $@) $< -x $@ >/dev/null 2>&1 || $(FBC) -gen gas64 -p $(dir $@) $< -x $@

manual/proguide/hello.txt:
	printf '%s\n' "Hello from embedded data." > $@

manual/proguide/hello_txt.o: manual/proguide/hello.txt
	cd manual/proguide && $(LD) -r -b binary -o hello_txt.o hello.txt

manual/proguide/embed-access-data-in-executable: manual/proguide/embed-access-data-in-executable.bas manual/proguide/hello_txt.o
	$(FBC) -p $(dir $@) $< manual/proguide/hello_txt.o -x $@

network/curl/CHttp/libCHttp.a: network/curl/CHttp/CHttp.bas network/curl/CHttp/CHttpStream.bas network/curl/CHttp/CHttpForm.bas
	$(FBC) -lib -p $(dir $@) $^ -x $@

network/curl/CHttp/test: network/curl/CHttp/test.bas network/curl/CHttp/libCHttp.a
	$(FBC) -p $(dir $@) $< -x $@

win32/COM/DropTarget/test: win32/COM/DropTarget/test.bas win32/COM/DropTarget/CDropTarget.bas win32/COM/DropTarget/CDropTarget.bi
	$(FBC) -p $(dir $@) $< win32/COM/DropTarget/CDropTarget.bas -x $@

win32/GDIPlus/circle: win32/GDIPlus/circle.bas win32/GDIPlus/frmwrk.bas win32/GDIPlus/frmwrk.bi
	$(FBC) -p $(dir $@) $< win32/GDIPlus/frmwrk.bas -x $@

ifeq ($(WINDOWS),1)
GUI/GTK+/GooCanvas/%: GUI/GTK+/GooCanvas/%.bas
	$(FBC) -d __USE_GTK3__ -p $(dir $@) $<
endif

files/FreeImage/%: files/FreeImage/%.bas
	$(FBC) -p $(dir $@) $<

manual/extras/freeimage: manual/extras/freeimage.bas
	$(FBC) -p $(dir $@) $< -x $@

ifeq ($(EXAMPLES_FBC_TARGET),win32)
files/GD/%: files/GD/%.bas
	$(FBC) -d NONDLL -p $(dir $@) $<
else
files/GD/%: files/GD/%.bas
	$(FBC) -p $(dir $@) $<
endif

manual/libraries/cryptlib: manual/libraries/cryptlib.bas
	$(FBC) -d CRYPTLIB_STATICLINK -p $(dir $@) $< -x $@

math/cryptlib/%: math/cryptlib/%.bas
	$(FBC) -d CRYPTLIB_STATICLINK -p $(dir $@) $<

ifeq ($(EXAMPLES_FBC_TARGET),win64)
math/ODE/%: math/ODE/%.bas
	$(FBC) -d dDOUBLE -p $(dir $@) $<
endif

regex/PCRE/preg_class: regex/PCRE/preg_class.bas
	$(FBC) -d PCRE_STATIC -p $(dir $@) $<

win32/COM/MoviePlayer/libmovctrl.a:
	$(MAKE) -C win32/COM/MoviePlayer/movctrl FBC="$(FBC)"

win32/COM/MoviePlayer/test: win32/COM/MoviePlayer/test.bas win32/COM/MoviePlayer/libmovctrl.a
	$(FBC) -p $(dir $@) $< -x $@

win32/COM/WebBrowser/libwebctrl.a:
	$(MAKE) -C win32/COM/WebBrowser/webctrl FBC="$(FBC)"

win32/COM/WebBrowser/test: win32/COM/WebBrowser/test.bas win32/COM/WebBrowser/libwebctrl.a
	$(FBC) -p $(dir $@) $< -x $@

win32/variants/libvariant.a:
	$(MAKE) -C win32/variants/libvariant FBC="$(FBC)"

%: %.bas
ifeq ($(STOP_ON_ERROR),n)
	@echo fbc -p $(dir $@) $<
	@$(FBC) -p $(dir $@) $< || \
		echo "Failed to compile '$<' - check for missing libraries"
else
	$(FBC) -p $(dir $@) $<
endif

clean:
	@echo CLEAN binary files
	@$(RM) $(ALL_FILES)
