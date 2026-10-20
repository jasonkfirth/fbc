##############################################################################
# source-graph.mk
#
# Source discovery with override precedence
# Project: FreeBASIC build system
# Purpose: Resolve selected implementations into unambiguous object lists.
# Responsibilities: Apply platform precedence and reject archive-member collisions.
# This file intentionally does NOT invoke compilers or publish archives.
#
# precedence:
#   target_os/target_arch > target_os > unix > generic
#
# The shared unix layer only applies to Unix-family targets such as Linux,
# BSD, Solaris, Haiku, and Android. Cygwin uses the Win32 target-specific
# source trees because the runtime/compiler configuration identifies it as a
# Win32 host with Cygwin compatibility shims, not as a HOST_UNIX target.
#
# Object architecture:
#   canonical object list -> derived PIC/MT variants
##############################################################################

##############################################################################
# Search directories
##############################################################################

UNIX_LAYER_OS := linux android aros amiga darwin freebsd netbsd openbsd dragonfly solaris illumos haiku riscos

SOURCE_OS := $(TARGET_OS)
ifeq ($(TARGET_OS),illumos)
SOURCE_OS := solaris
endif
ifeq ($(TARGET_OS),cygwin)
SOURCE_OS := win32
endif

SFX_SOURCE_OS := $(SOURCE_OS)
ifeq ($(TARGET_OS),dos)
SFX_SOURCE_OS := msdos
endif

##############################################################################
# Runtime C++ enablement
#
# Only targets with known C++ runtime/backend requirements should compile
# runtime C++ sources. At the moment that is Haiku only.
#
# This avoids silently pulling C++ compilation into targets that should remain
# pure C unless there is a demonstrated platform requirement.
##############################################################################

USE_RUNTIME_CXX :=
ifeq ($(TARGET_OS),haiku)
USE_RUNTIME_CXX := yes
endif

USE_UNIX_LAYER :=
ifneq ($(filter $(UNIX_LAYER_OS),$(TARGET_OS)),)
USE_UNIX_LAYER := yes
endif

include $(mkpath)/rtlib-sources.mk

FBRT_DIRS := \
$(srcdir)/fbrt

include $(mkpath)/gfx-sources.mk

GFXLIB2_DIRS := $(srcdir)/gfxlib2 $(GFX2_GENERIC_DIRS)
ifneq ($(USE_UNIX_LAYER),)
GFXLIB2_DIRS += $(srcdir)/gfxlib2/drivers/unix
endif
GFXLIB2_DIRS += $(srcdir)/gfxlib2/drivers/$(SOURCE_OS)
ifeq ($(TARGET_ARCH),x86)
GFXLIB2_DIRS += $(srcdir)/gfxlib2/drivers/x86
endif
ifneq ($(filter x86_64 aarch64,$(TARGET_ARCH)),)
GFXLIB2_DIRS += $(srcdir)/gfxlib2/drivers/$(TARGET_ARCH)
endif
ifeq ($(TARGET_ARCH),arm)
  ifneq ($(filter v7 v8,$(ARM_VER)),)
GFXLIB2_DIRS += $(srcdir)/gfxlib2/drivers/arm
  endif
endif

GFXLIB3_PLATFORM_OS := android darwin linux win32

GFXLIB3_DIRS := $(srcdir)/gfxlib3 $(GFX3_GENERIC_DIRS)
ifneq ($(filter $(SOURCE_OS),$(GFXLIB3_PLATFORM_OS)),)
GFXLIB3_DIRS += $(srcdir)/gfxlib3/drivers/$(SOURCE_OS)
endif

SFXLIB_DIRS := $(srcdir)/sfxlib
ifneq ($(USE_UNIX_LAYER),)
SFXLIB_DIRS += $(srcdir)/sfxlib/unix
endif
SFXLIB_DIRS += $(srcdir)/sfxlib/$(SOURCE_OS)
ifneq ($(filter x86 x86_64 aarch64,$(TARGET_ARCH)),)
SFXLIB_DIRS += $(srcdir)/sfxlib/$(TARGET_ARCH)
endif
ifeq ($(TARGET_ARCH),arm)
  ifneq ($(filter v7 v8,$(ARM_VER)),)
