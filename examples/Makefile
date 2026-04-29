#!/usr/bin/make -f
#
# examples makefile - attempts to build all examples
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

OS := $(shell uname)
# Handle MSYS2/MinGW which returns MSYS_NT-* or MINGW64_NT-*
ifneq (,$(findstring NT,$(OS)))
    EXEEXT := .exe
    DLLEXT := .dll
    WINDOWS := 1
else ifeq ($(OS),win32)
    EXEEXT := .exe
    DLLEXT := .dll
    WINDOWS := 1
else
    EXEEXT :=
    DLLEXT := .so
    WINDOWS :=
endif

FBC := fbc$(EXEEXT)
STOP_ON_ERROR := n



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


ALL_FILES := \
	$(BASICS) $(COMPRESSION) $(CONSOLE) $(DATABASE) \
	$(DLL) $(DOS) $(FILES) $(GRAPHICS) $(GUI) $(MANUAL) \
	$(MATH) $(MISC) $(NETWORK) $(OPTIMIZE) $(REGEX) $(SFXLIB) $(SOUND) \
	$(THREADS) $(UNICODE) $(WIN32) $(XML)

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
	@ldconfig -p | grep "lib$*.so" > /dev/null


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
	@rm -fv $(ALL_FILES)
