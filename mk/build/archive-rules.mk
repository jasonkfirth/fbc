##############################################################################
# Project: FreeBASIC build system
# File: archive-rules.mk
#
# Purpose: Build and publish complete runtime archives.
# Responsibilities: Archive selected objects, index them, and handle tool failures.
# This file intentionally does NOT select sources or compiler link inputs.
#
# Consumes object lists from source-graph.mk
##############################################################################

# Keep the object list out of the native command line. On Windows, the
# expanded rtlib list can exceed the process argument limit.
# GNU Make expands file functions even under -n. Inspect only its initial
# short-option word so --no-print-directory does not count as the n flag.
# Publish only a fully indexed replacement, retaining the previous archive
# if either tool fails. The trap owns only this archive's temporary files.
define DO_AR
	$(if $(findstring n,$(filter-out --%,$(firstword $(MAKEFLAGS)))),,$(file >$@.rsp,$(filter %.o,$^)))
	@set -e; \
	members="$(ARCHIVE_MEMBERS_DIR)/$(notdir $@)"; \
	trap 'rm -f "$@.tmp" "$@.rsp" "$$members.tmp"' 0; \
	rm -f "$@.tmp"; \
	$(AR) $(ARFLAGS) "$@.tmp" @"$@.rsp"; \
	$(RANLIB) "$@.tmp"; \
	mv -f "$@.tmp" "$@"; \
	cp "$@.rsp" "$$members.tmp"; \
	mv -f "$$members.tmp" "$$members"
endef

# Make notices newer inputs, but removing an input does not make an archive
# older than any remaining object. Compare the selected list with the list
# recorded after the last successful publication, independently of filesystem
# timestamp granularity. Failed builds leave that record unchanged so retries
# remain necessary. This also avoids rewriting stamps on unchanged builds.
# Keep metadata hidden so install/distribution rules do not copy it into an SDK.
ARCHIVE_MEMBERS_DIR := $(libdir)/.archive-members
.PHONY: archive-members-force
archive-members-force:

$(ARCHIVE_MEMBERS_DIR):
	@mkdir -p "$@"

archive_previous_members = $(if $(wildcard $(libdir)/$(1)),$(strip $(shell cat "$(ARCHIVE_MEMBERS_DIR)/$(1)" 2>/dev/null)))

define declare_archive_members
ifneq ($(strip $(2)),$(call archive_previous_members,$(1)))
$(libdir)/$(1): archive-members-force
endif
$(libdir)/$(1): | $(ARCHIVE_MEMBERS_DIR)
endef

$(eval $(call declare_archive_members,libfb.a,$(RTLIB_OBJ)))
$(eval $(call declare_archive_members,libfbpic.a,$(RTLIB_PIC_OBJ)))
$(eval $(call declare_archive_members,libfbmt.a,$(RTLIB_MT_OBJ)))
$(eval $(call declare_archive_members,libfbmtpic.a,$(RTLIB_MT_PIC_OBJ)))
$(eval $(call declare_archive_members,libfbrt.a,$(FBRT_OBJ)))
$(eval $(call declare_archive_members,libfbrtpic.a,$(FBRT_PIC_OBJ)))
$(eval $(call declare_archive_members,libfbrtmt.a,$(FBRT_MT_OBJ)))
$(eval $(call declare_archive_members,libfbrtmtpic.a,$(FBRT_MT_PIC_OBJ)))
$(eval $(call declare_archive_members,libfbgfx.a,$(GFX_OBJ)))
$(eval $(call declare_archive_members,libfbgfxpic.a,$(GFX_PIC_OBJ)))
$(eval $(call declare_archive_members,libfbgfxmt.a,$(GFX_MT_OBJ)))
$(eval $(call declare_archive_members,libfbgfxmtpic.a,$(GFX_MT_PIC_OBJ)))
$(eval $(call declare_archive_members,libfbgfx3.a,$(GFX3_OBJ)))
$(eval $(call declare_archive_members,libfbgfx3pic.a,$(GFX3_PIC_OBJ)))
$(eval $(call declare_archive_members,libfbgfx3mt.a,$(GFX3_MT_OBJ)))
$(eval $(call declare_archive_members,libfbgfx3mtpic.a,$(GFX3_MT_PIC_OBJ)))
$(eval $(call declare_archive_members,libsfx.a,$(SFX_OBJ)))
$(eval $(call declare_archive_members,libsfxpic.a,$(SFX_PIC_OBJ)))
$(eval $(call declare_archive_members,libsfxmt.a,$(SFX_MT_OBJ)))
$(eval $(call declare_archive_members,libsfxmtpic.a,$(SFX_MT_PIC_OBJ)))

##############################################################################
# Core runtime (rtlib)
##############################################################################