SFXLIB_DIRS += $(srcdir)/sfxlib/arm
  endif
endif

##############################################################################
# Compiler sources
##############################################################################

include $(mkpath)/compiler-sources.mk

##############################################################################
# RTLIB sources
##############################################################################

# Discovery and platform precedence are owned by rtlib-sources.mk above.

##############################################################################
# FBRT sources
##############################################################################

FBRT_SRC := $(wildcard $(srcdir)/fbrt/*.bas)

##############################################################################
# GFXLIB2 sources
##############################################################################

GFX_SRC_GENERIC := $(foreach directory,$(GFX2_GENERIC_DIRS),\
$(wildcard $(directory)/*.c))

ifneq ($(USE_RUNTIME_CXX),)
GFX_SRC_GENERIC += $(foreach directory,$(GFX2_GENERIC_DIRS),\
$(wildcard $(directory)/*.cpp))
endif

GFX_SRC_UNIX :=
ifneq ($(USE_UNIX_LAYER),)
 GFX_SRC_UNIX := \
$(wildcard $(srcdir)/gfxlib2/drivers/unix/*.c)
 ifneq ($(USE_RUNTIME_CXX),)
GFX_SRC_UNIX += $(wildcard $(srcdir)/gfxlib2/drivers/unix/*.cpp)
 endif
endif

GFX_SRC_TARGET := \
$(wildcard $(srcdir)/gfxlib2/drivers/$(SOURCE_OS)/*.c)

ifneq ($(USE_RUNTIME_CXX),)
GFX_SRC_TARGET += $(wildcard $(srcdir)/gfxlib2/drivers/$(SOURCE_OS)/*.cpp)
endif

GFX_SRC_TARGET_ARCH := \
$(wildcard $(srcdir)/gfxlib2/drivers/$(SOURCE_OS)/$(TARGET_ARCH)/*.c)

ifneq ($(USE_RUNTIME_CXX),)
GFX_SRC_TARGET_ARCH += $(wildcard $(srcdir)/gfxlib2/drivers/$(SOURCE_OS)/$(TARGET_ARCH)/*.cpp)
endif

GFX_SRC_ARCH :=
ifeq ($(TARGET_ARCH),x86)
GFX_SRC_ARCH := $(wildcard $(srcdir)/gfxlib2/drivers/x86/*.s)
endif

GFX_SRC_ARCH_C :=
ifneq ($(filter x86_64 aarch64,$(TARGET_ARCH)),)
GFX_SRC_ARCH_C := $(wildcard $(srcdir)/gfxlib2/drivers/$(TARGET_ARCH)/*.c)
endif
ifeq ($(TARGET_ARCH),arm)
  ifneq ($(filter v7 v8,$(ARM_VER)),)
GFX_SRC_ARCH_C := $(wildcard $(srcdir)/gfxlib2/drivers/arm/*.c)
  endif
endif

GFX_SRC_TARGET_ASM := $(wildcard $(srcdir)/gfxlib2/drivers/$(SOURCE_OS)/*.s)

GFX_BASE_GENERIC := $(notdir $(GFX_SRC_GENERIC))
GFX_BASE_UNIX := $(notdir $(GFX_SRC_UNIX))
GFX_BASE_TARGET := $(notdir $(GFX_SRC_TARGET))
GFX_BASE_TARGET_ARCH := $(notdir $(GFX_SRC_TARGET_ARCH))
GFX_BASE_ARCH_C := $(notdir $(GFX_SRC_ARCH_C))

GFX_SRC_TARGET := $(filter-out \
$(addprefix $(srcdir)/gfxlib2/drivers/$(SOURCE_OS)/,$(GFX_BASE_TARGET_ARCH)), \
$(GFX_SRC_TARGET))

GFX_SRC_UNIX := $(filter-out \
$(addprefix $(srcdir)/gfxlib2/drivers/unix/,$(GFX_BASE_TARGET)), \
$(GFX_SRC_UNIX))

GFX_SRC_GENERIC := $(filter-out \
$(foreach filename,$(GFX_BASE_UNIX) $(GFX_BASE_TARGET) \
$(GFX_BASE_TARGET_ARCH) $(GFX_BASE_ARCH_C),%/$(filename)), \
$(GFX_SRC_GENERIC))

GFX_SRC := $(GFX_SRC_GENERIC) $(GFX_SRC_UNIX) $(GFX_SRC_TARGET) \
$(GFX_SRC_TARGET_ARCH) $(GFX_SRC_ARCH_C) $(GFX_SRC_ARCH) \
$(GFX_SRC_TARGET_ASM)

ifdef DISABLE_X11
GFX_SRC := $(filter-out \
$(srcdir)/gfxlib2/drivers/unix/gfx_driver_opengl_x11.c \
$(srcdir)/gfxlib2/drivers/unix/gfx_driver_x11.c \
$(srcdir)/gfxlib2/drivers/unix/gfx_x11.c \
$(srcdir)/gfxlib2/drivers/unix/gfx_x11_icon_stub.c, \
$(GFX_SRC))
endif

ifdef DISABLE_OPENGL
GFX_SRC := $(filter-out \
$(srcdir)/gfxlib2/drivers/darwin/gfx_driver_opengl.c \
$(srcdir)/gfxlib2/drivers/haiku/gfx_driver_opengl.cpp \
$(srcdir)/gfxlib2/drivers/haiku/haiku_gl_view.cpp, \
$(GFX_SRC))
endif

##############################################################################
# GFXLIB3 sources
##############################################################################

# gfxlib3 currently has native Android/EGL, Darwin/Cocoa, Linux/X11,
# Linux DRM/GBM/EGL, and Win32/WGL adapters. Target sources override generic
# stubs with the same basename, matching the source precedence used by
# gfxlib2, rtlib, and sfxlib.
GFX3_SRC_GENERIC := $(foreach directory,$(GFX3_GENERIC_DIRS),\
$(wildcard $(directory)/*.c))
GFX3_SRC_TARGET :=
ifneq ($(filter $(SOURCE_OS),$(GFXLIB3_PLATFORM_OS)),)
GFX3_SRC_TARGET := $(wildcard $(srcdir)/gfxlib3/drivers/$(SOURCE_OS)/*.c)
endif

GFX3_BASE_TARGET := $(notdir $(GFX3_SRC_TARGET))
GFX3_SRC_GENERIC := $(filter-out \
$(foreach filename,$(GFX3_BASE_TARGET),%/$(filename)), \
$(GFX3_SRC_GENERIC))

GFX3_SRC := $(GFX3_SRC_TARGET) $(GFX3_SRC_GENERIC)

# ar stores member basenames even though objects retain their source paths.
# Reject duplicate selected names rather than silently replacing an archive
# member when someone adds a unit in the wrong subsystem.
GFX_ARCHIVE_MEMBERS := $(addsuffix .o,$(basename $(notdir $(GFX_SRC))))
GFX3_ARCHIVE_MEMBERS := $(addsuffix .o,$(basename $(notdir $(GFX3_SRC))))
GFX_DUPLICATE_BASENAMES := $(strip $(foreach member,\
$(sort $(GFX_ARCHIVE_MEMBERS)),\
$(if $(word 2,$(filter $(member),$(GFX_ARCHIVE_MEMBERS))),$(member))))
GFX3_DUPLICATE_BASENAMES := $(strip $(foreach member,\
$(sort $(GFX3_ARCHIVE_MEMBERS)),\
$(if $(word 2,$(filter $(member),$(GFX3_ARCHIVE_MEMBERS))),$(member))))
ifneq ($(GFX_DUPLICATE_BASENAMES),)
$(error Duplicate gfxlib2 source basenames: $(GFX_DUPLICATE_BASENAMES))
endif
ifneq ($(GFX3_DUPLICATE_BASENAMES),)
$(error Duplicate gfxlib3 source basenames: $(GFX3_DUPLICATE_BASENAMES))
endif

##############################################################################
# SFXLIB sources
##############################################################################

SFX_SRC_GENERIC := \
$(wildcard $(srcdir)/sfxlib/*.c)

ifneq ($(USE_RUNTIME_CXX),)
SFX_SRC_GENERIC += $(wildcard $(srcdir)/sfxlib/*.cpp)
endif

SFX_SRC_UNIX :=
ifneq ($(USE_UNIX_LAYER),)
 SFX_SRC_UNIX := \
$(wildcard $(srcdir)/sfxlib/unix/*.c)
 ifneq ($(USE_RUNTIME_CXX),)
SFX_SRC_UNIX += $(wildcard $(srcdir)/sfxlib/unix/*.cpp)
 endif
endif

SFX_SRC_TARGET := \
$(wildcard $(srcdir)/sfxlib/$(SOURCE_OS)/*.c)

ifneq ($(USE_RUNTIME_CXX),)
SFX_SRC_TARGET += $(wildcard $(srcdir)/sfxlib/$(SOURCE_OS)/*.cpp)
endif

SFX_SRC_ARCH_C :=
ifneq ($(filter x86 x86_64 aarch64,$(TARGET_ARCH)),)
SFX_SRC_ARCH_C := $(wildcard $(srcdir)/sfxlib/$(TARGET_ARCH)/*.c)
endif
ifeq ($(TARGET_ARCH),arm)
  ifneq ($(filter v7 v8,$(ARM_VER)),)
SFX_SRC_ARCH_C := $(wildcard $(srcdir)/sfxlib/arm/*.c)
  endif
endif

SFX_BASE_GENERIC := $(notdir $(SFX_SRC_GENERIC))
SFX_BASE_UNIX := $(notdir $(SFX_SRC_UNIX))
SFX_BASE_TARGET := $(notdir $(SFX_SRC_TARGET))
SFX_BASE_ARCH_C := $(notdir $(SFX_SRC_ARCH_C))

SFX_SRC_UNIX := $(filter-out \
$(addprefix $(srcdir)/sfxlib/unix/,$(SFX_BASE_TARGET)), \
$(SFX_SRC_UNIX))

SFX_SRC_GENERIC := $(filter-out \
$(addprefix $(srcdir)/sfxlib/,$(SFX_BASE_UNIX)) \
$(addprefix $(srcdir)/sfxlib/,$(SFX_BASE_TARGET)) \
$(addprefix $(srcdir)/sfxlib/,$(SFX_BASE_ARCH_C)), \
$(SFX_SRC_GENERIC))

SFX_MIDI_TARGET_OS := linux darwin haiku dos win32 wince cygwin xbox
SFX_MIDI_UNIX_OS := freebsd netbsd openbsd dragonfly solaris illumos

ifneq ($(filter $(SFX_MIDI_TARGET_OS) $(SFX_MIDI_UNIX_OS),$(TARGET_OS)),)
  SFX_SRC_GENERIC := $(filter-out \
    $(srcdir)/sfxlib/sfx_midi_driver_stub.c, \
    $(SFX_SRC_GENERIC))
endif

ifeq ($(filter $(SFX_MIDI_UNIX_OS),$(TARGET_OS)),)
  SFX_SRC_UNIX := $(filter-out \
    $(srcdir)/sfxlib/unix/sfx_midi_bsd.c, \
    $(SFX_SRC_UNIX))
endif

ifeq ($(TARGET_OS),darwin)
  # These Darwin files are override markers.  Their filenames reserve the
  # shared Unix slots above, but they intentionally do not contribute code.
  # Keeping them out of the archive avoids ranlib "has no symbols" warnings.
  SFX_SRC_TARGET := $(filter-out \
    $(srcdir)/sfxlib/darwin/sfx_midi_bsd.c \
    $(srcdir)/sfxlib/darwin/sfx_unix.c, \
    $(SFX_SRC_TARGET))
endif

ifeq ($(TARGET_OS),haiku)
  # These Haiku files are override markers.  Their filenames reserve the
  # shared Unix slots above, but they intentionally do not contribute code.
  # Keeping them out of the archive avoids ranlib "has no symbols" warnings.
  SFX_SRC_TARGET := $(filter-out \
    $(srcdir)/sfxlib/haiku/sfx_midi_bsd.c \
    $(srcdir)/sfxlib/haiku/sfx_unix.c, \
    $(SFX_SRC_TARGET))
endif

SFX_SRC := $(SFX_SRC_TARGET) $(SFX_SRC_UNIX) $(SFX_SRC_GENERIC) \
$(SFX_SRC_ARCH_C)

ifdef DISABLE_ALSA
SFX_SRC := $(filter-out \
$(srcdir)/sfxlib/linux/sfx_capture_alsa.c \
$(srcdir)/sfxlib/linux/sfx_driver_alsa.c \
$(srcdir)/sfxlib/linux/sfx_midi_alsa.c, \
$(SFX_SRC))
ifeq ($(TARGET_OS),linux)
SFX_SRC += $(srcdir)/sfxlib/sfx_midi_driver_stub.c
endif
endif

ifdef DISABLE_PULSE
SFX_SRC := $(filter-out \
$(srcdir)/sfxlib/linux/sfx_driver_pulse.c, \
$(SFX_SRC))
endif

# C and C++ selections must also have distinct archive member names. Their
# different source suffixes still map to the same .o basename.
SFX_ARCHIVE_MEMBERS := $(addsuffix .o,$(basename $(notdir $(SFX_SRC))))
SFX_DUPLICATE_BASENAMES := $(strip $(foreach member,\
$(sort $(SFX_ARCHIVE_MEMBERS)),\
$(if $(word 2,$(filter $(member),$(SFX_ARCHIVE_MEMBERS))),$(member))))
ifneq ($(SFX_DUPLICATE_BASENAMES),)
$(error Duplicate sfxlib source basenames: $(SFX_DUPLICATE_BASENAMES))
endif

##############################################################################
# Canonical object lists
##############################################################################

RTLIB_OBJ := $(patsubst $(srcdir)/rtlib/%.c,$(libfbobjdir)/%.o,$(RTLIB_SRC))
RTLIB_OBJ := $(patsubst $(srcdir)/rtlib/%.s,$(libfbobjdir)/%.o,$(RTLIB_OBJ))
FBRT_OBJ := $(patsubst $(srcdir)/fbrt/%.bas,$(libfbrtobjdir)/%.o,$(FBRT_SRC))
GFX_SRC_C := $(filter %.c,$(GFX_SRC))
GFX_SRC_CPP := $(filter %.cpp,$(GFX_SRC))
GFX_SRC_ASM := $(filter %.s,$(GFX_SRC))
GFX_OBJ := \
$(patsubst $(srcdir)/gfxlib2/%.c,$(libfbgfxobjdir)/%.o,$(GFX_SRC_C)) \
$(patsubst $(srcdir)/gfxlib2/%.cpp,$(libfbgfxobjdir)/%.o,$(GFX_SRC_CPP)) \
$(patsubst $(srcdir)/gfxlib2/%.s,$(libfbgfxobjdir)/%.o,$(GFX_SRC_ASM))
GFX3_OBJ := $(patsubst $(srcdir)/gfxlib3/%.c,$(libfbgfx3objdir)/%.o,$(GFX3_SRC))
SFX_SRC_C := $(filter %.c,$(SFX_SRC))
SFX_SRC_CPP := $(filter %.cpp,$(SFX_SRC))
SFX_OBJ := \
$(patsubst $(srcdir)/sfxlib/%.c,$(libsfxobjdir)/%.o,$(SFX_SRC_C)) \
$(patsubst $(srcdir)/sfxlib/%.cpp,$(libsfxobjdir)/%.o,$(SFX_SRC_CPP))

##############################################################################
# Derived object variants
##############################################################################

RTLIB_PIC_OBJ := $(RTLIB_OBJ:$(libfbobjdir)/%.o=$(libfbpicobjdir)/%.o)
RTLIB_MT_OBJ := $(RTLIB_OBJ:$(libfbobjdir)/%.o=$(libfbmtobjdir)/%.o)
RTLIB_MT_PIC_OBJ := $(RTLIB_OBJ:$(libfbobjdir)/%.o=$(libfbmtpicobjdir)/%.o)
FBRT_PIC_OBJ := $(FBRT_OBJ:$(libfbrtobjdir)/%.o=$(libfbrtpicobjdir)/%.o)
FBRT_MT_OBJ := $(FBRT_OBJ:$(libfbrtobjdir)/%.o=$(libfbrtmtobjdir)/%.o)
FBRT_MT_PIC_OBJ := $(FBRT_OBJ:$(libfbrtobjdir)/%.o=$(libfbrtmtpicobjdir)/%.o)
GFX_PIC_OBJ := $(GFX_OBJ:$(libfbgfxobjdir)/%.o=$(libfbgfxpicobjdir)/%.o)
GFX_MT_OBJ := $(GFX_OBJ:$(libfbgfxobjdir)/%.o=$(libfbgfxmtobjdir)/%.o)
GFX_MT_PIC_OBJ := $(GFX_OBJ:$(libfbgfxobjdir)/%.o=$(libfbgfxmtpicobjdir)/%.o)
GFX3_PIC_OBJ := $(GFX3_OBJ:$(libfbgfx3objdir)/%.o=$(libfbgfx3picobjdir)/%.o)
GFX3_MT_OBJ := $(GFX3_OBJ:$(libfbgfx3objdir)/%.o=$(libfbgfx3mtobjdir)/%.o)
GFX3_MT_PIC_OBJ := $(GFX3_OBJ:$(libfbgfx3objdir)/%.o=$(libfbgfx3mtpicobjdir)/%.o)
SFX_PIC_OBJ := $(SFX_OBJ:$(libsfxobjdir)/%.o=$(libsfxpicobjdir)/%.o)
SFX_MT_OBJ := $(SFX_OBJ:$(libsfxobjdir)/%.o=$(libsfxmtobjdir)/%.o)
SFX_MT_PIC_OBJ := $(SFX_OBJ:$(libsfxobjdir)/%.o=$(libsfxmtpicobjdir)/%.o)

##############################################################################
# Aggregate runtime object set (for dependency rules)
##############################################################################

ALL_RUNTIME_OBJS := \
$(RTLIB_OBJ) $(RTLIB_PIC_OBJ) $(RTLIB_MT_OBJ) $(RTLIB_MT_PIC_OBJ) \
$(FBRT_OBJ) $(FBRT_PIC_OBJ) $(FBRT_MT_OBJ) $(FBRT_MT_PIC_OBJ) \
$(GFX_OBJ) $(GFX_PIC_OBJ) $(GFX_MT_OBJ) $(GFX_MT_PIC_OBJ) \
$(GFX3_OBJ) $(GFX3_PIC_OBJ) $(GFX3_MT_OBJ) $(GFX3_MT_PIC_OBJ) \
$(SFX_OBJ) $(SFX_PIC_OBJ) $(SFX_MT_OBJ) $(SFX_MT_PIC_OBJ)

##############################################################################
# Headers
##############################################################################

LIBFB_H := $(wildcard $(srcdir)/rtlib/*.h) $(wildcard $(srcdir)/rtlib/*/*.h) \
$(wildcard $(RTLIB_DRIVER_DIR)/*/*.h) $(wildcard $(RTLIB_DRIVER_DIR)/*/*/*.h) \
$(foreach directory,$(RTLIB_GENERIC_DIRS),$(wildcard $(directory)/*.h))
LIBFBRT_BI := $(wildcard $(srcdir)/fbrt/*.bi)
LIBFBGFX_H := $(wildcard $(srcdir)/gfxlib2/*.h) $(wildcard $(srcdir)/gfxlib2/*/*.h) \
$(wildcard $(GFX2_DRIVER_DIR)/*/*.h) $(wildcard $(GFX2_DRIVER_DIR)/*/*/*.h) \
$(wildcard $(GFX2_DRIVER_DIR)/*/*.inc) \
$(wildcard $(srcdir)/gfxlib2/*.inc)
LIBFBGFX3_H := $(wildcard $(srcdir)/gfxlib3/*.h) $(wildcard $(srcdir)/gfxlib3/*/*.h) \
$(wildcard $(GFX3_DRIVER_DIR)/*/*.h) \
$(srcdir)/gfxlib2/gfx_font.h $(srcdir)/gfxlib2/gfx_paint_pattern.h
LIBSFX_H := $(wildcard $(srcdir)/sfxlib/*.h) $(wildcard $(srcdir)/sfxlib/*/*.h)

##############################################################################
# END source-graph.mk
##############################################################################
