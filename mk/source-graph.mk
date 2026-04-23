##############################################################################
# source-graph.mk
#
# Source discovery with override precedence
#
# precedence:
#   target_os > unix > generic
#
# The shared unix layer only applies to Unix-family targets such as Linux,
# BSD, Solaris, Haiku, and Android. Cygwin uses the Win32 target-specific
# source trees because the runtime/compiler configuration identifies it as a
# Win32 host with Cygwin compatibility shims, not as a HOST_UNIX target.
#
# Object architecture:
#   canonical object list  derived PIC/MT variants
##############################################################################

##############################################################################
# Search directories
##############################################################################

UNIX_LAYER_OS := linux android darwin freebsd netbsd openbsd dragonfly solaris haiku

SOURCE_OS := $(TARGET_OS)
ifeq ($(TARGET_OS),cygwin)
SOURCE_OS := win32
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

RTLIB_DIRS := $(srcdir)/rtlib
ifneq ($(USE_UNIX_LAYER),)
RTLIB_DIRS += $(srcdir)/rtlib/unix
endif
RTLIB_DIRS += $(srcdir)/rtlib/$(SOURCE_OS)
ifeq ($(TARGET_ARCH),x86)
RTLIB_DIRS += $(srcdir)/rtlib/x86
endif

FBRT_DIRS := \
$(srcdir)/fbrt

GFXLIB2_DIRS := $(srcdir)/gfxlib2
ifneq ($(USE_UNIX_LAYER),)
GFXLIB2_DIRS += $(srcdir)/gfxlib2/unix
endif
GFXLIB2_DIRS += $(srcdir)/gfxlib2/$(SOURCE_OS)
ifeq ($(TARGET_ARCH),x86)
GFXLIB2_DIRS += $(srcdir)/gfxlib2/x86
endif

SFXLIB_DIRS := $(srcdir)/sfxlib
ifneq ($(USE_UNIX_LAYER),)
SFXLIB_DIRS += $(srcdir)/sfxlib/unix
endif
SFXLIB_DIRS += $(srcdir)/sfxlib/$(SOURCE_OS)

##############################################################################
# Compiler sources
##############################################################################

