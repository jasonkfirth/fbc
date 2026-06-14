########################
# toolchain-flags.mk
########################
#
# Toolchain flag realization layer
#
# Responsibilities
#   - translate platform feature flags into toolchain flags
#   - provide compiler defaults
#   - apply optional distro hardening
#   - preserve override semantics
#
# Inputs
#   TARGET_OS
#   TARGET_ARCH
#   ENABLE_* feature flags (platform-features.mk)
#
########################


##############################################################################
# Guard
##############################################################################

ifeq ($(strip $(TARGET_OS)),)
$(error toolchain-flags.mk: TARGET_OS undefined (include platform.mk first))
endif


##############################################################################
# Toolchain executables
##############################################################################

CC      ?= gcc
CXX     ?= g++
AR      ?= ar
ARFLAGS ?= rcs
RANLIB  ?= ranlib
ELF2DOL ?= elf2dol

tool_cmd = $(firstword $(strip $(1)))
tool_args = $(wordlist 2,$(words $(strip $(1))),$(strip $(1)))
tool_path = $(subst \,/,$(call tool_cmd,$(1)))
tool_bindir = $(strip $(if $(findstring /,$(call tool_path,$(1))),$(patsubst %/,%,$(dir $(call tool_path,$(1))))))

TOOLCHAIN_CC_TOOL := $(notdir $(firstword $(CC)))
TOOLCHAIN_CC_IS_CLANG :=
ifneq ($(findstring clang,$(TOOLCHAIN_CC_TOOL)),)
TOOLCHAIN_CC_IS_CLANG := yes
endif

# OpenBSD requires GCC (clang incompatible with fbc output)
ifeq ($(TARGET_OS),openbsd)
  CC  := egcc
  CXX := eg++
endif


##############################################################################
# Dragonfly exception
##############################################################################

ifeq ($(TARGET_OS),dragonfly)
  # Ports
  CPPFLAGS += -I/usr/local/include
  CFLAGS   += -I/usr/local/include
  LDFLAGS  += -L/usr/local/lib
  LDFLAGS += -Wl,-R/usr/local/lib

  # Ports
  CPPFLAGS += -I/usr/local/include/ncurses
  CFLAGS   += -I/usr/local/include/ncurses
  LDFLAGS  += -L/usr/local/lib/ncurses
  LDFLAGS += -Wl,-R/usr/local/lib/ncurses


  # X11
  CPPFLAGS += -I/usr/X11R7/include
  CFLAGS   += -I/usr/X11R7/include
  LDFLAGS  += -L/usr/X11R7/lib
 
  # libexecinfo (needed on OpenBSD)
#  LDFLAGS  += -lexecinfo
endif

ifeq ($(TARGET_OS),netbsd)

  # pkgsrc
  CPPFLAGS += -I/usr/pkg/include
  CFLAGS   += -I/usr/pkg/include
  LDFLAGS  += -L/usr/pkg/lib
  LDFLAGS += -Wl,-R/usr/pkg/lib

  # X11
  CPPFLAGS += -I/usr/X11R7/include
  CFLAGS   += -I/usr/X11R7/include
  LDFLAGS  += -L/usr/X11R7/lib

  # libexecinfo (needed on OpenBSD)
#  LDFLAGS  += -lexecinfo
endif

##############################################################################
# illumos exception
##############################################################################

ifeq ($(TARGET_OS),illumos)

  # OmniOS OOCE
  ifeq ($(TARGET_ARCH),x86_64)
    ILLUMOS_OOCE_LIBDIR := /opt/ooce/lib/amd64
  else
    ILLUMOS_OOCE_LIBDIR := /opt/ooce/lib
  endif

  CPPFLAGS += -I/opt/ooce/include
  CFLAGS   += -I/opt/ooce/include
  LDFLAGS  += -L$(ILLUMOS_OOCE_LIBDIR)
  LDFLAGS  += -Wl,-R$(ILLUMOS_OOCE_LIBDIR)
endif

 

##############################################################################
# OpenBSD exception
##############################################################################

ifeq ($(TARGET_OS),openbsd)

  # Ports
  CPPFLAGS += -I/usr/local/include
  CFLAGS   += -I/usr/local/include
  LDFLAGS  += -L/usr/local/lib

  # X11
  CPPFLAGS += -I/usr/X11R6/include
  CFLAGS   += -I/usr/X11R6/include
  LDFLAGS  += -L/usr/X11R6/lib

  # libexecinfo (needed on OpenBSD)
  LDFLAGS  += -lexecinfo
