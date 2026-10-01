##############################################################################
# FreeBASIC classic AmigaOS port
#
# File: amiga/toolchain-flags.mk
#
# Purpose:
#   Keep the Amiga GCC Hunk and 68020 soft-float contract in one place.
#
# Responsibilities:
#   - apply the same ABI to runtime, graphics, sound, and generated BASIC C
#   - enable the sound worker even for programs using the ordinary runtime
#
# This file intentionally does NOT contain:
#   - generic m68k identity, runtime implementations, or emulator settings
##############################################################################

ifneq ($(TARGET_ARCH),m68k)
$(error AmigaOS currently supports only the m68k architecture)
endif

AMIGA_MACHDEP := -m68020 -msoft-float -fno-common
TOOLCHAIN_CFLAGS += $(AMIGA_MACHDEP)
TOOLCHAIN_CXXFLAGS += $(AMIGA_MACHDEP)
TOOLCHAIN_LDFLAGS += -m68020 -msoft-float
CPPFLAGS += -DFB_SFX_MT_ENABLED=1

AMIGA_FBC_STAGE_FLAGS := $(foreach option,$(AMIGA_MACHDEP),-Wc $(option))
TOOLCHAIN_FBCFLAGS += $(AMIGA_FBC_STAGE_FLAGS)
TOOLCHAIN_FBRTCFLAGS += $(AMIGA_FBC_STAGE_FLAGS)
TOOLCHAIN_FBRTLFLAGS += $(AMIGA_FBC_STAGE_FLAGS)

# end of amiga/toolchain-flags.mk