FBC_BI := $(wildcard $(srcdir)/compiler/*.bi)
FBC_SRC := $(wildcard $(srcdir)/compiler/*.bas)
FBC_OBJS := $(patsubst $(srcdir)/compiler/%.bas,$(fbcobjdir)/%.o,$(FBC_SRC))

##############################################################################
# RTLIB sources
##############################################################################

RTLIB_SRC_GENERIC := $(wildcard $(srcdir)/rtlib/*.c)
RTLIB_SRC_UNIX :=
ifneq ($(USE_UNIX_LAYER),)
RTLIB_SRC_UNIX := $(wildcard $(srcdir)/rtlib/unix/*.c)
endif
RTLIB_SRC_TARGET := $(wildcard $(srcdir)/rtlib/$(SOURCE_OS)/*.c)
RTLIB_SRC_ARCH :=
ifeq ($(TARGET_ARCH),x86)
RTLIB_SRC_ARCH := $(wildcard $(srcdir)/rtlib/x86/*.s)
endif
RTLIB_SRC_TARGET_ASM := $(wildcard $(srcdir)/rtlib/$(SOURCE_OS)/*.s)

RTLIB_BASE_GENERIC := $(notdir $(RTLIB_SRC_GENERIC))
RTLIB_BASE_UNIX := $(notdir $(RTLIB_SRC_UNIX))
RTLIB_BASE_TARGET := $(notdir $(RTLIB_SRC_TARGET))

RTLIB_SRC_UNIX := $(filter-out \
$(addprefix $(srcdir)/rtlib/unix/,$(RTLIB_BASE_TARGET)), \
$(RTLIB_SRC_UNIX))

RTLIB_SRC_GENERIC := $(filter-out \
$(addprefix $(srcdir)/rtlib/,$(RTLIB_BASE_UNIX)) \
$(addprefix $(srcdir)/rtlib/,$(RTLIB_BASE_TARGET)), \
$(RTLIB_SRC_GENERIC))

RTLIB_SRC := $(RTLIB_SRC_GENERIC) $(RTLIB_SRC_UNIX) $(RTLIB_SRC_TARGET) $(RTLIB_SRC_ARCH) $(RTLIB_SRC_TARGET_ASM)

ifdef DISABLE_X11
RTLIB_SRC := $(filter-out \
$(srcdir)/rtlib/unix/io_xfocus.c \
$(srcdir)/rtlib/unix/scancodes_x11.c, \
$(RTLIB_SRC))
endif

##############################################################################
# FBRT sources
##############################################################################

FBRT_SRC := $(wildcard $(srcdir)/fbrt/*.bas)

##############################################################################
# GFXLIB2 sources
##############################################################################

GFX_SRC_GENERIC := \
$(wildcard $(srcdir)/gfxlib2/*.c)

ifneq ($(USE_RUNTIME_CXX),)
GFX_SRC_GENERIC += $(wildcard $(srcdir)/gfxlib2/*.cpp)
endif

GFX_SRC_UNIX :=
ifneq ($(USE_UNIX_LAYER),)
 GFX_SRC_UNIX := \
$(wildcard $(srcdir)/gfxlib2/unix/*.c)
 ifneq ($(USE_RUNTIME_CXX),)
GFX_SRC_UNIX += $(wildcard $(srcdir)/gfxlib2/unix/*.cpp)
 endif
endif

GFX_SRC_TARGET := \
$(wildcard $(srcdir)/gfxlib2/$(SOURCE_OS)/*.c)

ifneq ($(USE_RUNTIME_CXX),)
GFX_SRC_TARGET += $(wildcard $(srcdir)/gfxlib2/$(SOURCE_OS)/*.cpp)
endif

GFX_SRC_ARCH :=
ifeq ($(TARGET_ARCH),x86)
GFX_SRC_ARCH := $(wildcard $(srcdir)/gfxlib2/x86/*.s)
endif

GFX_SRC_TARGET_ASM := $(wildcard $(srcdir)/gfxlib2/$(SOURCE_OS)/*.s)

GFX_BASE_GENERIC := $(notdir $(GFX_SRC_GENERIC))
GFX_BASE_UNIX := $(notdir $(GFX_SRC_UNIX))
GFX_BASE_TARGET := $(notdir $(GFX_SRC_TARGET))

GFX_SRC_UNIX := $(filter-out \
$(addprefix $(srcdir)/gfxlib2/unix/,$(GFX_BASE_TARGET)), \
$(GFX_SRC_UNIX))

GFX_SRC_GENERIC := $(filter-out \
$(addprefix $(srcdir)/gfxlib2/,$(GFX_BASE_UNIX)) \
$(addprefix $(srcdir)/gfxlib2/,$(GFX_BASE_TARGET)), \
$(GFX_SRC_GENERIC))

GFX_SRC := $(GFX_SRC_GENERIC) $(GFX_SRC_UNIX) $(GFX_SRC_TARGET) $(GFX_SRC_ARCH) $(GFX_SRC_TARGET_ASM)

ifdef DISABLE_X11
GFX_SRC := $(filter-out \
$(srcdir)/gfxlib2/unix/gfx_driver_opengl_x11.c \
$(srcdir)/gfxlib2/unix/gfx_driver_x11.c \
$(srcdir)/gfxlib2/unix/gfx_x11.c \
$(srcdir)/gfxlib2/unix/gfx_x11_icon_stub.c, \
$(GFX_SRC))
endif

ifdef DISABLE_OPENGL
GFX_SRC := $(filter-out \
$(srcdir)/gfxlib2/darwin/gfx_driver_opengl.c, \
$(GFX_SRC))
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

SFX_BASE_GENERIC := $(notdir $(SFX_SRC_GENERIC))
SFX_BASE_UNIX := $(notdir $(SFX_SRC_UNIX))
SFX_BASE_TARGET := $(notdir $(SFX_SRC_TARGET))

SFX_SRC_UNIX := $(filter-out \
$(addprefix $(srcdir)/sfxlib/unix/,$(SFX_BASE_TARGET)), \
$(SFX_SRC_UNIX))

SFX_SRC_GENERIC := $(filter-out \
$(addprefix $(srcdir)/sfxlib/,$(SFX_BASE_UNIX)) \
$(addprefix $(srcdir)/sfxlib/,$(SFX_BASE_TARGET)), \
$(SFX_SRC_GENERIC))

SFX_SRC := $(SFX_SRC_GENERIC) $(SFX_SRC_UNIX) $(SFX_SRC_TARGET)

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
$(SFX_OBJ) $(SFX_PIC_OBJ) $(SFX_MT_OBJ) $(SFX_MT_PIC_OBJ)

##############################################################################
# Headers
##############################################################################

LIBFB_H := $(wildcard $(srcdir)/rtlib/*.h) $(wildcard $(srcdir)/rtlib/*/*.h)
LIBFBRT_BI := $(wildcard $(srcdir)/fbrt/*.bi)
LIBFBGFX_H := $(wildcard $(srcdir)/gfxlib2/*.h) $(wildcard $(srcdir)/gfxlib2/*/*.h)
LIBSFX_H := $(wildcard $(srcdir)/sfxlib/*.h) $(wildcard $(srcdir)/sfxlib/*/*.h)

##############################################################################
# END source-graph.mk
##############################################################################