endif


##############################################################################
# Base warnings and portability
##############################################################################

BASE_WARN_CFLAGS := \
  -Wall \
  -Wextra \
  -Wno-unused-parameter \
  -Werror=implicit-function-declaration

BASE_WARN_CXXFLAGS := \
  -Wall \
  -Wextra \
  -Wno-unused-parameter

BASE_PORTABILITY_CFLAGS := \
  -fno-strict-aliasing

BASE_PORTABILITY_CXXFLAGS := \
  -fno-strict-aliasing

BASE_CFLAGS := \
  $(BASE_WARN_CFLAGS) \
  $(BASE_PORTABILITY_CFLAGS)

BASE_CXXFLAGS := \
  $(BASE_WARN_CXXFLAGS) \
  $(BASE_PORTABILITY_CXXFLAGS)


##############################################################################
# Hardening implementation
##############################################################################

HARDEN_CFLAGS :=
HARDEN_CXXFLAGS :=
HARDEN_LDFLAGS :=


##############################################################################
# Stack protection
##############################################################################

ifdef ENABLE_STACK_PROTECTOR
HARDEN_CFLAGS  += -fstack-protector-strong
HARDEN_CXXFLAGS += -fstack-protector-strong
endif


##############################################################################
# Fortify (glibc bounds checking)
##############################################################################

FORTIFY_PRESET := $(findstring _FORTIFY_SOURCE,$(CPPFLAGS) $(CFLAGS) $(CXXFLAGS))

ifdef ENABLE_FORTIFY
ifeq ($(strip $(FORTIFY_PRESET)),)
HARDEN_CFLAGS  += -D_FORTIFY_SOURCE=2
HARDEN_CXXFLAGS += -D_FORTIFY_SOURCE=2
endif
endif


##############################################################################
# Stack clash protection
##############################################################################

ifdef ENABLE_STACK_CLASH
HARDEN_CFLAGS  += -fstack-clash-protection
HARDEN_CXXFLAGS += -fstack-clash-protection
endif


##############################################################################
# Format string security
##############################################################################

ifdef ENABLE_FORMAT_SECURITY
HARDEN_CFLAGS  += -Wformat -Werror=format-security
HARDEN_CXXFLAGS += -Wformat -Werror=format-security
endif


##############################################################################
# RELRO
##############################################################################

ifdef ENABLE_RELRO
HARDEN_LDFLAGS += -Wl,-z,relro
endif


##############################################################################
# Immediate binding
##############################################################################

ifdef ENABLE_NOW
HARDEN_LDFLAGS += -Wl,-z,now
endif


##############################################################################
# NX stack
##############################################################################

ifdef ENABLE_NOEXECSTACK
HARDEN_LDFLAGS += -Wl,-z,noexecstack
endif


##############################################################################
# Separate code segments
##############################################################################

ifdef ENABLE_SEPARATE_CODE
HARDEN_LDFLAGS += -Wl,-z,separate-code
endif


##############################################################################
# CET (Control-flow Enforcement Technology)
##############################################################################

ifdef ENABLE_CET
ifneq ($(filter x86 x86_64,$(TARGET_ARCH)),)
HARDEN_CET_C_SUPPORTED := $(shell printf 'int main(void){return 0;}\n' | $(CC) -x c -Werror -fcf-protection=full -c -o /dev/null - >/dev/null 2>&1 && echo yes)
HARDEN_CET_CXX_SUPPORTED := $(shell printf 'int main(void){return 0;}\n' | $(CXX) -x c++ -Werror -fcf-protection=full -c -o /dev/null - >/dev/null 2>&1 && echo yes)

ifeq ($(HARDEN_CET_C_SUPPORTED),yes)
HARDEN_CFLAGS  += -fcf-protection=full
endif

ifeq ($(HARDEN_CET_CXX_SUPPORTED),yes)
HARDEN_CXXFLAGS += -fcf-protection=full
endif
endif
endif


##############################################################################
# Remove PLT indirection
##############################################################################

