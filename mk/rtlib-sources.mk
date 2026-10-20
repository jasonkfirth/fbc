##############################################################################
# Project: FreeBASIC runtime library
# File: rtlib-sources.mk
# Purpose: Own runtime subsystem discovery and platform replacement precedence.
# Responsibilities: Select sources once and reject ambiguous archive members.
# This file intentionally does NOT contain flags, object rules or runtime code.
##############################################################################

# Portable implementations are grouped by responsibility. Keep discovery
# explicit so a backup, generated tree or new directory cannot enter a build.
RTLIB_SOURCE_GROUPS := \
core \
arrays \
console \
hooks \
data \
devices \
errors \
files \
datetime \
math \
memory \
objects \
profiling \
strings \
strings/wide \
strings/unicode \
strings/encoding \
threads \
services \
system \
legacy

RTLIB_GENERIC_DIRS := $(addprefix $(srcdir)/rtlib/,$(RTLIB_SOURCE_GROUPS))
RTLIB_DRIVER_DIR := $(srcdir)/rtlib/drivers
RTLIB_DIRS := $(srcdir)/rtlib $(RTLIB_GENERIC_DIRS)
ifneq ($(USE_UNIX_LAYER),)
RTLIB_DIRS += $(RTLIB_DRIVER_DIR)/unix
endif
RTLIB_DIRS += $(RTLIB_DRIVER_DIR)/$(SOURCE_OS)
RTLIB_DIRS += $(RTLIB_DRIVER_DIR)/$(SOURCE_OS)/$(TARGET_ARCH)
ifeq ($(TARGET_ARCH),x86)
RTLIB_DIRS += $(RTLIB_DRIVER_DIR)/x86
endif

RTLIB_GENERIC_ALL := $(foreach directory,$(RTLIB_GENERIC_DIRS),$(wildcard $(directory)/*.c))

# ar stores a basename even though the object directories preserve source
# paths. Duplicate names within a layer are ambiguous, not platform overrides.
define rtlib_check_names
ifneq ($$(words $$(notdir $$(basename $(1)))),$$(words $$(sort $$(notdir $$(basename $(1))))))
$$(error Duplicate rtlib source basename in $(2))
endif
endef
$(eval $(call rtlib_check_names,$(RTLIB_GENERIC_ALL),portable subsystems))

# Sort by basename, independent of the subsystem path. Make's sort is stable
# across host locales; native wildcard collation is not. Archive member names
# and the platform replacement policy retain their established identities.
RTLIB_SRC_GENERIC := $(foreach name,$(sort $(notdir $(RTLIB_GENERIC_ALL))),$(filter %/$(name),$(RTLIB_GENERIC_ALL)))
RTLIB_SRC_UNIX :=
ifneq ($(USE_UNIX_LAYER),)
RTLIB_SRC_UNIX := $(wildcard $(RTLIB_DRIVER_DIR)/unix/*.c)
endif
RTLIB_SRC_TARGET := $(wildcard $(RTLIB_DRIVER_DIR)/$(SOURCE_OS)/*.c)
RTLIB_SRC_TARGET_ARCH := $(wildcard $(RTLIB_DRIVER_DIR)/$(SOURCE_OS)/$(TARGET_ARCH)/*.c)
RTLIB_SRC_ARCH :=
ifeq ($(TARGET_ARCH),x86)
RTLIB_SRC_ARCH := $(wildcard $(RTLIB_DRIVER_DIR)/x86/*.s)
endif
RTLIB_SRC_TARGET_ASM := $(wildcard $(RTLIB_DRIVER_DIR)/$(SOURCE_OS)/*.s)
RTLIB_SRC_TARGET_ARCH_ASM := $(wildcard $(RTLIB_DRIVER_DIR)/$(SOURCE_OS)/$(TARGET_ARCH)/*.s)

$(eval $(call rtlib_check_names,$(RTLIB_SRC_UNIX),shared Unix layer))
$(eval $(call rtlib_check_names,$(RTLIB_SRC_TARGET) $(RTLIB_SRC_TARGET_ASM),target layer))
$(eval $(call rtlib_check_names,$(RTLIB_SRC_TARGET_ARCH) $(RTLIB_SRC_TARGET_ARCH_ASM),target architecture layer))

RTLIB_BASE_UNIX := $(notdir $(RTLIB_SRC_UNIX))
RTLIB_BASE_TARGET := $(notdir $(RTLIB_SRC_TARGET))
RTLIB_BASE_TARGET_ARCH := $(notdir $(RTLIB_SRC_TARGET_ARCH))

# Higher layers replace whole C translation units by filename. Filtering by
# basename, rather than a generic directory prefix, keeps the same policy when
# an implementation moves to its owning subsystem.
rtlib_without_names = $(foreach source,$(1),$(if $(filter $(notdir $(source)),$(2)),,$(source)))
RTLIB_SRC_TARGET := $(call rtlib_without_names,$(RTLIB_SRC_TARGET),$(RTLIB_BASE_TARGET_ARCH))
RTLIB_SRC_UNIX := $(call rtlib_without_names,$(RTLIB_SRC_UNIX),$(RTLIB_BASE_TARGET) $(RTLIB_BASE_TARGET_ARCH))
RTLIB_SRC_GENERIC := $(call rtlib_without_names,$(RTLIB_SRC_GENERIC),$(RTLIB_BASE_UNIX) $(RTLIB_BASE_TARGET) $(RTLIB_BASE_TARGET_ARCH))

RTLIB_SRC := $(RTLIB_SRC_GENERIC) $(RTLIB_SRC_UNIX) $(RTLIB_SRC_TARGET) $(RTLIB_SRC_TARGET_ARCH) $(RTLIB_SRC_ARCH) $(RTLIB_SRC_TARGET_ASM) $(RTLIB_SRC_TARGET_ARCH_ASM)

ifdef DISABLE_X11
RTLIB_SRC := $(filter-out \
$(RTLIB_DRIVER_DIR)/unix/io_xfocus.c \
$(RTLIB_DRIVER_DIR)/unix/scancodes_x11.c, \
$(RTLIB_SRC))
endif
$(eval $(call rtlib_check_names,$(RTLIB_SRC),selected runtime))

##############################################################################
# end of rtlib-sources.mk
##############################################################################
