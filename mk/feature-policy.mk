#####################
# feature-policy.mk #
#####################
#
# High-level feature policy layer.
#
# This file:
#   - Applies compile-time feature defines (via ALLCFLAGS / ALLFBLFLAGS)
#   - Does NOT detect platform (platform.mk does that)
#   - Does NOT decide PIC (platform-features.mk does that)
#
# Inputs expected:
#   TARGET_OS
#   MAKECMDGOALS
#
# Defines/Modifies:
#   ALLCFLAGS
#   ALLFBLFLAGS
#   DISABLE_MT   (may be set here for special targets)
#
#####################

# ---------------------------------------------------------------------------
# Guard
# ---------------------------------------------------------------------------

ifeq ($(strip $(TARGET_OS)),)
  $(error feature-policy.mk: TARGET_OS not defined (include platform.mk first))
endif

# ---------------------------------------------------------------------------
# Bootstrap-minimal policy
#
# Strip optional subsystems to reduce dependency surface.
# ---------------------------------------------------------------------------

ifneq ($(strip $(filter bootstrap-minimal,$(MAKECMDGOALS)) $(BOOTSTRAP_MINIMAL)),)
  DISABLE_X11 := YesPlease
  ALLCFLAGS += \
    -DDISABLE_GPM \
    -DDISABLE_FFI \
    -DDISABLE_X11 \
    -DDISABLE_NCURSES \
    -DDISABLE_LANGINFO
endif

# ---------------------------------------------------------------------------
# DOS (djgpp)
# ---------------------------------------------------------------------------

ifeq ($(TARGET_OS),dos)
  ALLCFLAGS += \
    -DDISABLE_WCHAR \
    -DDISABLE_FFI \
    -DDISABLE_X11 \
    -DDISABLE_NCURSES \
    -DDISABLE_LANGINFO
endif

# ---------------------------------------------------------------------------
# Android
#
# Android Bionic does not provide full glibc surface.
# ---------------------------------------------------------------------------

ifeq ($(TARGET_OS),android)
  ALLCFLAGS += \
    -DDISABLE_NCURSES \
    -DDISABLE_X11 \
    -DDISABLE_FFI \
    -DDISABLE_LANGINFO \
    -DDISABLE_GPM
endif

# ---------------------------------------------------------------------------
# JavaScript backend
# ---------------------------------------------------------------------------

ifeq ($(TARGET_OS),js)
  DISABLE_MT := YesPlease
endif

# ---------------------------------------------------------------------------
# Windows console stack size
#
# Keep console stack conservative but non-fragile.
# ---------------------------------------------------------------------------

ifneq ($(filter win32 cygwin,$(TARGET_OS)),)
  ALLFBLFLAGS += -t 2048
endif

############################
# end of feature-policy.mk #
############################
