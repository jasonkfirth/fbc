###############
# multilib.mk #
###############
# Project: FreeBASIC build system
# Purpose: Apply the already selected x86 ABI width to every compiler stage.
# Responsibilities: Forward matching BASIC, C, C++, and linker width flags.
# This file intentionally does NOT resolve platform identity or SDK paths.

##############################################################################
# Multilib / -arch plumbing
#
# Fixes:
#  - Do NOT pass GNU triplets (e.g. x86_64-w64-mingw32) to fbc's -target.
#    fbc wants lowering targets (win32/win64/linux-x86_64/...)
#    and that is already handled in compiler-config.mk via FB_LOWER_TARGET.
#  - Keep the selected x86 width consistent across BASIC, C, C++, and linking.
#    platform.mk resolves the effective target and rejects incompatible widths
#    before layout or source discovery. This file only realizes tool flags.
##############################################################################

# fbc -target is handled in compiler-config.mk (FB_LOWER_TARGET), not here.

ifdef MULTILIB
  ALLFBCFLAGS    += -arch $(MULTILIB)
  ALLFBLFLAGS    += -arch $(MULTILIB)
  ALLFBRTCFLAGS  += -arch $(MULTILIB)
  ALLFBRTLFLAGS  += -arch $(MULTILIB)

  ALLCFLAGS   += -m$(MULTILIB)
  ALLCXXFLAGS += -m$(MULTILIB)
  ALLLDFLAGS  += -m$(MULTILIB)
endif

######################
# end of multilib.mk #
######################
