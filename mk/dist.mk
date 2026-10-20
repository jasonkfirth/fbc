################
# dist.mk
################
# Project: FreeBASIC build system
# Purpose: Stage complete binary distributions and produce tar/zip archives.
# Responsibilities: Order cleanup, staging, and packaging of the native SDK.
# This file intentionally does NOT define installation layout or compile rules.

DISTROOT := dist
DISTDIR  := $(DISTROOT)/freebasic
DIST_BINDIR := $(prefixbindir)
DIST_INCDIR := $(prefixincdir)
DIST_RUNTIME_DIR := $(FBINSTALL_RUNTIME_DIR)

ifneq ($(filter win32 dos,$(TARGET_OS)),)
DIST_BINDIR := $(patsubst $(prefix)%,%,$(prefixbindir))
DIST_INCDIR := $(patsubst $(prefix)%,%,$(prefixincdir))
DIST_RUNTIME_DIR := $(patsubst $(prefix)%,%,$(FBINSTALL_RUNTIME_DIR))
endif

.PHONY: dist
dist: dist-package

.PHONY: dist-clean
dist-clean:
	rm -rf "$(DISTROOT)"

.PHONY: dist-stage
dist-stage: dist-bin dist-includes dist-runtime

# Cleanup must finish before any staging starts, including under parallel
# make. Packaging then reads the complete shared stage without modifying it.
dist-bin dist-includes dist-runtime: | dist-clean

.PHONY: dist-bin
dist-bin: $(FBC_EXE)
	mkdir -p "$(DISTDIR)$(DIST_BINDIR)"
	cp "$(FBC_EXE)" "$(DISTDIR)$(DIST_BINDIR)/fbc$(EXEEXT)"

.PHONY: dist-includes
dist-includes:
	mkdir -p "$(DISTDIR)$(DIST_INCDIR)"
	cp -a "$(rootdir)/inc/." "$(DISTDIR)$(DIST_INCDIR)/"

.PHONY: dist-runtime
dist-runtime: runtime-libs
# Platform runtime headers and provider source bundles are part of the same
# payload as installation. Keep that selection in one owner; its file loop
# also excludes private archive-membership metadata.
	$(MAKE) DESTDIR="$(abspath $(DISTDIR))" install-runtime

.PHONY: dist-package
dist-package: dist-stage
	tar -C "$(DISTROOT)" -cf freebasic-dist.tar freebasic

.PHONY: dist-zip
dist-zip: dist-stage
	rm -f freebasic-dist.zip
	cd "$(DISTROOT)" && zip -r ../freebasic-dist.zip freebasic

# end of dist.mk
