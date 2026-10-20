##############################################################################
# Project: FreeBASIC build system
# File: build-targets.mk
#
# Purpose: Define high-level build targets and compiler link dependencies.
# Responsibilities: Select embedded runtime inputs and native platform libraries.
# This file intentionally does NOT compile sources or define archive contents.
#
# Consumes artifact definitions from archives.mk
#
# Includes portable terminal library handling
##############################################################################

##############################################################################
# Terminal library selection
##############################################################################

TERM_LIB := -lncurses

ifneq ($(strip $(DISABLE_NCURSES)),)
TERM_LIB :=
endif

ifeq ($(TARGET_OS),haiku)
#
# Haiku may provide the wide-character ncurses runtime as versioned shared
# objects without the unversioned libncursesw.so linker symlink.
#
# Prefer the normal -lncursesw form when the toolchain accepts it.
# Otherwise fall back to the installed soname path so linking works without
# requiring users to create local compatibility symlinks.
#
HAIKU_NCURSESW_LINKABLE := $(shell printf 'int main(void){return 0;}\n' | $(CC) -x c -o /dev/null - -lncursesw >/dev/null 2>&1 && echo yes)
HAIKU_NCURSESW_SONAME := $(firstword $(wildcard /boot/system/lib/libncursesw.so.* /boot/home/config/non-packaged/lib/libncursesw.so.*))

ifeq ($(HAIKU_NCURSESW_LINKABLE),yes)
TERM_LIB := -lncursesw
else ifneq ($(strip $(HAIKU_NCURSESW_SONAME)),)
TERM_LIB := $(HAIKU_NCURSESW_SONAME)
endif
endif

ifeq ($(TARGET_OS),dos)
TERM_LIB :=
endif

# Native Windows console support does not use the Unix ncurses library.
ifneq ($(filter win32 wince,$(TARGET_OS)),)
TERM_LIB :=
endif

# UnixLib contains the termcap entry points used by the console layer.
ifeq ($(TARGET_OS),riscos)
TERM_LIB :=
endif

# AROS console support is native and does not use ncurses.
ifeq ($(TARGET_OS),aros)
TERM_LIB :=
endif

##############################################################################
# Phony targets
##############################################################################

.PHONY: \
	compiler \
	compiler-js \
	compiler-android \
	compiler-wii \
	rtlib \
	fbrt \
	gfxlib2 \
	gfxlib3 \
	sfxlib \
	libs \
	runtime

##############################################################################
# Aggregate runtime
##############################################################################

runtime: runtime-libs

##############################################################################
# Runtime layers
##############################################################################

rtlib: $(RTL_LIBS)

fbrt: $(FBRTL_LIBS)

gfxlib2: $(GFX_LIBS)

gfxlib3: $(GFX3_LIBS)

sfxlib: $(SFX_LIBS)

##############################################################################
# Convenience target
##############################################################################

libs: rtlib fbrt gfxlib2 gfxlib3 sfxlib

##############################################################################
# Compiler executable
##############################################################################

ifeq ($(ENABLE_PIE),YesPlease)
  COMPILER_RT0 := $(libdir)/fbrt0pic.o
  COMPILER_RTL := $(libdir)/libfbpic.a
else
  COMPILER_RT0 := $(libdir)/fbrt0.o
  COMPILER_RTL := $(libdir)/libfb.a
endif

# A Watt-32 DOS runtime also needs its provider archive in the compiler
# executable. The compiler's own driver adds Watt-32 when it links user
# programs, but that does not resolve references in this embedded runtime.
COMPILER_PLATFORM_LIBS :=
ifeq ($(TARGET_OS),dos)
ifdef ENABLE_DOS_WATT32
COMPILER_PLATFORM_LIBS += $(DOS_WATT32_LIBS) -Wl,--allow-multiple-definition
endif
endif

# The compiler only links against rtlib.  Keeping optional graphics and sound
# libraries out of this order-only prerequisite lets a compiler be rebuilt on
# a small bootstrap host before those platform backends are available.
# These files are embedded in each compiler executable. They must be normal
# prerequisites so a runtime repair relinks an otherwise unchanged compiler.
COMPILER_LINK_INPUTS := $(COMPILER_RT0) $(COMPILER_RTL)

$(FBC_EXE): $(FBC_OBJS) $(COMPILER_LINK_INPUTS) | rtlib
	@mkdir -p "$(dir $@)"
	@echo "Linking compiler: $@"
	$(RUN_CC) $(ALLLDFLAGS) $(FBC_PIE_LDFLAGS) -o $@ \
		$(FBC_OBJS) \
		$(COMPILER_RT0) \
		$(COMPILER_RTL) \
		-lm \
		$(THREAD_FLAGS) \
		$(TERM_LIB) \
		$(COMPILER_PLATFORM_LIBS)

$(FBC_JS_EXE): $(FBC_JS_OBJS) $(COMPILER_LINK_INPUTS) | rtlib
	@mkdir -p "$(dir $@)"
	@echo "Linking JS-targeting compiler: $@"
	$(RUN_CC) $(ALLLDFLAGS) $(FBC_PIE_LDFLAGS) -o $@ \
		$(FBC_JS_OBJS) \
		$(COMPILER_RT0) \
		$(COMPILER_RTL) \
		-lm \
		$(THREAD_FLAGS) \
		$(TERM_LIB)

$(FBC_ANDROID_EXE): $(FBC_ANDROID_OBJS) $(COMPILER_LINK_INPUTS) | rtlib
	@mkdir -p "$(dir $@)"
	@echo "Linking Android-targeting compiler: $@"
	$(RUN_CC) $(ALLLDFLAGS) $(FBC_PIE_LDFLAGS) -o $@ \
		$(FBC_ANDROID_OBJS) \
		$(COMPILER_RT0) \
		$(COMPILER_RTL) \
		-lm \
		$(THREAD_FLAGS) \
		$(TERM_LIB)

$(FBC_WII_EXE): $(FBC_WII_OBJS) $(COMPILER_LINK_INPUTS) | rtlib
	@mkdir -p "$(dir $@)"
	@echo "Linking Wii-targeting compiler: $@"
	$(RUN_CC) $(ALLLDFLAGS) $(FBC_PIE_LDFLAGS) -o $@ \
		$(FBC_WII_OBJS) \
		$(COMPILER_RT0) \
		$(COMPILER_RTL) \
		-lm \
		$(THREAD_FLAGS) \
		$(TERM_LIB)

##############################################################################
# User-facing compiler target
##############################################################################

compiler: $(FBC_EXE)

compiler-js: $(FBC_JS_EXE)

compiler-android: $(FBC_ANDROID_EXE)

compiler-wii: $(FBC_WII_EXE)

##############################################################################
# END build-targets.mk
##############################################################################
