################
# cpu.mk
################
#
# Consolidated CPU sub-architecture and default CPU policy
#
# Replaces:
#   cpu-subarch.mk
#   cpu-defaults.mk
#
# Responsibilities:
#   - Determine ARM sub-architecture version (ARM_VER)
#   - Determine ARM floating ABI (ARM_FLOAT_ABI fallback)
#   - Provide DEFAULT_CPUTYPE_ARM for compiler configuration
#
# Non-responsibilities:
#   - ISA family detection (handled in platform.mk)
#   - TARGET_ARCH normalization (handled in platform.mk)
#
################


##############################################################################
# ARM sub-architecture detection
##############################################################################

ARM_VER :=

ifeq ($(TARGET_ARCH),arm)

  ifneq ($(filter armv8%,$(TARGET_ARCH_RAW)),)
    ARM_VER := v8
  else ifneq ($(filter armv7%,$(TARGET_ARCH_RAW)),)
    ARM_VER := v7
  else ifneq ($(filter armv6%,$(TARGET_ARCH_RAW)),)
    ARM_VER := v6
  else ifneq ($(filter armv5%,$(TARGET_ARCH_RAW)),)
    ARM_VER := v5
  else ifneq ($(filter armv4%,$(TARGET_ARCH_RAW)),)
    ARM_VER := v4
  endif

  # GCCSDK targets classic APCS-32 machines.  Keep an unqualified
  # arm-unknown-riscos triplet on the StrongARM-compatible baseline defined by
  # this port instead of inheriting a newer host compiler default.
  ifeq ($(TARGET_OS),riscos)
    ARM_VER := v4
  endif

endif


##############################################################################
# ARM float ABI detection (fallback)
#
# platform.mk normally sets ARM_FLOAT_ABI using the GNU triplet.
# This block provides a fallback if it was not already set.
##############################################################################

ifndef ARM_FLOAT_ABI

ifeq ($(TARGET_ARCH),arm)

  ifneq ($(filter armhf,$(TARGET_ARCH_RAW)),)
    ARM_FLOAT_ABI := hf
  else
    ARM_FLOAT_ABI := soft
  endif

endif

endif


##############################################################################
# Default ARM CPU type selection
#
# This provides a compiler-level CPU type constant used by the runtime
# when the architecture requires a subtype.
##############################################################################

ifeq ($(TARGET_ARCH),arm)

ARM_FLOAT_ABI ?= soft

ifndef ARM_VER
  ifeq ($(ARM_FLOAT_ABI),hf)
    ARM_VER := v7
  else
    ARM_VER := v6
  endif
endif

ifndef DEFAULT_CPUTYPE_ARM

  ifeq ($(ARM_FLOAT_ABI),hf)

    ifeq ($(ARM_VER),v6)
      DEFAULT_CPUTYPE_ARM := FB_CPUTYPE_ARMV6_FP
    else
      DEFAULT_CPUTYPE_ARM := FB_CPUTYPE_ARMV7A_FP
    endif

  else

    ifeq ($(ARM_VER),v4)
      DEFAULT_CPUTYPE_ARM := FB_CPUTYPE_ARMV4
    else ifeq ($(ARM_VER),v5)
      DEFAULT_CPUTYPE_ARM := FB_CPUTYPE_ARMV5TE
    else ifeq ($(ARM_VER),v6)
      DEFAULT_CPUTYPE_ARM := FB_CPUTYPE_ARMV6
    else
      DEFAULT_CPUTYPE_ARM := FB_CPUTYPE_ARMV7A
    endif

  endif

endif

endif


##############################################################################
# Export CPU configuration
##############################################################################

export ARM_VER
export ARM_FLOAT_ABI
export DEFAULT_CPUTYPE_ARM


####################
# end of cpu.mk
####################
