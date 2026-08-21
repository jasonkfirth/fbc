##############################################################################
# wince/toolchain-flags.mk
#
# Windows CE cross-toolchain policy.
#
# Responsibilities:
#   - keep the supported ARM baseline aligned with classic CE devices
#   - keep the supported MIPS baseline aligned with classic CE devices
#   - preserve each toolchain's software floating-point ABI
#   - forward the same contract through staged FreeBASIC compilations
#
# Non-responsibilities:
#   - tool selection and prefix discovery
#   - Windows CE runtime, graphics, or sound implementation
#   - emulator and package orchestration
##############################################################################

ifeq ($(TARGET_ARCH),arm)
WINCE_ARCH_FLAGS := \
  -march=armv4t \
  -mfloat-abi=soft \
  -D_WIN32_WCE=0x0500

TOOLCHAIN_CFLAGS   += $(WINCE_ARCH_FLAGS)
TOOLCHAIN_CXXFLAGS += $(WINCE_ARCH_FLAGS)
TOOLCHAIN_LDFLAGS  += $(WINCE_ARCH_FLAGS)

WINCE_FBC_STAGE_FLAGS := \
  -arch armv4 \
  -Wc -march=armv4t \
  -Wc -mfloat-abi=soft \
  -Wc -D_WIN32_WCE=0x0500

TOOLCHAIN_FBCFLAGS   += $(WINCE_FBC_STAGE_FLAGS)
TOOLCHAIN_FBRTCFLAGS += $(WINCE_FBC_STAGE_FLAGS)
TOOLCHAIN_FBRTLFLAGS += $(WINCE_FBC_STAGE_FLAGS)
endif

ifeq ($(TARGET_ARCH),mips32el)
WINCE_ARCH_FLAGS := \
  -mcpu=mips2 \
  -mno-check-zero-division \
  -mabi=32 \
  -msoft-float \
  -ffreestanding \
  -fno-builtin \
  -D__MINGW32CE__ \
  -D__MINGW32__ \
  -D__COREDLL__ \
  -DFFI_STATIC_BUILD \
  -D_WIN32_WCE=0x0500 \
  -D_M_MRX000=4000 \
  -DMIPS

CPPFLAGS := -I$(srcdir)/rtlib/wince/mips32el $(CPPFLAGS)
TOOLCHAIN_CFLAGS   += $(WINCE_ARCH_FLAGS)
TOOLCHAIN_CXXFLAGS += $(WINCE_ARCH_FLAGS)

WINCE_FBC_STAGE_FLAGS := \
  -gen clang \
  -arch mips32el \
  -Wc -mcpu=mips2 \
  -Wc -mno-check-zero-division \
  -Wc -D__MINGW32CE__ \
  -Wc -D__MINGW32__ \
  -Wc -D__COREDLL__ \
  -Wc -DFFI_STATIC_BUILD \
  -Wc -D_WIN32_WCE=0x0500 \
  -Wc -D_M_MRX000=4000 \
  -Wc -DMIPS

TOOLCHAIN_FBCFLAGS   += $(WINCE_FBC_STAGE_FLAGS)
TOOLCHAIN_FBRTCFLAGS += $(WINCE_FBC_STAGE_FLAGS)
TOOLCHAIN_FBRTLFLAGS += $(WINCE_FBC_STAGE_FLAGS)
endif

##############################################################################
# end of wince/toolchain-flags.mk
##############################################################################
