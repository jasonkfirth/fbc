###############
# os-flags.mk #
###############
#
# OS-specific low-level toolchain adjustments.
#
# Responsibilities:
#   - Apply narrowly-scoped ABI/compiler quirks
#   - Avoid feature policy (feature-policy.mk handles that)
#   - Avoid warning policy (toolchain-flags.mk handles that)
#
# This file must remain minimal and predictable.
#
# Inputs expected:
#   TARGET_OS
#
# Modifies:
#   ALLCFLAGS
#   ALLLDFLAGS
#
###############

# ---------------------------------------------------------------------------
# Guard
# ---------------------------------------------------------------------------

ifeq ($(strip $(TARGET_OS)),)
  $(error os-flags.mk: TARGET_OS not defined (include platform.mk first))
endif

# ---------------------------------------------------------------------------
# Windows (MinGW / native win32)
#
# -fno-ident prevents GCC from emitting .ident ELF metadata strings,
# which are meaningless on PE/COFF and occasionally trigger reproducibility
# noise across toolchains.
# ---------------------------------------------------------------------------

ifeq ($(TARGET_OS),win32)
  ALLCFLAGS += -fno-ident
endif

# ---------------------------------------------------------------------------
# Cygwin
#
# Cygwin uses ELF internally; no need for -fno-ident suppression.
# ---------------------------------------------------------------------------

# (intentionally empty)

# ---------------------------------------------------------------------------
# DOS (djgpp)
#
# DJGPP toolchain does not require special flags here.
# ---------------------------------------------------------------------------

# (intentionally empty)

# ---------------------------------------------------------------------------
# Android
#
# Bionic does not fully implement glibc; some toolchains require
# position-independent code by default (handled elsewhere).
# No additional flags here.
# ---------------------------------------------------------------------------

# (intentionally empty)

# ---------------------------------------------------------------------------
# Darwin / BSD / Linux / Solaris / Haiku
#
# No unconditional OS-specific flags required here.
# Hardening and warnings are handled elsewhere.
# ---------------------------------------------------------------------------

# (intentionally empty)

######################
# end of os-flags.mk #
######################
