########################
# toolchain-flags.mk
########################
#
# AROS SDK toolchain policy
#
# Responsibilities
#   - select the architecture contract of each supported AROS SDK
#   - expose AROS feature-test macros to runtime and library sources
#   - keep AROS object-format and warning policy out of generic architectures
#
# Inputs
#   TARGET_ARCH
#   TOOLCHAIN_* variables initialized by mk/toolchain-flags.mk
#
# This file intentionally does NOT contain
#   - generic m68k architecture identity or alignment rules
#   - non-AROS processor baselines
#   - Amiga Hunk packaging or emulator transport
#
########################


##############################################################################
# AROS declarations and library features
##############################################################################

# AROS keeps POSIX.1-2008 declarations behind the standard feature-test macro
# even though libposixc provides the implementations by default.
CPPFLAGS += -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700
CPPFLAGS += -DFB_SFX_MT_ENABLED=1


##############################################################################
# SDK architecture contracts
##############################################################################

ifeq ($(TARGET_ARCH),m68k)
  AROS_MACHDEP := -march=68000 -msoft-float
  # The m68k SDK declares the stack pointer as an AROS global register
  # variable for inline library vectors. GCC 6 warns at every containing
  # function even though the SDK-provided call sequence is intentional.
  AROS_WARNING_FLAGS := -Wno-volatile-register-var
else ifeq ($(TARGET_ARCH),arm)
  AROS_MACHDEP := -march=armv7-a -marm -mfloat-abi=hard -mfpu=neon-vfpv4
else ifeq ($(TARGET_ARCH),x86_64)
  AROS_MACHDEP := -m64 -mcmodel=large -mno-red-zone
endif

# AROS LoadSeg rejects ELF SHN_COMMON symbols, and elf2hunk cannot translate
# them for m68k releases. This object-format contract is independent of the
# reusable architecture baselines.
AROS_OBJECT_FLAGS := -fno-common


##############################################################################
# Toolchain realization
##############################################################################

TOOLCHAIN_CFLAGS   += $(AROS_MACHDEP) $(AROS_OBJECT_FLAGS) $(AROS_WARNING_FLAGS)
TOOLCHAIN_CXXFLAGS += $(AROS_MACHDEP) $(AROS_OBJECT_FLAGS) $(AROS_WARNING_FLAGS)

# The AROS driver already emits a self-contained executable module. Its
# -static option changes the C library selection and drops libposixc, so it
# must not be confused with the static-link policy used on ELF Unix hosts.
TOOLCHAIN_LDFLAGS += $(AROS_MACHDEP)

AROS_FBC_STAGE_FLAGS := \
  $(foreach option,$(AROS_MACHDEP) $(AROS_OBJECT_FLAGS) $(AROS_WARNING_FLAGS),-Wc $(option))
TOOLCHAIN_FBCFLAGS   += $(AROS_FBC_STAGE_FLAGS)
TOOLCHAIN_FBRTCFLAGS += $(AROS_FBC_STAGE_FLAGS)
TOOLCHAIN_FBRTLFLAGS += $(AROS_FBC_STAGE_FLAGS)


########################
# end of toolchain-flags.mk
########################
