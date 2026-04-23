##############################################################################
# compiler-config.mk
#
# Toolchain configuration and compiler selection
# (regenerated with layout-safe FBC handling)
##############################################################################

##############################################################################
# Host toolchain detection
##############################################################################

LOCAL_GCC_CANDIDATE := $(firstword $(wildcard bin/gcc$(EXEEXT) bin/gcc.exe bin/gcc))
LOCAL_GXX_CANDIDATE := $(firstword $(wildcard bin/g++$(EXEEXT) bin/g++.exe bin/g++))
LOCAL_AR_CANDIDATE := $(firstword $(wildcard bin/ar$(EXEEXT) bin/ar.exe bin/ar))
LOCAL_RANLIB_CANDIDATE := $(firstword $(wildcard bin/ranlib$(EXEEXT) bin/ranlib.exe bin/ranlib))
LOCAL_AS_CANDIDATE := $(firstword $(wildcard bin/as$(EXEEXT) bin/as.exe bin/as))
LOCAL_LD_CANDIDATE := $(firstword $(wildcard bin/ld$(EXEEXT) bin/ld.exe bin/ld))

LOCAL_GCC := $(strip $(shell if [ -n "$(LOCAL_GCC_CANDIDATE)" ] && "$(LOCAL_GCC_CANDIDATE)" -dumpmachine >/dev/null 2>&1; then echo "$(LOCAL_GCC_CANDIDATE)"; fi))
LOCAL_GXX := $(strip $(shell if [ -n "$(LOCAL_GXX_CANDIDATE)" ] && "$(LOCAL_GXX_CANDIDATE)" --version >/dev/null 2>&1; then echo "$(LOCAL_GXX_CANDIDATE)"; fi))
LOCAL_AR := $(strip $(shell if [ -n "$(LOCAL_AR_CANDIDATE)" ] && "$(LOCAL_AR_CANDIDATE)" --version >/dev/null 2>&1; then echo "$(LOCAL_AR_CANDIDATE)"; fi))
LOCAL_RANLIB := $(strip $(shell if [ -n "$(LOCAL_RANLIB_CANDIDATE)" ] && "$(LOCAL_RANLIB_CANDIDATE)" --version >/dev/null 2>&1; then echo "$(LOCAL_RANLIB_CANDIDATE)"; fi))
LOCAL_AS := $(strip $(shell if [ -n "$(LOCAL_AS_CANDIDATE)" ] && "$(LOCAL_AS_CANDIDATE)" --version >/dev/null 2>&1; then echo "$(LOCAL_AS_CANDIDATE)"; fi))
LOCAL_LD := $(strip $(shell if [ -n "$(LOCAL_LD_CANDIDATE)" ] && "$(LOCAL_LD_CANDIDATE)" --version >/dev/null 2>&1; then echo "$(LOCAL_LD_CANDIDATE)"; fi))

MSYS_GCC := $(firstword $(wildcard /mingw64/bin/gcc.exe /ucrt64/bin/gcc.exe /clang64/bin/gcc.exe))
MSYS_GXX := $(firstword $(wildcard /mingw64/bin/g++.exe /ucrt64/bin/g++.exe /clang64/bin/g++.exe))
MSYS_AR := $(firstword $(wildcard /mingw64/bin/ar.exe /ucrt64/bin/ar.exe /clang64/bin/ar.exe))
MSYS_RANLIB := $(firstword $(wildcard /mingw64/bin/ranlib.exe /ucrt64/bin/ranlib.exe /clang64/bin/ranlib.exe))
MSYS_AS := $(firstword $(wildcard /mingw64/bin/as.exe /ucrt64/bin/as.exe /clang64/bin/as.exe))
MSYS_LD := $(firstword $(wildcard /mingw64/bin/ld.exe /ucrt64/bin/ld.exe /clang64/bin/ld.exe))

