##############################################################################
# platform.mk
#
# Canonical platform identity layer
#
# Responsibilities:
#   - resolve host / target triplet
#   - detect TARGET_OS
#   - normalize TARGET_ARCH
#   - compute FBC_TARGET (FreeBASIC runtime/target key)
#   - define runtime directory identity (canonical) vs packaging identity
#
# Non-responsibilities:
#   - toolchain prefix handling  → compiler-config.mk
#   - filesystem layout	  → layout.mk / build-layout.mk
##############################################################################


##############################################################################
# 0) Canonical triplet resolution
##############################################################################

# Consume TARGET once (triplet input only), then clear it (main Makefile may do
# this already; keeping it here makes platform.mk self-contained & robust).
TARGET_TRIPLET ?= $(TARGET)
ifdef TARGET
  ifeq ($(strip $(TARGET_TRIPLET)),)
    TARGET_TRIPLET := $(TARGET)
  endif
  override TARGET :=
endif

# Determine host triplet via compiler driver; do NOT assume gcc exists

# Try known GNU-style compilers in order
_cc_for_dump := $(firstword \
  $(foreach c, \
    $(CC) gcc egcc clang, \
    $(if $(shell command -v $(c) 2>/dev/null),$(c)) \
  ) \
)

# Final fallback (last resort only)
ifeq ($(_cc_for_dump),)
  _cc_for_dump := cc
endif

HOST_TRIPLET := $(strip $(shell $(_cc_for_dump) -dumpmachine 2>/dev/null || echo unknown-unknown-unknown))

# Fallback to host if no explicit target
TARGET_TRIPLET := $(strip $(if $(TARGET_TRIPLET),$(TARGET_TRIPLET),$(HOST_TRIPLET)))

# Normalized lowercase form
TARGET_TRIPLET_LC := $(strip $(shell printf '%s' "$(TARGET_TRIPLET)" | tr 'A-Z' 'a-z'))

TRIPLET_TOKENS := $(subst -, ,$(TARGET_TRIPLET_LC))
TRIPLET_ARCH   := $(word 1,$(TRIPLET_TOKENS))


##############################################################################
# 1) TARGET_OS detection
##############################################################################

ifndef TARGET_OS

TARGET_OS :=

define _set_os_if_token
  ifeq ($(strip $(TARGET_OS)),)
    ifneq ($(filter $(1)%,$(TRIPLET_TOKENS)),)
      TARGET_OS := $(2)
    endif
  endif
endef

$(eval $(call _set_os_if_token,linux,linux))
$(eval $(call _set_os_if_token,android,android))
$(eval $(call _set_os_if_token,darwin,darwin))
$(eval $(call _set_os_if_token,apple,darwin))
$(eval $(call _set_os_if_token,freebsd,freebsd))
$(eval $(call _set_os_if_token,netbsd,netbsd))
$(eval $(call _set_os_if_token,openbsd,openbsd))
$(eval $(call _set_os_if_token,dragonfly,dragonfly))
$(eval $(call _set_os_if_token,haiku,haiku))
$(eval $(call _set_os_if_token,solaris,solaris))
$(eval $(call _set_os_if_token,sunos,solaris))
$(eval $(call _set_os_if_token,illumos,solaris))
$(eval $(call _set_os_if_token,cygwin,cygwin))
$(eval $(call _set_os_if_token,msdosdjgpp,dos))
$(eval $(call _set_os_if_token,djgpp,dos))

# MinGW-like triplets show up in a bunch of forms
$(eval $(call _set_os_if_token,mingw,win32))
$(eval $(call _set_os_if_token,mingw32,win32))
$(eval $(call _set_os_if_token,mingw64,win32))
$(eval $(call _set_os_if_token,w64,win32))
$(eval $(call _set_os_if_token,msys,win32))

ifeq ($(strip $(TARGET_OS)),)
  $(error Unable to determine TARGET_OS from TARGET_TRIPLET='$(TARGET_TRIPLET)')
endif

endif


##############################################################################
# 2) Architecture normalization
##############################################################################

# Keep the raw arch token for sub-arch parsing (armv6l/armv7l/etc)
TARGET_ARCH_RAW := $(TRIPLET_ARCH)
TARGET_ARCH     := $(TARGET_ARCH_RAW)

