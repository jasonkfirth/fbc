##############################################################################
# layout.mk
##############################################################################

libdirname := lib

ifdef ENABLE_LIB64
  ifneq ($(filter x86_64 aarch64 arm64 ppc64 ppc64le s390x,$(ISA_FAMILY)),)
    libdirname := lib64
  endif
endif

ifeq ($(TARGET_OS),win32)
libdirname := lib
endif

ifeq ($(TARGET_OS),cygwin)
libdirname := lib
endif

ifeq ($(TARGET_OS),dos)
libdirname := lib
endif

##############################################################################
# INSTALL PREFIX POLICY
##############################################################################

prefix ?= /usr/local

ifeq ($(TARGET_OS),haiku)
prefix := /
endif

ifeq ($(TARGET_OS),netbsd)
prefix := /usr/pkg
endif

ifeq ($(TARGET_OS),win32)
prefix := C:/FreeBASIC
endif

ifeq ($(TARGET_OS),dos)
prefix := /fb
endif

##############################################################################
# INSTALL DIRECTORY LAYOUT (generic defaults)
##############################################################################

prefixbindir := $(prefix)/bin
prefixincdir := $(prefix)/include/$(FBNAME)
prefixruntimedir := $(prefix)/$(libdirname)/freebasic

##############################################################################
# HAIKU LAYOUT OVERRIDES
##############################################################################

ifeq ($(TARGET_OS),haiku)

# binaries
prefixbindir := $(prefix)/bin

# headers belong in the Haiku development tree
prefixincdir := $(prefix)/develop/headers/freebasic

# runtime libraries must be in lib/freebasic
prefixruntimedir := $(prefix)/lib/freebasic

endif

##############################################################################
# NETBSD LAYOUT
##############################################################################

ifeq ($(TARGET_OS),netbsd)

prefixbindir := $(prefix)/bin
prefixincdir := $(prefix)/include/freebasic
prefixruntimedir := $(prefix)/lib/freebasic

endif

##############################################################################
# WINDOWS LAYOUT
##############################################################################

ifeq ($(TARGET_OS),win32)
prefixbindir := $(prefix)
prefixincdir := $(prefix)/inc
prefixruntimedir := $(prefix)/lib
endif

##############################################################################
# DOS LAYOUT
##############################################################################

ifeq ($(TARGET_OS),dos)
prefixbindir := $(prefix)
prefixincdir := $(prefix)/inc
prefixruntimedir := $(prefix)/lib
endif

##############################################################################
# DERIVED PATHS
##############################################################################

FBINSTALL_RUNTIME_DIR := $(prefixruntimedir)/$(FBTARGET)
prefixlibdir := $(FBINSTALL_RUNTIME_DIR)

##############################################################################
# INCLUDE PATHS
##############################################################################

SRC_INCDIR := $(rootdir)/inc
BOOTSTRAP_INCDIR := $(rootdir)/bootstrap/inc
FBC_INCDIR := $(prefixincdir)

FBC_BOOTSTRAP_I := -i $(BOOTSTRAP_INCDIR)
FBC_INCLUDE_FLAGS := -i $(SRC_INCDIR)

##############################################################################
# end of layout.mk
##############################################################################