HOMEBREW_GCC := $(strip $(shell find /opt/homebrew/bin /usr/local/bin -maxdepth 1 \( -type f -o -type l \) -name 'gcc-*' 2>/dev/null | grep -E '/gcc-[0-9]+$$' | sort -V | tail -n1))
HOMEBREW_GXX := $(strip $(shell find /opt/homebrew/bin /usr/local/bin -maxdepth 1 \( -type f -o -type l \) -name 'g++-*' 2>/dev/null | grep -E '/g[+][+]-[0-9]+$$' | sort -V | tail -n1))

HOST_CC_FOR_PROBE := $(strip $(or $(LOCAL_GCC),$(MSYS_GCC),$(HOMEBREW_GCC),$(CC),gcc))
HOST_TRIPLET := $(shell $(HOST_CC_FOR_PROBE) -dumpmachine 2>/dev/null || echo unknown)
HOST_TRIPLET_LC := $(strip $(shell printf '%s' "$(HOST_TRIPLET)" | tr 'A-Z' 'a-z'))
HOST_TRIPLET_TOKENS := $(subst -, ,$(HOST_TRIPLET_LC))
HOST_ARCH_RAW := $(word 1,$(HOST_TRIPLET_TOKENS))
HOST_ARCH := $(HOST_ARCH_RAW)
HOST_OS :=

ifneq ($(filter 386 486 586 686 i386 i486 i586 i686 x86,$(HOST_ARCH_RAW)),)
  HOST_ARCH := x86
endif

ifneq ($(filter x86_64 amd64 x86-64,$(HOST_ARCH_RAW)),)
  HOST_ARCH := x86_64
endif

ifneq ($(filter arm%,$(HOST_ARCH_RAW)),)
  HOST_ARCH := arm
endif
ifneq ($(filter armhf armel,$(HOST_ARCH_RAW)),)
  HOST_ARCH := arm
endif

ifneq ($(filter aarch64 arm64,$(HOST_ARCH_RAW)),)
  HOST_ARCH := aarch64
endif

ifneq ($(filter ppc powerpc,$(HOST_ARCH_RAW)),)
  HOST_ARCH := powerpc
endif

ifneq ($(filter ppc64 powerpc64,$(HOST_ARCH_RAW)),)
  HOST_ARCH := powerpc64
endif

ifneq ($(filter ppc64le powerpc64le ppc64el,$(HOST_ARCH_RAW)),)
  HOST_ARCH := powerpc64le
endif

ifneq ($(filter riscv64,$(HOST_ARCH_RAW)),)
  HOST_ARCH := riscv64
endif

ifneq ($(filter s390x,$(HOST_ARCH_RAW)),)
  HOST_ARCH := s390x
endif

ifneq ($(filter loongarch64 loong64,$(HOST_ARCH_RAW)),)
  HOST_ARCH := loongarch64
endif

ifneq ($(findstring linux,$(HOST_TRIPLET_LC)),)
  HOST_OS := linux
endif
ifneq ($(findstring android,$(HOST_TRIPLET_LC)),)
  HOST_OS := android
endif
ifneq ($(or $(findstring darwin,$(HOST_TRIPLET_LC)),$(findstring apple,$(HOST_TRIPLET_LC))),)
  HOST_OS := darwin
endif
ifneq ($(findstring freebsd,$(HOST_TRIPLET_LC)),)
  HOST_OS := freebsd
endif
ifneq ($(findstring netbsd,$(HOST_TRIPLET_LC)),)
  HOST_OS := netbsd
endif
ifneq ($(findstring openbsd,$(HOST_TRIPLET_LC)),)
  HOST_OS := openbsd
endif
ifneq ($(findstring dragonfly,$(HOST_TRIPLET_LC)),)
  HOST_OS := dragonfly
endif
ifneq ($(findstring haiku,$(HOST_TRIPLET_LC)),)
  HOST_OS := haiku
endif
ifneq ($(or $(findstring solaris,$(HOST_TRIPLET_LC)),$(findstring sunos,$(HOST_TRIPLET_LC)),$(findstring illumos,$(HOST_TRIPLET_LC))),)
  HOST_OS := solaris
endif
ifneq ($(findstring cygwin,$(HOST_TRIPLET_LC)),)
  HOST_OS := cygwin
