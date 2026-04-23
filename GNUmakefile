############
# Makefile #
############

#!/usr/bin/make -f

.DEFAULT_GOAL := all

##############################################################################
# Root discovery
##############################################################################

rootdir := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
srcdir  := $(rootdir)/src
mkpath  := $(rootdir)/mk

##############################################################################
# Recursive make hardening
##############################################################################

MAKEFLAGS += --no-print-directory

##############################################################################
# Toolchain defaults
##############################################################################

FBC    ?= fbc
prefix ?= /usr/local

CFLAGS  ?= -Wfatal-errors -O2
CFLAGS  += -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables
FBFLAGS ?= -maxerr 1

##############################################################################
# Version
##############################################################################

include $(mkpath)/version.mk

##############################################################################
# Platform identity
##############################################################################

include $(mkpath)/platform.mk

ifdef TARGET
  ifeq ($(strip $(TARGET_TRIPLET)),)
    TARGET_TRIPLET := $(TARGET)
  endif
  override TARGET :=
endif

##############################################################################
# CPU normalization
##############################################################################

include $(mkpath)/cpu.mk

##############################################################################
# Platform policy
##############################################################################

include $(mkpath)/platform-features.mk
include $(mkpath)/feature-policy.mk

##############################################################################
# Toolchain realization
##############################################################################

include $(mkpath)/toolchain-flags.mk
include $(mkpath)/host-tools.mk
include $(mkpath)/os-flags.mk
include $(mkpath)/compiler-config.mk

##############################################################################
# Naming + layout
##############################################################################

include $(mkpath)/naming.mk
include $(mkpath)/layout.mk
include $(mkpath)/build-layout.mk
include $(mkpath)/multilib.mk

##############################################################################
# Optional prerequisites module
##############################################################################

-include $(mkpath)/prereqs.mk

##############################################################################
# Source graph
##############################################################################

include $(mkpath)/source-graph.mk

##############################################################################
# Artifact graph
##############################################################################

include $(mkpath)/archives.mk

##############################################################################
# Build mechanics
##############################################################################

include $(mkpath)/build/build-dirs.mk
include $(mkpath)/build/compile-rules.mk
include $(mkpath)/build/dependency-rules.mk
include $(mkpath)/build/archive-rules.mk
include $(mkpath)/build/build-targets.mk
include $(mkpath)/build/clean-rules.mk

##############################################################################
# Bootstrap system
##############################################################################

include $(mkpath)/bootstrap/bootstrap-core.mk
include $(mkpath)/bootstrap/bootstrap-emit.mk
include $(mkpath)/bootstrap/bootstrap-dist.mk

##############################################################################
# Distribution / installation / tests
##############################################################################

include $(mkpath)/dist.mk
include $(mkpath)/tests.mk
include $(mkpath)/maketests.mk
include $(mkpath)/inst_uninst.mk

##############################################################################
# Prerequisite gating
##############################################################################

.PHONY: maybe-prereqs

ifdef HAVE_PREREQS_MK
maybe-prereqs: prereqs
else
maybe-prereqs:
endif

.PHONY: maybe-build-fbc

ifdef HAVE_PREREQS_MK
maybe-build-fbc: prereqs-fbc
else
maybe-build-fbc:
endif

##############################################################################
# Targets that require prereqs
##############################################################################

libs: | maybe-prereqs
compiler-stage: | maybe-prereqs
compiler: | maybe-prereqs
rtlib: | maybe-prereqs
fbrt: | maybe-prereqs
gfxlib2: | maybe-prereqs
sfxlib: | maybe-prereqs

bootstrap: | maybe-prereqs
bootstrap-minimal: | maybe-prereqs
bootstrap-emit: | maybe-prereqs
bootstrap-emit-matrix: | maybe-prereqs
bootstrap-dist-target: | maybe-prereqs
bootstrap-dist: | maybe-prereqs
bootstrap-dist-arm: | maybe-prereqs
bootstrap-dist-all: | maybe-prereqs

dist: | maybe-prereqs
dist-zip: | maybe-prereqs
pkg-tar: | maybe-prereqs
pkg-zip: | maybe-prereqs

install: | maybe-prereqs
uninstall: | maybe-prereqs

quick-test: | maybe-prereqs
full-test: | maybe-prereqs
bootstrap-test: | maybe-prereqs
bootstrap-emit-test: | maybe-prereqs
bootstrap-emit-matrix-test: | maybe-prereqs
bootstrap-dist-test: | maybe-prereqs
bootstrap-dist-matrix-test: | maybe-prereqs
bootstrap-rebuild-test: | maybe-prereqs
bootstrap-stage-test: | maybe-prereqs
packaging-test: | maybe-prereqs
pkg-test: | maybe-prereqs
install-test: | maybe-prereqs
uninstall-test: | maybe-prereqs
matrix-test: | maybe-prereqs
tests-test: | maybe-prereqs