ifdef ENABLE_NO_PLT
HARDEN_NO_PLT_C_SUPPORTED := $(shell printf 'int main(void){return 0;}\n' | $(CC) -x c -Werror -fno-plt -c -o /dev/null - >/dev/null 2>&1 && echo yes)
HARDEN_NO_PLT_CXX_SUPPORTED := $(shell printf 'int main(void){return 0;}\n' | $(CXX) -x c++ -Werror -fno-plt -c -o /dev/null - >/dev/null 2>&1 && echo yes)

ifeq ($(HARDEN_NO_PLT_C_SUPPORTED),yes)
HARDEN_CFLAGS  += -fno-plt
endif

ifeq ($(HARDEN_NO_PLT_CXX_SUPPORTED),yes)
HARDEN_CXXFLAGS += -fno-plt
endif
endif


##############################################################################
# Automatic variable initialization
##############################################################################

ifdef ENABLE_AUTO_VAR_INIT
AUTO_VAR_INIT_C_SUPPORTED := $(shell printf 'int main(void){return 0;}\n' | $(CC) -x c -Werror -ftrivial-auto-var-init=zero -c -o /dev/null - >/dev/null 2>&1 && echo yes)
AUTO_VAR_INIT_CXX_SUPPORTED := $(shell printf 'int main(void){return 0;}\n' | $(CXX) -x c++ -Werror -ftrivial-auto-var-init=zero -c -o /dev/null - >/dev/null 2>&1 && echo yes)

ifeq ($(AUTO_VAR_INIT_C_SUPPORTED),yes)
HARDEN_CFLAGS  += -ftrivial-auto-var-init=zero
endif

ifeq ($(AUTO_VAR_INIT_CXX_SUPPORTED),yes)
HARDEN_CXXFLAGS += -ftrivial-auto-var-init=zero
endif
endif


##############################################################################
# Reproducible build flags
##############################################################################

ifdef ENABLE_REPRODUCIBLE

ifneq ($(strip $(rootdir)),)

HARDEN_FILE_PREFIX_MAP_C_SUPPORTED := $(shell printf 'int main(void){return 0;}\n' | $(CC) -x c -Werror -ffile-prefix-map=$(rootdir)=. -c -o /dev/null - >/dev/null 2>&1 && echo yes)
HARDEN_FILE_PREFIX_MAP_CXX_SUPPORTED := $(shell printf 'int main(void){return 0;}\n' | $(CXX) -x c++ -Werror -ffile-prefix-map=$(rootdir)=. -c -o /dev/null - >/dev/null 2>&1 && echo yes)
HARDEN_DEBUG_PREFIX_MAP_C_SUPPORTED := $(shell printf 'int main(void){return 0;}\n' | $(CC) -x c -Werror -fdebug-prefix-map=$(rootdir)=. -c -o /dev/null - >/dev/null 2>&1 && echo yes)
HARDEN_DEBUG_PREFIX_MAP_CXX_SUPPORTED := $(shell printf 'int main(void){return 0;}\n' | $(CXX) -x c++ -Werror -fdebug-prefix-map=$(rootdir)=. -c -o /dev/null - >/dev/null 2>&1 && echo yes)

ifeq ($(HARDEN_FILE_PREFIX_MAP_C_SUPPORTED),yes)
HARDEN_CFLAGS += -ffile-prefix-map=$(rootdir)=.
endif

ifeq ($(HARDEN_DEBUG_PREFIX_MAP_C_SUPPORTED),yes)
HARDEN_CFLAGS += -fdebug-prefix-map=$(rootdir)=.
endif

ifeq ($(HARDEN_FILE_PREFIX_MAP_CXX_SUPPORTED),yes)
HARDEN_CXXFLAGS += -ffile-prefix-map=$(rootdir)=.
endif

ifeq ($(HARDEN_DEBUG_PREFIX_MAP_CXX_SUPPORTED),yes)
HARDEN_CXXFLAGS += -fdebug-prefix-map=$(rootdir)=.
endif

endif

endif


##############################################################################
# PIC / MT variants
##############################################################################

PIC_CFLAGS :=
MT_CFLAGS  :=
MTPIC_CFLAGS :=

ifdef ENABLE_PIC
PIC_CFLAGS := -fPIC
endif

ifeq ($(THREAD_MODEL),posix)
MT_CFLAGS := -pthread -DENABLE_MT
endif

