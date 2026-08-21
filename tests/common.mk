# common.mk
# This file is part of the FreeBASIC test suite
#
# Shared test-suite platform and tool defaults.
#
# HOST takes possible values dos|unix|win32.
# TARGET_OS takes possible values dos|unix|win32|js.
#
# The tests can be run directly from tests/ or through the root build.  Reuse
# the root platform/toolchain logic when it is available so BSD hosts, MinGW
# shells, and cross-target runs see the same compiler tool names as the main
# build.
#

testsdir := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
rootdir ?= $(abspath $(testsdir)/..)
mkpath ?= $(rootdir)/mk

TESTS_INPUT_TARGET := $(TARGET)
PRESERVE_FBC := 1

ifneq ($(wildcard $(mkpath)/platform.mk),)
include $(mkpath)/platform.mk
include $(mkpath)/cpu.mk
include $(mkpath)/platform-features.mk
include $(mkpath)/compiler-config.mk
endif

ifneq ($(TESTS_INPUT_TARGET),)
override TARGET := $(TESTS_INPUT_TARGET)
endif

.DEFAULT_GOAL := all

TESTS_HOST_OS := $(HOST_OS)
TESTS_TARGET_OS := $(TARGET_OS)

HOST :=
ifeq ($(TESTS_HOST_OS),dos)
	HOST := dos
else ifneq ($(filter win32 cygwin,$(TESTS_HOST_OS)),)
	HOST := win32
else ifneq ($(TESTS_HOST_OS),)
	HOST := unix
else ifeq ($(OS),DOS)
	HOST := dos
else ifeq ($(OS),Windows_NT)
	HOST := win32
else ifdef WINDIR
	HOST := win32
else ifdef windir
	HOST := win32
else ifdef HOME
	HOST := unix
endif

ifndef HOST
CHECKHOST_MSG := $(error error: HOST couldn't be guessed)
else
CHECKHOST_MSG :=
endif

ifeq ($(TESTS_TARGET_OS),dos)
	TARGET_OS := dos
else ifeq ($(TESTS_TARGET_OS),js)
	TARGET_OS := js
else ifeq ($(TESTS_TARGET_OS),wince)
	TARGET_OS := wince
else ifneq ($(filter win32 cygwin xbox,$(TESTS_TARGET_OS)),)
	TARGET_OS := win32
else ifneq ($(TESTS_TARGET_OS),)
	TARGET_OS := unix
else
	TARGET_OS := $(HOST)
endif

# set default command names
#

ifeq ($(HOST),unix)
	EXEEXT :=
else
	EXEEXT := .exe
endif
ifeq ($(TARGET_OS),unix)
	TARGET_EXEEXT :=
else ifeq ($(TARGET_OS),wince)
	TARGET_EXEEXT := .exe
else ifeq ($(TARGET_OS),js)
	ifeq ($(NODEJS),)
		TARGET_EXEEXT := .html
	else
		TARGET_EXEEXT := .js
	endif
else
	TARGET_EXEEXT := .exe
endif

TESTS_TOOLCHAIN_BINDIR :=
ifneq ($(findstring /,$(CC))$(findstring \,$(CC)),)
TESTS_TOOLCHAIN_BINDIR := $(patsubst %/,%,$(dir $(CC)))
endif

TESTS_FBC_ENV_EXTRA :=
ifeq ($(TESTS_HOST_OS),darwin)
TESTS_DARWIN_SDKROOT := $(strip $(shell xcrun --sdk macosx --show-sdk-path 2>/dev/null || xcrun --show-sdk-path 2>/dev/null))
TESTS_DARWIN_DEPLOYMENT_TARGET ?= $(MACOSX_DEPLOYMENT_TARGET)
ifneq ($(strip $(TESTS_DARWIN_DEPLOYMENT_TARGET)),)
TESTS_FBC_ENV_EXTRA += MACOSX_DEPLOYMENT_TARGET='$(TESTS_DARWIN_DEPLOYMENT_TARGET)'
endif
ifneq ($(strip $(TESTS_DARWIN_SDKROOT)),)
TESTS_FBC_ENV_EXTRA += SDKROOT='$(TESTS_DARWIN_SDKROOT)'
endif
endif

ifeq ($(HOST),dos)
TESTS_FBC_ENV :=
else
TESTS_FBC_ENV := env \
	$(TESTS_FBC_ENV_EXTRA) \
	PATH="$(if $(strip $(TESTS_TOOLCHAIN_BINDIR)),$(TESTS_TOOLCHAIN_BINDIR):)$$PATH" \
	AS="$(AS)" \
	AR="$(AR)" \
	LD="$(LD)" \
	GCC="$(CC)" \
	CLANG="$(CLANG)" \
	LLC="$(LLC)" \
	DLLTOOL="$(DLLTOOL)" \
	WINDRES="$(WINDRES)" \
	GORC="$(GORC)" \
	EMAS="$(EMAS)" \
	EMAR="$(EMAR)" \
	EMLD="$(EMLD)" \
	EMCC="$(EMCC)" \
	CXBE="$(CXBE)" \
	DXEGEN="$(DXEGEN)"
endif

TESTS_FBC_TARGET_ARGS := $(if $(strip $(BUILD_FBC_TARGET)),-target $(BUILD_FBC_TARGET))
TESTS_FBC_BUILDPREFIX_ARGS := $(if $(strip $(BUILD_FBC_BUILDPREFIX)),-buildprefix $(BUILD_FBC_BUILDPREFIX))
TESTS_DEFAULT_FBC := $(TESTS_FBC_ENV) fbc$(EXEEXT) $(TESTS_FBC_TARGET_ARGS) $(TESTS_FBC_BUILDPREFIX_ARGS)