endif
ifneq ($(or $(findstring mingw,$(HOST_TRIPLET_LC)),$(findstring w64,$(HOST_TRIPLET_LC)),$(findstring msys,$(HOST_TRIPLET_LC))),)
  HOST_OS := win32
endif
ifneq ($(or $(findstring djgpp,$(HOST_TRIPLET_LC)),$(findstring msdosdjgpp,$(HOST_TRIPLET_LC))),)
  HOST_OS := dos
endif

# Determine whether we are cross-compiling
CROSS_BUILD :=

ifneq ($(strip $(TARGET_TRIPLET)),)
  ifneq ($(strip $(HOST_OS)),)
    ifneq ($(TARGET_OS),$(HOST_OS))
      CROSS_BUILD := yes
    else ifneq ($(TARGET_ARCH),$(HOST_ARCH))
      CROSS_BUILD := yes
    endif
  else ifneq ($(TARGET_TRIPLET),$(HOST_TRIPLET))
    CROSS_BUILD := yes
  endif
endif


##############################################################################
# Toolchain prefix
##############################################################################

BUILD_PREFIX ?=

ifeq ($(CROSS_BUILD),yes)

  ifeq ($(strip $(BUILD_PREFIX)),)
    BUILD_PREFIX := $(TARGET_TRIPLET)-
  endif

endif

##############################################################################
# Verify cross compiler exists
##############################################################################

ifneq ($(strip $(BUILD_PREFIX)),)

PREFIXED_GCC := $(BUILD_PREFIX)gcc

ifeq ($(shell command -v $(PREFIXED_GCC) 2>/dev/null),)

$(warning Requested cross toolchain '$(PREFIXED_GCC)' not found)

# fallback to host compiler
BUILD_PREFIX :=

endif

endif

##############################################################################
# Base tools
##############################################################################

# C compiler
ifneq ($(filter default file,$(origin CC)),)
  ifeq ($(CROSS_BUILD),yes)
    CC := $(BUILD_PREFIX)gcc
  else ifneq ($(strip $(LOCAL_GCC)),)
    CC := $(LOCAL_GCC)
  else ifneq ($(strip $(MSYS_GCC)),)
    CC := $(MSYS_GCC)
  else ifneq ($(strip $(HOMEBREW_GCC)),)
    CC := $(HOMEBREW_GCC)
  else
    CC := gcc
  endif
endif


# C++ compiler
ifneq ($(filter default file,$(origin CXX)),)
  ifeq ($(CROSS_BUILD),yes)
    CXX := $(BUILD_PREFIX)g++
  else ifneq ($(strip $(LOCAL_GXX)),)
    CXX := $(LOCAL_GXX)
  else ifneq ($(strip $(MSYS_GXX)),)
    CXX := $(MSYS_GXX)
  else ifneq ($(strip $(HOMEBREW_GXX)),)
    CXX := $(HOMEBREW_GXX)
  else
    CXX := g++
  endif
endif


# Archiver
ifeq ($(origin AR),default)
  ifeq ($(CROSS_BUILD),yes)
    AR := $(BUILD_PREFIX)ar
  else ifneq ($(strip $(LOCAL_AR)),)
    AR := $(LOCAL_AR)
  else ifneq ($(strip $(MSYS_AR)),)
    AR := $(MSYS_AR)
  else
    AR := ar
  endif
endif


# Ranlib
ifeq ($(origin RANLIB),default)
  ifeq ($(CROSS_BUILD),yes)
    RANLIB := $(BUILD_PREFIX)ranlib
  else ifneq ($(strip $(LOCAL_RANLIB)),)
    RANLIB := $(LOCAL_RANLIB)
  else ifneq ($(strip $(MSYS_RANLIB)),)
    RANLIB := $(MSYS_RANLIB)
  else
    RANLIB := ranlib
  endif
endif


# Assembler
ifeq ($(origin AS),default)
  ifeq ($(CROSS_BUILD),yes)
    AS := $(BUILD_PREFIX)as
  else ifneq ($(strip $(LOCAL_AS)),)
    AS := $(LOCAL_AS)
  else ifneq ($(strip $(MSYS_AS)),)
    AS := $(MSYS_AS)
  else
    AS := as
  endif
