##############################################################################
# FreeBASIC Android toolchain policy
##############################################################################
#
# File: mk/android/toolchain-flags.mk
#
# Purpose:
#   Keep Android runtime objects on the instruction-set baseline selected by
#   the public Android target.
#
# Responsibilities:
#   - override recent NDK Clang ARMv7 defaults that enable NEON implicitly
#   - preserve the armeabi-v7a VFPv3-D16 floating-point baseline
#
# This file intentionally does NOT contain:
#   - optional NEON kernel flags
#   - run-time CPU capability checks
#   - Android SDK or NDK discovery
#
##############################################################################

ifeq ($(TARGET_ARCH),arm)
  ifeq ($(ARM_VER),v7)
    # The compiler emits the same flags for normal Android ARMv7 programs.
    # gfxlib2 and sfxlib append -mfpu=neon only to their isolated SIMD objects,
    # which are selected after checking AT_HWCAP at run time.
    ANDROID_ARMV7_BASELINE_CFLAGS := \
      -mfloat-abi=softfp \
      -mfpu=vfpv3-d16
    TOOLCHAIN_CFLAGS   += $(ANDROID_ARMV7_BASELINE_CFLAGS)
    TOOLCHAIN_CXXFLAGS += $(ANDROID_ARMV7_BASELINE_CFLAGS)
  endif
endif

##############################################################################
# end of mk/android/toolchain-flags.mk
##############################################################################
