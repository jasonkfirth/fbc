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
HARDEN_CFLAGS  += -fcf-protection=full
HARDEN_CXXFLAGS += -fcf-protection=full
endif
endif


##############################################################################
# Remove PLT indirection
##############################################################################

ifdef ENABLE_NO_PLT
HARDEN_CFLAGS  += -fno-plt
HARDEN_CXXFLAGS += -fno-plt
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

HARDEN_CFLAGS += -ffile-prefix-map=$(rootdir)=.
HARDEN_CFLAGS += -fdebug-prefix-map=$(rootdir)=.

HARDEN_CXXFLAGS += -ffile-prefix-map=$(rootdir)=.
HARDEN_CXXFLAGS += -fdebug-prefix-map=$(rootdir)=.

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
MT_CFLAGS := -pthread
endif

ifeq ($(THREAD_MODEL),win32)
MT_CFLAGS := -mthreads
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

ifdef ENABLE_STANDALONE
BASE_FBCFLAGS += -d ENABLE_STANDALONE
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


ifeq ($(TARGET_OS),win32)

TOOLCHAIN_CFLAGS   += -mconsole
TOOLCHAIN_CXXFLAGS += -mconsole
TOOLCHAIN_LDFLAGS  += -mconsole

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
FBC_WCFLAGS := $(foreach f,$(FBC_FORWARD_CFLAGS),-Wc $(f))

ALLFBCFLAGS += $(FBC_WCFLAGS)


#############################
# end of toolchain-flags.mk
#############################