ifeq ($(THREAD_MODEL),win32)
ifeq ($(TARGET_OS),xbox)
MT_CFLAGS := -DENABLE_MT
else ifeq ($(TOOLCHAIN_CC_IS_CLANG),yes)
MT_CFLAGS := -DENABLE_MT
else
MT_CFLAGS := -mthreads -DENABLE_MT
endif
endif

ifeq ($(THREAD_MODEL),wii)
MT_CFLAGS := -DENABLE_MT
endif

ifdef DISABLE_MT
MT_CFLAGS :=
endif

MTPIC_CFLAGS := $(strip $(MT_CFLAGS) $(PIC_CFLAGS))


##############################################################################
# PIE policy (compiler executable)
##############################################################################

FBC_PIE_CFLAGS :=
FBC_PIE_LDFLAGS :=

ifdef ENABLE_PIE
FBC_PIE_CFLAGS := -Wc -fPIE
FBC_PIE_LDFLAGS := -pie
endif


##############################################################################
# FreeBASIC base flags
##############################################################################

BASE_FBCFLAGS   := -e -m fbc -w pedantic
BASE_FBRTCFLAGS := -e -m nomain
VERSION_FBCFLAGS :=

ifneq ($(strip $(FBVERSION)),)
VERSION_MAJOR := $(word 1,$(subst ., ,$(FBVERSION)))
VERSION_MINOR := $(word 2,$(subst ., ,$(FBVERSION)))
VERSION_PATCH := $(word 3,$(subst ., ,$(FBVERSION)))

ifneq ($(strip $(VERSION_MAJOR)),)
VERSION_FBCFLAGS += -d BUILD_FB_VER_MAJOR=$(VERSION_MAJOR)
endif
ifneq ($(strip $(VERSION_MINOR)),)
VERSION_FBCFLAGS += -d BUILD_FB_VER_MINOR=$(VERSION_MINOR)
endif
ifneq ($(strip $(VERSION_PATCH)),)
VERSION_FBCFLAGS += -d BUILD_FB_VER_PATCH=$(VERSION_PATCH)
endif
endif

ifneq ($(strip $(REV)),)
VERSION_FBCFLAGS += -d BUILD_FB_REV=$(REV)
endif

ifneq ($(strip $(DEFAULT_CPUTYPE_ARM)),)
VERSION_FBCFLAGS += -d BUILD_FB_DEFAULT_CPUTYPE_ARM=$(DEFAULT_CPUTYPE_ARM)
endif

ifdef ENABLE_STANDALONE
BASE_FBCFLAGS += -d ENABLE_STANDALONE
endif

ifeq ($(TARGET_OS),linux)
  ifneq ($(findstring musl,$(TARGET_TRIPLET_LC)),)
    BASE_FBCFLAGS += -d ENABLE_MUSL_DYNAMIC_LINKER
  endif
endif


##############################################################################
# Toolchain quirks
##############################################################################

TOOLCHAIN_CFLAGS :=
TOOLCHAIN_CXXFLAGS :=
TOOLCHAIN_LDFLAGS :=

TOOLCHAIN_FBCFLAGS :=
TOOLCHAIN_FBLFLAGS :=
TOOLCHAIN_FBRTCFLAGS :=
TOOLCHAIN_FBRTLFLAGS :=
TOOLCHAIN_FBC_ENV :=


ifeq ($(TARGET_OS),win32)

ifneq ($(TOOLCHAIN_CC_IS_CLANG),yes)
TOOLCHAIN_CFLAGS   += -mconsole
TOOLCHAIN_CXXFLAGS += -mconsole
endif
TOOLCHAIN_LDFLAGS  += -mconsole

endif

ifeq ($(TARGET_OS),wii)

DEVKITPRO ?= /opt/devkitpro
DEVKITPPC ?= $(DEVKITPRO)/devkitPPC
WII_LIBOGC_INC ?= $(DEVKITPRO)/libogc/include
WII_LIBOGC_LIB ?= $(DEVKITPRO)/libogc/lib/wii
ELF2DOL ?= $(DEVKITPRO)/tools/bin/elf2dol

WII_MACHDEP := -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float

CPPFLAGS += -I$(WII_LIBOGC_INC)
TOOLCHAIN_CFLAGS   += $(WII_MACHDEP)
TOOLCHAIN_CXXFLAGS += $(WII_MACHDEP)
TOOLCHAIN_LDFLAGS  += -mrvl -mcpu=750 -meabi -mhard-float -L$(WII_LIBOGC_LIB)

