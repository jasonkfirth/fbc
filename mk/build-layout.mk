###################
# build-layout.mk #
###################

##############################################################################
# Build layout derivation
#
# Responsibilities:
#   1) Define canonical per-target build directories
#   2) Define build-tree artifact locations
#   3) Export build paths for other build rules
#
# Provided by earlier layers:
#   platform.mk → FBTARGET
#   layout.mk   → libdirname
#
# IMPORTANT:
#   This file must NOT define install layout. That belongs in layout.mk.
##############################################################################

##############################################################################
# Canonical per-target build directories
##############################################################################

# Canonical runtime/layout key
libsubdir := $(FBTARGET)

# Object subdir mirrors runtime identity
objsubdir := $(libsubdir)


##############################################################################
# BUILD TREE LAYOUT
##############################################################################

ifdef ENABLE_STANDALONE

# Standalone builds keep executables at root
build_bindir := .
build_libdir := lib/$(libsubdir)

FBC_EXE    := fbc$(EXEEXT)
FBCNEW_EXE := fbc-new$(EXEEXT)

else

# Standard build layout
build_bindir := bin
build_libdir := $(libdirname)/$(FBNAME)/$(libsubdir)

FBC_EXE    := $(build_bindir)/fbc$(ENABLE_SUFFIX)$(EXEEXT)
FBCNEW_EXE := $(build_bindir)/fbc$(ENABLE_SUFFIX)-new$(EXEEXT)

endif


##############################################################################
# Canonical build runtime directory
##############################################################################

# Runtime libraries used during build
libdir := $(build_libdir)


##############################################################################
# Object directory layout
#
# IMPORTANT:
# These paths MUST match the expectations inside the original build rules.
# Do NOT move these outside src/.../obj/ without rewriting all pattern rules.
##############################################################################

# Compiler objects
fbcobjdir := src/compiler/obj/$(FBTARGET)

# Runtime objects
libfbobjdir	 := src/rtlib/obj/$(objsubdir)
libfbpicobjdir      := src/rtlib/obj/$(objsubdir)/pic
libfbmtobjdir       := src/rtlib/obj/$(objsubdir)/mt
libfbmtpicobjdir    := src/rtlib/obj/$(objsubdir)/mt/pic

# FB runtime layer
libfbrtobjdir       := src/fbrt/obj/$(objsubdir)
libfbrtpicobjdir    := src/fbrt/obj/$(objsubdir)/pic
libfbrtmtobjdir     := src/fbrt/obj/$(objsubdir)/mt
libfbrtmtpicobjdir  := src/fbrt/obj/$(objsubdir)/mt/pic

# Graphics runtime
libfbgfxobjdir      := src/gfxlib2/obj/$(objsubdir)
libfbgfxpicobjdir   := src/gfxlib2/obj/$(objsubdir)/pic
libfbgfxmtobjdir    := src/gfxlib2/obj/$(objsubdir)/mt
libfbgfxmtpicobjdir := src/gfxlib2/obj/$(objsubdir)/mt/pic

# Sound runtime

libsfxobjdir       := src/sfxlib/obj/$(objsubdir)
libsfxpicobjdir    := src/sfxlib/obj/$(objsubdir)/pic
libsfxmtobjdir     := src/sfxlib/obj/$(objsubdir)/mt
libsfxmtpicobjdir  := src/sfxlib/obj/$(objsubdir)/mt/pic


##############################################################################
# Export useful build paths
##############################################################################

export libdir

##########################
# end of build-layout.mk #
##########################