compiler-stage: | maybe-build-fbc
compiler: | maybe-build-fbc
fbrt: | maybe-build-fbc
bootstrap-emit: | maybe-build-fbc
bootstrap-emit-matrix: | maybe-build-fbc
unit-tests: | maybe-build-fbc
log-tests: | maybe-build-fbc
warning-tests: | maybe-build-fbc
tests-test: | maybe-build-fbc
compiler-smoke: | maybe-build-fbc

##############################################################################
# Build ordering
##############################################################################

libs: rtlib fbrt gfxlib2 sfxlib
compiler-stage: rtlib compiler

##############################################################################
# Default target
##############################################################################

all: libs compiler-stage

##############################################################################
# Debug configuration
##############################################################################

.PHONY: print-config

print-config:
	@echo "rootdir=$(rootdir)"
	@echo "srcdir=$(srcdir)"
	@echo "mkpath=$(mkpath)"
	@echo "TARGET_TRIPLET=$(TARGET_TRIPLET)"
	@echo "TARGET_OS=$(TARGET_OS)"
	@echo "TARGET_ARCH=$(TARGET_ARCH)"
	@echo "ISA_FAMILY=$(ISA_FAMILY)"
	@echo "ARM_VER=$(ARM_VER)"
	@echo "ARM_FLOAT_ABI=$(ARM_FLOAT_ABI)"
	@echo "DEFAULT_CPUTYPE_ARM=$(DEFAULT_CPUTYPE_ARM)"
	@echo "FBC_TARGET=$(FBC_TARGET)"
	@echo "FBTARGET=$(FBTARGET)"
	@echo "FBPACK_DIR=$(FBPACK_DIR)"
	@echo "FBINSTALL_RUNTIME_DIR=$(FBINSTALL_RUNTIME_DIR)"
	@echo "ENABLE_PIC=$(ENABLE_PIC)"
	@echo "ENABLE_NONPIC=$(ENABLE_NONPIC)"
	@echo "DISABLE_MT=$(DISABLE_MT)"
	@echo "THREAD_MODEL=$(THREAD_MODEL)"
	@echo "USE_RUNTIME_CXX=$(USE_RUNTIME_CXX)"
	@echo "ALLCFLAGS=$(ALLCFLAGS)"
	@echo "ALLCXXFLAGS=$(ALLCXXFLAGS)"
	@echo "ALLFBCFLAGS=$(ALLFBCFLAGS)"
	@echo "ALLFBRTCFLAGS=$(ALLFBRTCFLAGS)"
	@echo "ALLLDFLAGS=$(ALLLDFLAGS)"
	@echo "FBC_EXE=$(FBC_EXE)"
	@echo "FBCNEW_EXE=$(FBCNEW_EXE)"
	@echo "libdir=$(libdir)"
	@echo "prefixlibdir=$(prefixlibdir)"

##############################################################################
# Help
##############################################################################

.PHONY: help

help:
	@echo "FreeBASIC Build System"
	@echo ""
	@echo "Primary targets:"
	@echo "  all                     Build compiler and libraries"
	@echo "  compiler                Build FreeBASIC compiler"
	@echo "  rtlib                   Build runtime library"
	@echo "  fbrt                    Build FreeBASIC runtime"
	@echo "  gfxlib2                 Build graphics library"
	@echo "  sfxlib                  Build sound library"
	@echo ""
	@echo "Convenience:"
	@echo "  libs                    Build rtlib + fbrt + gfxlib2 + sfxlib"
	@echo ""
	@echo "Bootstrap:"
	@echo "  bootstrap               Build compiler using bootstrap sources"
	@echo "  bootstrap-minimal       Minimal bootstrap compiler"
	@echo "  bootstrap-emit          Generate bootstrap sources"
	@echo "  bootstrap-emit-matrix   Generate bootstrap sources for the target matrix"
	@echo "  bootstrap-seed-peer     Last resort: seed current bootstrap from a peer target"
	@echo "  bootstrap-dist-target   Create bootstrap tarball"
	@echo ""
	@echo "Testing:"
	@echo "  quick-test              Fast build verification"
	@echo "  full-test               Full test harness"
	@echo ""
	@echo "Install:"
	@echo "  install                 Install compiler and libraries"
	@echo "  uninstall               Remove installed files"
	@echo ""
	@echo "Maintenance:"
	@echo "  clean                   Remove normal build artifacts"
	@echo "  distclean               Remove build/dist artifacts"
	@echo "  clean-bootstrap         Remove bootstrap compiler binary/objects"
	@echo "  clean-bootstrap-src     Remove emitted bootstrap sources"
	@echo "  print-config            Show resolved configuration"

###################
# End of Makefile #
###################