WII_FBC_STAGE_FLAGS := \
  -Wc -DGEKKO \
  -Wc -I$(WII_LIBOGC_INC) \
  -Wc -mrvl \
  -Wc -mcpu=750 \
  -Wc -meabi \
  -Wc -mhard-float \
  -Wa -mrvl \
  -Wa -mcpu=750 \
  -Wa -meabi \
  -Wa -mhard-float

TOOLCHAIN_FBCFLAGS   += $(WII_FBC_STAGE_FLAGS)
TOOLCHAIN_FBRTCFLAGS += $(WII_FBC_STAGE_FLAGS)
TOOLCHAIN_FBRTLFLAGS += $(WII_FBC_STAGE_FLAGS)
TOOLCHAIN_FBC_ENV += DEVKITPRO='$(DEVKITPRO)' DEVKITPPC='$(DEVKITPPC)' ELF2DOL='$(ELF2DOL)'

endif

ifeq ($(TARGET_OS),darwin)

#
# Darwin SDK handling
#
# Modern macOS installations keep system headers inside the Command Line Tools
# SDK instead of /usr/include.
#
# Apple clang understands -isysroot and uses the SDK headers normally. Real GCC
# from Homebrew also needs the SDK, but feeding it -isysroot makes it prefer
# GCC's include-fixed copies of Apple headers. Those fixed headers can be
# incomplete against newer SDKs, so GCC gets explicit SDK include/framework
# search paths plus a linker syslibroot instead.
#
DARWIN_SDKROOT := $(strip $(shell xcrun --sdk macosx --show-sdk-path 2>/dev/null || xcrun --show-sdk-path 2>/dev/null))
DARWIN_CC_IS_CLANG := $(strip $(shell $(CC) -dM -E -x c /dev/null 2>/dev/null | grep -q __clang__ && echo yes || true))
DARWIN_NCURSES_CFLAGS := $(strip $(shell pkg-config --cflags ncurses 2>/dev/null))
DARWIN_LIBFFI_CFLAGS := $(strip $(shell pkg-config --cflags libffi 2>/dev/null))
DARWIN_PKG_LDFLAGS := $(strip $(shell pkg-config --libs-only-L ncurses libffi 2>/dev/null))

#
# Homebrew dylibs are often built with a newer minimum macOS version than
# FreeBASIC's baseline target.  When the compiler links against those dylibs,
# the resulting binary cannot run below their own minimum anyway.  Raise the
# default deployment target to match the linked dependency floor instead of
# letting ld64 print a misleading "built for newer version" warning.
#
DARWIN_PKG_LIBDIRS := $(patsubst -L%,%,$(filter -L%,$(DARWIN_PKG_LDFLAGS)))
DARWIN_PKG_MIN_DEPLOYMENT_TARGET := $(strip $(shell \
  printf '%s\n' $(DARWIN_PKG_LIBDIRS) \
  | while IFS= read -r d; do \
      [ -n "$$d" ] || continue; \
      for f in "$$d"/libncurses*.dylib "$$d"/libffi*.dylib; do \
        [ -f "$$f" ] || continue; \
        otool -l "$$f" 2>/dev/null \
        | awk '/LC_BUILD_VERSION/{inbuild=1; next} inbuild && /minos/{print $$2; exit} /^Load command/{inbuild=0}'; \
      done; \
    done \
  | awk -F. 'NF { major=$$1+0; minor=$$2+0; patch=$$3+0; if ((major > best_major) || (major == best_major && minor > best_minor) || (major == best_major && minor == best_minor && patch > best_patch)) { best=$$0; best_major=major; best_minor=minor; best_patch=patch; } } END { if (best != "") print best; }' \
))
DARWIN_DEPLOYMENT_TARGET ?= $(MACOSX_DEPLOYMENT_TARGET)
ifeq ($(strip $(DARWIN_DEPLOYMENT_TARGET)),)
  ifeq ($(TARGET_ARCH),aarch64)
    DARWIN_BASE_DEPLOYMENT_TARGET := 11.0
  else
    DARWIN_BASE_DEPLOYMENT_TARGET := 10.5
  endif
  DARWIN_DEPLOYMENT_TARGET := $(strip $(shell \
    printf '%s\n%s\n' '$(DARWIN_BASE_DEPLOYMENT_TARGET)' '$(DARWIN_PKG_MIN_DEPLOYMENT_TARGET)' \
    | awk -F. 'NF { major=$$1+0; minor=$$2+0; patch=$$3+0; if ((major > best_major) || (major == best_major && minor > best_minor) || (major == best_major && minor == best_minor && patch > best_patch)) { best=$$0; best_major=major; best_minor=minor; best_patch=patch; } } END { print best; }' \
  ))