$(libdir)/libfb.a: $(RTLIB_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbpic.a: $(RTLIB_PIC_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbmt.a: $(RTLIB_MT_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbmtpic.a: $(RTLIB_MT_PIC_OBJ) | $(libdir)
	$(call DO_AR)

ifeq ($(TARGET_OS),amiga)
AMIGA_SUPPORT_ROOT ?= $(rootdir)/out/amiga
AMIGA_TOOLCHAIN_ROOT ?= $(AMIGA_SUPPORT_ROOT)/toolchain

$(libdir)/libfbsoftfloat.a: $(rootdir)/build_scripts/amiga-build-softfloat.py $(rootdir)/build_scripts/amiga/softfloat-convert.c | $(libdir)
	python3 "$(rootdir)/build_scripts/amiga-build-softfloat.py" --toolchain-root "$(AMIGA_TOOLCHAIN_ROOT)" --work "$(AMIGA_SUPPORT_ROOT)/softfloat" --output "$@"

$(libdir)/libpthread.a: $(rootdir)/build_scripts/amiga-build-pthread.py | $(libdir)
	python3 "$(rootdir)/build_scripts/amiga-build-pthread.py" --toolchain-root "$(AMIGA_TOOLCHAIN_ROOT)" --work "$(AMIGA_SUPPORT_ROOT)/pthread" --output "$@"

$(libdir)/libffi.a: $(rootdir)/build_scripts/amiga-build-ffi.py | $(libdir)
	python3 "$(rootdir)/build_scripts/amiga-build-ffi.py" --toolchain-root "$(AMIGA_TOOLCHAIN_ROOT)" --work "$(AMIGA_SUPPORT_ROOT)/ffi" --output "$@"
endif

ifeq ($(THREAD_MODEL),pdmlwp)
DOS_PDMLWP_DIR := $(rootdir)/contrib/dos/pdmlwp
DOS_PDMLWP_OBJ := $(libfbmtobjdir)/pdmlwp/lwp.o $(libfbmtobjdir)/pdmlwp/lwpasm.o
$(eval $(call declare_archive_members,libfblwp.a,$(DOS_PDMLWP_OBJ)))

# The imported scheduler locks regions bounded by source-order markers.
# Disable function/data reordering and LTO for these two objects only.
$(libfbmtobjdir)/pdmlwp/lwp.o: $(DOS_PDMLWP_DIR)/src/lwp.c $(DOS_PDMLWP_DIR)/include/lwp.h
	@mkdir -p "$(dir $@)"
	$(RUN_CC) -I$(DOS_PDMLWP_DIR)/include -std=gnu11 -O1 -march=i386 -mno-sse -mno-mmx -fno-lto -fno-toplevel-reorder -fno-reorder-functions -fno-strict-aliasing -fno-omit-frame-pointer -c $< -o $@

$(libfbmtobjdir)/pdmlwp/lwpasm.o: $(DOS_PDMLWP_DIR)/src/lwpasm.s
	@mkdir -p "$(dir $@)"
	$(RUN_CC) -x assembler-with-cpp -c $< -o $@

$(libdir)/libfblwp.a: $(DOS_PDMLWP_OBJ) | $(libdir)
	$(call DO_AR)
endif

##############################################################################
# FreeBASIC runtime layer (fbrt)
##############################################################################

$(libdir)/libfbrt.a: $(FBRT_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbrtpic.a: $(FBRT_PIC_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbrtmt.a: $(FBRT_MT_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbrtmtpic.a: $(FBRT_MT_PIC_OBJ) | $(libdir)
	$(call DO_AR)

##############################################################################
# Graphics runtime (gfxlib2)
##############################################################################

$(libdir)/libfbgfx.a: $(GFX_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbgfxpic.a: $(GFX_PIC_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbgfxmt.a: $(GFX_MT_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbgfxmtpic.a: $(GFX_MT_PIC_OBJ) | $(libdir)
	$(call DO_AR)

##############################################################################
# Experimental GPU graphics runtime (gfxlib3)
##############################################################################

$(libdir)/libfbgfx3.a: $(GFX3_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbgfx3pic.a: $(GFX3_PIC_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbgfx3mt.a: $(GFX3_MT_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libfbgfx3mtpic.a: $(GFX3_MT_PIC_OBJ) | $(libdir)
	$(call DO_AR)

##############################################################################
# Sound runtime (sfxlib)
##############################################################################

$(libdir)/libsfx.a: $(SFX_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libsfxpic.a: $(SFX_PIC_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libsfxmt.a: $(SFX_MT_OBJ) | $(libdir)
	$(call DO_AR)

$(libdir)/libsfxmtpic.a: $(SFX_MT_PIC_OBJ) | $(libdir)
	$(call DO_AR)

##############################################################################
# END archive-rules.mk
##############################################################################
