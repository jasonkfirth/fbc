#############
# naming.mk #
#############

##############################################################################
# Executable base name and linker script naming
#
# Conventions used by the mk/ build:
#   - FBNAME is the installed “include/” namespace name (and may be suffixed).
#   - fbextra.x is ALWAYS the linker *augmentation* script (discard .fbctinf).
#   - FB_LDSCRIPT is the *primary* runtime linker script and must NOT be named
#     fbextra.x (archives.mk will fail fast if it collides).
#
# This file only names things. It should not create files or directories.
##############################################################################

##############################################################################
# Sanity defaults if ENABLE_SUFFIX is unset
##############################################################################

ENABLE_SUFFIX ?=

##############################################################################
# FBNAME (include namespace / branding)
##############################################################################

# Default naming (non-DOS)
FBNAME := freebasic$(ENABLE_SUFFIX)

# DOS (djgpp/go32) historically uses an 8.3-ish name in some contexts
ifeq ($(TARGET_OS),dos)
  FBNAME := freebas$(ENABLE_SUFFIX)
endif

##############################################################################
# Linker script naming
##############################################################################

# fbextra.x is reserved for the augmentation script, never the primary script.
FBEXTRA_SCRIPT := fbextra.x

# DOS uses a specific primary linker script name.
ifeq ($(TARGET_OS),dos)
  FB_LDSCRIPT := i386go32.x
endif

# If FB_LDSCRIPT wasn't set by DOS or the environment, pick a safe primary name.
ifndef FB_LDSCRIPT
  # Prefer an upstream-provided primary script filename when present.
  # (These are common historical names in FreeBASIC trees.)
  ifneq ($(wildcard $(rootdir)/lib/fb.x),)
    FB_LDSCRIPT := fb.x
  else ifneq ($(wildcard $(rootdir)/lib/fbc.x),)
    FB_LDSCRIPT := fbc.x
  else ifneq ($(wildcard $(rootdir)/lib/ldscript.x),)
    FB_LDSCRIPT := ldscript.x
  else
    # Stable fallback (non-colliding with fbextra.x)
    FB_LDSCRIPT := fb-ldscript.x
  endif
endif

# Hard fix-up if something forces FB_LDSCRIPT=fbextra.x (collision).
ifeq ($(strip $(FB_LDSCRIPT)),$(FBEXTRA_SCRIPT))
  ifneq ($(wildcard $(rootdir)/lib/fb.x),)
    override FB_LDSCRIPT := fb.x
  else ifneq ($(wildcard $(rootdir)/lib/fbc.x),)
    override FB_LDSCRIPT := fbc.x
  else ifneq ($(wildcard $(rootdir)/lib/ldscript.x),)
    override FB_LDSCRIPT := ldscript.x
  else
    override FB_LDSCRIPT := fb-ldscript.x
  endif
endif

##############################################################################
# Windows / Cygwin notes:
#  - Keep FBNAME as "freebasic" (plus optional suffix) to match existing layout.
#  - Do NOT set FB_LDSCRIPT to fbextra.x here (that’s the augmentation script).
##############################################################################

ifeq ($(TARGET_OS),win32)
  FBNAME := freebasic$(ENABLE_SUFFIX)
endif

ifeq ($(TARGET_OS),cygwin)
  FBNAME := freebasic$(ENABLE_SUFFIX)
endif

####################
# end of naming.mk #
####################