endif

TOOLCHAIN_CFLAGS   += $(DARWIN_NCURSES_CFLAGS) $(DARWIN_LIBFFI_CFLAGS)
TOOLCHAIN_CXXFLAGS += $(DARWIN_NCURSES_CFLAGS) $(DARWIN_LIBFFI_CFLAGS)
TOOLCHAIN_LDFLAGS  += $(DARWIN_PKG_LDFLAGS)

#
# macOS package staging rewrites the compiler's dependent library paths after
# installation so the bundled toolchain can live under FreeBASIC's prefix.
# ld64 must reserve enough load-command space at link time for those later
# install_name_tool edits.
#
TOOLCHAIN_LDFLAGS  += -Wl,-headerpad_max_install_names

#
# Some Darwin linker paths can emit optional __LINKEDIT metadata load commands
# after the symbol-table load command even though their data appears earlier in
# the file. Older install_name_tool builds reject that command/data ordering
# when rewriting dependencies.
#
TOOLCHAIN_LDFLAGS  += -Wl,-no_function_starts
TOOLCHAIN_LDFLAGS  += -Wl,-no_data_in_code_info

ifneq ($(strip $(DARWIN_DEPLOYMENT_TARGET)),)
  TOOLCHAIN_CFLAGS   += -mmacosx-version-min=$(DARWIN_DEPLOYMENT_TARGET)
  TOOLCHAIN_CXXFLAGS += -mmacosx-version-min=$(DARWIN_DEPLOYMENT_TARGET)
  TOOLCHAIN_LDFLAGS  += -mmacosx-version-min=$(DARWIN_DEPLOYMENT_TARGET)
  TOOLCHAIN_FBCFLAGS += -d FB_DARWIN_DEFAULT_DEPLOYMENT_TARGET=\"$(DARWIN_DEPLOYMENT_TARGET)\"
  TOOLCHAIN_FBC_ENV  += MACOSX_DEPLOYMENT_TARGET='$(DARWIN_DEPLOYMENT_TARGET)'

  #
  # fbc-driven build steps need the same deployment target at the generated-C
  # and assembler stages. Without this, modern clang emits object metadata for
  # the SDK version and ld64 warns when the final executable targets older
  # macOS. The builtin control is scoped to generated compiler/runtime C:
  # fbc compiles with -nostdinc and declares C runtime entry points itself.
  #
  DARWIN_FBC_STAGE_FLAGS := \
    -Wc -mmacosx-version-min=$(DARWIN_DEPLOYMENT_TARGET) \
    -Wa -mmacosx-version-min=$(DARWIN_DEPLOYMENT_TARGET) \
    -Wc -fno-builtin
  TOOLCHAIN_FBCFLAGS   += $(DARWIN_FBC_STAGE_FLAGS)
  TOOLCHAIN_FBRTCFLAGS += $(DARWIN_FBC_STAGE_FLAGS)
  TOOLCHAIN_FBRTLFLAGS += $(DARWIN_FBC_STAGE_FLAGS)
endif

ifneq ($(strip $(DARWIN_SDKROOT)),)
  TOOLCHAIN_FBC_ENV += SDKROOT='$(DARWIN_SDKROOT)'
  ifeq ($(DARWIN_CC_IS_CLANG),yes)
    TOOLCHAIN_CFLAGS   += -isysroot $(DARWIN_SDKROOT)
    TOOLCHAIN_CXXFLAGS += -isysroot $(DARWIN_SDKROOT)
    TOOLCHAIN_LDFLAGS  += -isysroot $(DARWIN_SDKROOT)
  else
    TOOLCHAIN_CFLAGS   += -isystem $(DARWIN_SDKROOT)/usr/include
    TOOLCHAIN_CFLAGS   += -iframework $(DARWIN_SDKROOT)/System/Library/Frameworks
    TOOLCHAIN_CXXFLAGS += -isystem $(DARWIN_SDKROOT)/usr/include
    TOOLCHAIN_CXXFLAGS += -iframework $(DARWIN_SDKROOT)/System/Library/Frameworks
    TOOLCHAIN_LDFLAGS  += -Wl,-syslibroot,$(DARWIN_SDKROOT)
  endif
