###############
# multilib.mk #
###############

##############################################################################
# Multilib / -arch plumbing
#
# Fixes:
#  - Do NOT pass GNU triplets (e.g. x86_64-w64-mingw32) to fbc's -target.
#    fbc wants lowering targets (win32/win64/linux-x86_64/...)
#    and that is already handled in compiler-config.mk via FB_LOWER_TARGET.
#  - Keep MULTILIB working for fbc via -arch, and for GCC via -m where valid.
##############################################################################

# fbc -target is handled in compiler-config.mk (FB_LOWER_TARGET), not here.

ifdef MULTILIB
  ALLFBCFLAGS    += -arch $(MULTILIB)
  ALLFBLFLAGS    += -arch $(MULTILIB)
  ALLFBRTCFLAGS  += -arch $(MULTILIB)
  ALLFBRTLFLAGS  += -arch $(MULTILIB)

  # GCC/Clang multilib flags: only for platforms that accept -m32/-m64, etc.
  # Avoid ARM where -m$(MULTILIB) isn't meaningful in the same way.
  ifeq ($(filter arm arm64,$(ISA_FAMILY)),)
    ALLCFLAGS += -m$(MULTILIB)
  endif
endif

######################
# end of multilib.mk #
######################