# x86 32-bit (various spellings)
ifneq ($(filter 386 486 586 686 i386 i486 i586 i686 x86,$(TARGET_ARCH_RAW)),)
  TARGET_ARCH := x86
endif

# x86_64
ifneq ($(filter x86_64 amd64 x86-64,$(TARGET_ARCH_RAW)),)
  TARGET_ARCH := x86_64
endif

# ARM 32-bit (arm, armv6l, armv7l, armhf/armel sometimes appear in non-GNU ids)
ifneq ($(filter arm%,$(TARGET_ARCH_RAW)),)
  TARGET_ARCH := arm
endif
ifneq ($(filter armhf armel,$(TARGET_ARCH_RAW)),)
  TARGET_ARCH := arm
endif

# AArch64
ifneq ($(filter aarch64 arm64,$(TARGET_ARCH_RAW)),)
  TARGET_ARCH := aarch64
endif

# PowerPC 32
ifneq ($(filter ppc powerpc,$(TARGET_ARCH_RAW)),)
  TARGET_ARCH := powerpc
endif

# PowerPC 64 (be)
ifneq ($(filter ppc64 powerpc64,$(TARGET_ARCH_RAW)),)
  TARGET_ARCH := powerpc64
endif

# PowerPC 64 little-endian:
# Debian packaging calls this "ppc64el", GNU triplets usually use "powerpc64le"
ifneq ($(filter ppc64le powerpc64le ppc64el,$(TARGET_ARCH_RAW)),)
  TARGET_ARCH := powerpc64le
endif

# RISC-V 64
ifneq ($(filter riscv64,$(TARGET_ARCH_RAW)),)
  TARGET_ARCH := riscv64
endif

# IBM z
ifneq ($(filter s390x,$(TARGET_ARCH_RAW)),)
  TARGET_ARCH := s390x
endif

# LoongArch 64
ifneq ($(filter loongarch64 loong64,$(TARGET_ARCH_RAW)),)
  TARGET_ARCH := loongarch64
endif


##############################################################################
# 3) ISA family (directory key for src/*/<family>)
##############################################################################

ISA_FAMILY := $(TARGET_ARCH)

# aarch64 shares arm sources
ifeq ($(TARGET_ARCH),aarch64)
  ISA_FAMILY := arm
endif

# powerpc64 variants share powerpc sources
ifneq ($(filter powerpc powerpc64 powerpc64le,$(TARGET_ARCH)),)
  ISA_FAMILY := powerpc
endif

# riscv64 uses riscv directory
ifeq ($(TARGET_ARCH),riscv64)
  ISA_FAMILY := riscv
endif

# s390x uses s390 directory
ifeq ($(TARGET_ARCH),s390x)
  ISA_FAMILY := s390
endif

# loongarch64 uses loongarch directory
ifeq ($(TARGET_ARCH),loongarch64)
  ISA_FAMILY := loongarch
endif


##############################################################################
# 4) ARM float ABI detection (needed for packaging name decisions)
##############################################################################

ARM_FLOAT_ABI :=
ifeq ($(TARGET_ARCH),arm)
  ifneq ($(findstring eabihf,$(TARGET_TRIPLET_LC)),)
    ARM_FLOAT_ABI := hf
  else
    ARM_FLOAT_ABI := sf
  endif
endif


##############################################################################
# 5) FreeBASIC lowering target (FBC_TARGET)
#
# This is the canonical runtime/layout identity:
#   lib/freebasic/<FBC_TARGET>/
#
# IMPORTANT HARDENING:
# Some build environments export FBC_TARGET as a GNU triplet (e.g.
# powerpc64le-linux-gnu). That must NOT become the runtime dir key.
# If FBC_TARGET looks like a triplet, ignore it and compute canonical form.
##############################################################################

# Detect "looks like a GNU triplet" (>=3 dash-separated tokens, contains "gnu",
# "musl", "uclibc", or an OS token we already detect).
_fbc_target_tokens := $(subst -, ,$(strip $(FBC_TARGET)))
_fbc_target_ntok   := $(words $(_fbc_target_tokens))
_fbc_target_smells_like_triplet := \
  $(if $(filter gnu gnueabi gnueabihf musl uclibc uclibceabi,$(_fbc_target_tokens)),yes,)