endif

endif


##############################################################################
# Final aggregation
##############################################################################

ALLCFLAGS += $(BASE_CFLAGS)
ALLCFLAGS += $(HARDEN_CFLAGS)
ALLCFLAGS += $(CFLAGS)
ALLCFLAGS += $(TOOLCHAIN_CFLAGS)

ALLCXXFLAGS += $(BASE_CXXFLAGS)
ALLCXXFLAGS += $(HARDEN_CXXFLAGS)
ALLCXXFLAGS += $(CXXFLAGS)
ALLCXXFLAGS += $(TOOLCHAIN_CXXFLAGS)

ALLLDFLAGS += $(HARDEN_LDFLAGS)
ALLLDFLAGS += $(LDFLAGS)
ALLLDFLAGS += $(TOOLCHAIN_LDFLAGS)


ALLFBCFLAGS += $(BASE_FBCFLAGS)
ALLFBCFLAGS += $(VERSION_FBCFLAGS)
ALLFBCFLAGS += $(FBCFLAGS) $(FBFLAGS)
ALLFBCFLAGS += $(FBC_PIE_CFLAGS)
ALLFBCFLAGS += $(TOOLCHAIN_FBCFLAGS)

ALLFBLFLAGS += $(BASE_FBCFLAGS)
ALLFBLFLAGS += $(FBLFLAGS) $(FBFLAGS)
ALLFBLFLAGS += $(FBC_PIE_LDFLAGS)
ALLFBLFLAGS += $(TOOLCHAIN_FBLFLAGS)

ALLFBRTCFLAGS += $(BASE_FBRTCFLAGS)
ALLFBRTCFLAGS += $(FBRTCFLAGS) $(FBFLAGS)
ALLFBRTCFLAGS += $(TOOLCHAIN_FBRTCFLAGS)

ALLFBRTLFLAGS += $(BASE_FBRTCFLAGS)
ALLFBRTLFLAGS += $(FBRTLFLAGS) $(FBFLAGS)
ALLFBRTLFLAGS += $(TOOLCHAIN_FBRTLFLAGS)

##############################################################################
# Forward C hardening flags through fbc
##############################################################################

FBC_FORWARD_CFLAGS := $(filter-out -MMD -MP,$(ALLCFLAGS))

# fbc does not support clang-style -Wp, forwarding from environment hardening
# flags for preprocessor options.  Drop those entries before converting to
# -Wc arguments so builds stay compatible with distros that inject -Wp,-D...
FBC_FORWARD_CFLAGS := $(filter-out -Wp%,$(FBC_FORWARD_CFLAGS))
FBC_WCFLAGS := $(foreach f,$(FBC_FORWARD_CFLAGS),-Wc $(f))

ALLFBCFLAGS += $(FBC_WCFLAGS)

##############################################################################
# Generated C warning suppressions
##############################################################################

# Keep these after forwarded -Wall/-Wextra so generated compiler/runtime C stays
# quiet without weakening diagnostics for handwritten C sources.
GENERATED_C_WARN_SUPPRESS := \
  -Wc -Wno-missing-field-initializers

ifeq ($(TOOLCHAIN_CC_IS_CLANG),yes)
GENERATED_C_WARN_SUPPRESS += \
  -Wc -Wno-sometimes-uninitialized \
  -Wc -Wno-unused-but-set-variable \
  -Wc -Wno-unused-function \
  -Wc -Wno-unused-label \
  -Wc -Wno-unused-variable
else ifneq ($(filter linux cygwin darwin win32 win64,$(TARGET_OS)),)
GENERATED_C_WARN_SUPPRESS += \
  -Wc -Wno-builtin-declaration-mismatch \
  -Wc -Wno-maybe-uninitialized \
  -Wc -Wno-type-limits
endif

ALLFBCFLAGS   += $(GENERATED_C_WARN_SUPPRESS)
ALLFBRTCFLAGS += $(GENERATED_C_WARN_SUPPRESS)
ALLFBRTLFLAGS += $(GENERATED_C_WARN_SUPPRESS)


#############################
# end of toolchain-flags.mk
#############################