endif


##############################################################################
# Linker selection
##############################################################################

ifeq ($(origin LD),default)

  ifneq ($(filter linux android darwin freebsd netbsd openbsd dragonfly solaris haiku,$(TARGET_OS)),)
    LD := $(CC)
  else
    ifeq ($(CROSS_BUILD),yes)
      LD := $(BUILD_PREFIX)ld
    else ifneq ($(strip $(LOCAL_LD)),)
      LD := $(LOCAL_LD)
    else ifneq ($(strip $(MSYS_LD)),)
      LD := $(MSYS_LD)
    else
      LD := ld
    endif
  endif

endif


##############################################################################
# FreeBASIC compiler used during the build
#
# IMPORTANT:
# Do NOT overwrite FBC_EXE defined by build-layout.mk
##############################################################################

FBC_EXE    ?= bin/fbc$(EXEEXT)
FBCNEW_EXE ?= bin/fbc-new$(EXEEXT)

# Prefer the locally built compiler when it exists, otherwise fall back to a
# system fbc for bootstrap/emission flows.
LOCAL_FBC := $(firstword $(wildcard $(FBC_EXE)))
SYSTEM_FBC := $(strip $(shell command -v fbc 2>/dev/null))
AVAILABLE_FBC := $(strip $(or $(LOCAL_FBC),$(SYSTEM_FBC)))

# Keep FBC pointing at the in-tree compiler path for install/test flows.
FBC := $(FBC_EXE)

# Compiler that is executed during the build. In cross-builds this may need to
# stay host-runnable even while FBC_EXE is being produced for the target.
ifndef BUILD_FBC
  ifeq ($(CROSS_BUILD),yes)
    ifneq ($(strip $(BOOT_FBC)),)
      BUILD_FBC := $(BOOT_FBC)
    else ifneq ($(strip $(AVAILABLE_FBC)),)
      BUILD_FBC := $(AVAILABLE_FBC)
    else
      BUILD_FBC := $(FBC)
    endif
  else ifneq ($(strip $(AVAILABLE_FBC)),)
    BUILD_FBC := $(AVAILABLE_FBC)
  else
    BUILD_FBC := $(FBC)
  endif
endif

ifeq ($(CROSS_BUILD),yes)
  BUILD_FBC_TARGET ?= $(FBC_TARGET)
  BUILD_FBC_BUILDPREFIX ?= $(BUILD_PREFIX)
endif


##############################################################################
# Safety guard
##############################################################################

ifeq ($(notdir $(FBC_EXE)),fbc)
ifneq ($(dir $(FBC_EXE)),./)
ifneq ($(dir $(FBC_EXE)),)
endif
endif
endif


##############################################################################
# Compiler flags
##############################################################################

CFLAGS ?= -O2 -Wall
CFLAGS += -Wfatal-errors

CXXFLAGS ?= -O2 -Wall


##############################################################################
# FreeBASIC compiler flags
##############################################################################

FBFLAGS ?= -maxerr 1


##############################################################################
# Link flags
##############################################################################

LDFLAGS ?=


##############################################################################
# Thread flags
##############################################################################

THREAD_FLAGS :=

ifeq ($(THREAD_MODEL),posix)
  THREAD_FLAGS := -pthread
endif

ifeq ($(THREAD_MODEL),win32)
  THREAD_FLAGS := -mthreads
endif

ifdef DISABLE_MT
  THREAD_FLAGS :=
endif


##############################################################################
# Debugging output helpers
##############################################################################

TOOLCHAIN_VARS := \
  HOST_TRIPLET \
  TARGET_TRIPLET \
  CROSS_BUILD \
  BUILD_PREFIX \
  CC \
  CXX \
  AR \
  RANLIB \
  AS \
  LD \
  LOCAL_FBC \
  SYSTEM_FBC \
  AVAILABLE_FBC \
  FBC_EXE \
  BUILD_FBC

##############################################################################
# End compiler-config.mk
##############################################################################