ifdef FBC_TARGET
  ifneq ($(strip $(_fbc_target_smells_like_triplet)),)
    override FBC_TARGET :=
  else ifneq ($(filter 3 4 5 6,$(_fbc_target_ntok)),)
    # If it’s multi-token and not a known fbc target family, treat as suspicious
    # (this catches powerpc64le-linux-gnu, arm-linux-gnueabihf, etc.)
    ifeq ($(filter win32 win64 dos cygwin,$(FBC_TARGET)),)
      override FBC_TARGET :=
    endif
  endif
endif

ifndef FBC_TARGET
  ifeq ($(TARGET_OS),win32)
    ifeq ($(TARGET_ARCH),x86_64)
      FBC_TARGET := win64
    else
      FBC_TARGET := win32
    endif
  else ifeq ($(TARGET_OS),dos)
    FBC_TARGET := dos
  else
    FBC_TARGET := $(TARGET_OS)-$(TARGET_ARCH)
  endif
endif


##############################################################################
# 6) Runtime directory identity (CANONICAL)
##############################################################################

ifdef FBTARGET_DIR_OVERRIDE
  FBTARGET_DIR := $(FBTARGET_DIR_OVERRIDE)
else
  FBTARGET_DIR := $(FBC_TARGET)
endif

# Canonical build identity: MUST match runtime layout key
FBTARGET := $(FBTARGET_DIR)


##############################################################################
# 7) Packaging directory names (OPTIONAL / NOT CANONICAL)
##############################################################################

FBPACK_DIR := $(FBTARGET_DIR)

ifeq ($(TARGET_OS),linux)

  ifeq ($(TARGET_ARCH),x86_64)
    FBPACK_DIR := linux-amd64

  else ifeq ($(TARGET_ARCH),x86)
    FBPACK_DIR := linux-i386

  else ifeq ($(TARGET_ARCH),aarch64)
    FBPACK_DIR := linux-arm64

  else ifeq ($(TARGET_ARCH),arm)
    ifeq ($(ARM_FLOAT_ABI),hf)
      FBPACK_DIR := linux-armhf
    else
      FBPACK_DIR := linux-armel
    endif

  else ifeq ($(TARGET_ARCH),powerpc64le)
    FBPACK_DIR := linux-ppc64el

  else ifeq ($(TARGET_ARCH),riscv64)
    FBPACK_DIR := linux-riscv64

  else ifeq ($(TARGET_ARCH),s390x)
    FBPACK_DIR := linux-s390x

  else ifeq ($(TARGET_ARCH),loongarch64)
    FBPACK_DIR := linux-loongarch64

  else
    FBPACK_DIR := linux-$(TARGET_ARCH)
  endif

else ifeq ($(TARGET_OS),win32)

  ifeq ($(TARGET_ARCH),x86_64)
    FBPACK_DIR := mingw-x86_64
  else
    FBPACK_DIR := mingw-x86
  endif

else
  FBPACK_DIR := $(TARGET_OS)-$(TARGET_ARCH)
endif


##############################################################################
# 8) Executable suffix
##############################################################################

EXEEXT :=
ifneq ($(filter win32 win64 cygwin dos,$(TARGET_OS)),)
  EXEEXT := .exe
endif


##############################################################################
# 9) Debug helper
##############################################################################

.PHONY: platform-print
platform-print:
	@echo "TARGET_TRIPLET=$(TARGET_TRIPLET)"
	@echo "HOST_TRIPLET=$(HOST_TRIPLET)"
	@echo "TARGET_OS=$(TARGET_OS)"
	@echo "TARGET_ARCH_RAW=$(TARGET_ARCH_RAW)"
	@echo "TARGET_ARCH=$(TARGET_ARCH)"
	@echo "ISA_FAMILY=$(ISA_FAMILY)"
	@echo "ARM_FLOAT_ABI=$(ARM_FLOAT_ABI)"
	@echo "FBC_TARGET=$(FBC_TARGET)"
	@echo "FBTARGET_DIR=$(FBTARGET_DIR)"
	@echo "FBTARGET=$(FBTARGET)"
	@echo "FBPACK_DIR=$(FBPACK_DIR)"
	@echo "EXEEXT=$(EXEEXT)"


##############################################################################
# End platform.mk
##############################################################################
