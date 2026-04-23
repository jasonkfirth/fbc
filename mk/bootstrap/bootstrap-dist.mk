##############################################################################
# bootstrap-dist.mk
# Bootstrap distribution packaging
##############################################################################

.PHONY: \\
	bootstrap-emit-matrix \
	bootstrap-dist \
	bootstrap-dist-target \
	bootstrap-dist-arm \
	bootstrap-dist-all \
	clean-bootstrap-dist

##############################################################################
# Supported target matrix
##############################################################################

include $(mkpath)/supported_targets.mk

BOOTSTRAP_DIR := $(if $(strip $(FBTARGET_DIR_OVERRIDE)),$(FBTARGET_DIR_OVERRIDE),$(FBTARGET))
BOOTSTRAP_MATRIX := $(SUPPORTED_BOOTSTRAP_TARGETS)

##############################################################################
# Distribution tools
##############################################################################

BOOT_DIST_TAR := $(if $(strip $(DIST_TAR)),$(DIST_TAR),tar -cJf)
BOOT_DIST_EXT := $(if $(strip $(DIST_EXT)),$(DIST_EXT),tar.xz)
BOOTSTRAP_RSYNC_EXCLUDES := \
	--prune-empty-dirs \
	--exclude-from="$(mkpath)/source-copy-excludes.rsync"

##############################################################################
# Archive naming
##############################################################################

BOOTSTRAP_TITLE   := FreeBASIC-$(FBVERSION)-source-bootstrap-$(BOOTSTRAP_DIR)
BOOTSTRAP_ARCHIVE := $(BOOTSTRAP_TITLE).$(BOOT_DIST_EXT)

##############################################################################
# Staging layout
##############################################################################

BOOTSTRAP_STAGE_ROOT := stage/bootstrap-dist
BOOTSTRAP_STAGE_DIR  := $(BOOTSTRAP_STAGE_ROOT)/$(BOOTSTRAP_TITLE)

##############################################################################
# Primary bootstrap distribution target
##############################################################################

bootstrap-dist-target: bootstrap-check bootstrap-emit
	@echo "==> Building bootstrap distribution for $(BOOTSTRAP_DIR)"
	@echo "==> Output archive: $(BOOTSTRAP_ARCHIVE)"
	@$(MAKE) clean-example-artifacts

	#
	# The rsync fallback is used when the source tree is not a git checkout.
	# In that mode we must avoid staging generated build output such as
	# previous package trees, test scratch directories, and prior bootstrap
	# staging roots. Including them makes the archive much larger than the
	# real source bootstrap payload and can recursively drag prior staging
	# content into new archives.
	#
	rm -rf "$(BOOTSTRAP_STAGE_ROOT)"
	mkdir -p "$(BOOTSTRAP_STAGE_ROOT)"

	@if command -v git >/dev/null 2>&1 && git rev-parse --is-inside-work-tree >/dev/null 2>&1; then \
		echo "==> Packaging sources via git archive"; \
		git archive --format=tar --prefix="$(BOOTSTRAP_TITLE)/" HEAD | tar -C "$(BOOTSTRAP_STAGE_ROOT)" -xf -; \
	else \
		echo "==> Packaging sources via rsync"; \
		mkdir -p "$(BOOTSTRAP_STAGE_DIR)"; \
		rsync -a $(BOOTSTRAP_RSYNC_EXCLUDES) ./ "$(BOOTSTRAP_STAGE_DIR)/"; \
	fi

	mkdir -p "$(BOOTSTRAP_STAGE_DIR)/bootstrap/$(BOOTSTRAP_DIR)"

	rsync -a "bootstrap/$(BOOTSTRAP_DIR)/" "$(BOOTSTRAP_STAGE_DIR)/bootstrap/$(BOOTSTRAP_DIR)/"

	rm -f "$(BOOTSTRAP_ARCHIVE)"

	$(BOOT_DIST_TAR) "$(BOOTSTRAP_ARCHIVE)" -C "$(BOOTSTRAP_STAGE_ROOT)" "$(BOOTSTRAP_TITLE)"

	@echo "==> Wrote $(BOOTSTRAP_ARCHIVE)"

##############################################################################
# Full bootstrap emission matrix
##############################################################################

bootstrap-emit-matrix:
	@for d in $(BOOTSTRAP_MATRIX); do \
		echo "==> Generating bootstrap sources for $$d"; \
		$(MAKE) bootstrap-emit FBTARGET_DIR_OVERRIDE=$$d; \
	done

##############################################################################
# Convenience wrapper
##############################################################################

bootstrap-dist: bootstrap-dist-target

##############################################################################
# ARM bootstrap matrix (legacy helper)
##############################################################################

bootstrap-dist-arm:
	@for d in linux-armel linux-armhf linux-arm64; do \
		echo "==> Generating bootstrap archive for $$d"; \
		$(MAKE) bootstrap-dist-target FBTARGET_DIR_OVERRIDE=$$d; \
	done

##############################################################################
# Full bootstrap matrix
##############################################################################

bootstrap-dist-all:
	@for d in $(BOOTSTRAP_MATRIX); do \
		echo "==> Generating bootstrap archive for $$d"; \
		$(MAKE) bootstrap-dist-target FBTARGET_DIR_OVERRIDE=$$d; \
	done

##############################################################################
# Cleaning helpers
##############################################################################

clean-bootstrap-dist:
	rm -rf "$(BOOTSTRAP_STAGE_ROOT)"
	rm -f FreeBASIC-*source-bootstrap-*.$(BOOT_DIST_EXT)

##############################################################################
# End bootstrap-dist.mk
##############################################################################
