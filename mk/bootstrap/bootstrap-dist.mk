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
BOOTSTRAP_MATRIX_FBC := $(BOOT_FBC)

#
# BOOTSTRAP_MATRIX accepts either modern matrix entries:
#
#   fbc-target:bootstrap-dir:target-triplet
#
# or a legacy bare bootstrap directory name. The bare form is kept for local
# one-off testing and older scripts that override BOOTSTRAP_MATRIX directly.
#

##############################################################################
# Distribution tools
##############################################################################

BOOT_DIST_TAR := $(if $(strip $(DIST_TAR)),$(DIST_TAR),tar -cJf)
BOOT_DIST_EXT := $(if $(strip $(DIST_EXT)),$(DIST_EXT),tar.xz)
BOOTSTRAP_DIST_WORKTREE ?= 0
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
	# Worktree releases and trees without Git metadata are copied through the
	# source filter.  This includes modified, added, untracked, and deleted
	# source exactly as it exists while keeping generated build output out of
	# the archive.
	#
	rm -rf "$(BOOTSTRAP_STAGE_ROOT)"
	mkdir -p "$(BOOTSTRAP_STAGE_ROOT)"

	@if [ "$(BOOTSTRAP_DIST_WORKTREE)" = "1" ]; then \
		echo "==> Packaging sources via filtered working tree"; \
		mkdir -p "$(BOOTSTRAP_STAGE_DIR)"; \
		rsync -a $(BOOTSTRAP_RSYNC_EXCLUDES) \
			--exclude="/bootstrap/*/" \
			./ "$(BOOTSTRAP_STAGE_DIR)/"; \
	elif command -v git >/dev/null 2>&1 && git rev-parse --is-inside-work-tree >/dev/null 2>&1; then \
		echo "==> Packaging sources via git archive"; \
		git archive --format=tar --prefix="$(BOOTSTRAP_TITLE)/" HEAD | tar -C "$(BOOTSTRAP_STAGE_ROOT)" -xf -; \
	else \
		echo "==> Packaging sources via rsync"; \
		mkdir -p "$(BOOTSTRAP_STAGE_DIR)"; \
		rsync -a $(BOOTSTRAP_RSYNC_EXCLUDES) \
			--exclude="/bootstrap/*/" \
			./ "$(BOOTSTRAP_STAGE_DIR)/"; \
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
	@set -e; \
	test -n "$(BOOTSTRAP_MATRIX_FBC)" || { echo "ERROR: bootstrap matrix emission requires a runnable host compiler"; exit 1; }; \
	for spec in $(BOOTSTRAP_MATRIX); do \
		fbc_target=; \
		dir=; \
		target_triplet=; \
		case "$$spec" in \
		*:*:*) \
			fbc_target=$${spec%%:*}; \
			rest=$${spec#*:}; \
			dir=$${rest%%:*}; \
			target_triplet=$${rest#*:}; \
			;; \
		*:*) \
			fbc_target=$${spec%%:*}; \
			dir=$${spec#*:}; \
			;; \
		*) \
			dir=$$spec; \
			;; \
		esac; \
		test -n "$$dir" || { echo "ERROR: empty bootstrap matrix directory in '$$spec'"; exit 1; }; \
		echo "==> Generating bootstrap sources for $$dir"; \
		test -z "$$fbc_target" || echo "==> fbc target: $$fbc_target"; \
		test -z "$$target_triplet" || echo "==> target triplet: $$target_triplet"; \
		make_args="FBTARGET_DIR_OVERRIDE=$$dir"; \
		test -z "$$fbc_target" || make_args="$$make_args FBC_TARGET=$$fbc_target"; \
		test -z "$$target_triplet" || make_args="$$make_args TARGET_TRIPLET=$$target_triplet"; \
		$(MAKE) bootstrap-emit BOOT_FBC="$(BOOTSTRAP_MATRIX_FBC)" $$make_args; \
	done

##############################################################################
# Convenience wrapper
##############################################################################

bootstrap-dist: bootstrap-dist-target

##############################################################################
# ARM bootstrap matrix (legacy helper)
##############################################################################

bootstrap-dist-arm:
	@set -e; \
	test -n "$(BOOTSTRAP_MATRIX_FBC)" || { echo "ERROR: ARM bootstrap distribution requires a runnable host compiler"; exit 1; }; \
	for spec in \
		linux-arm:linux-armel:arm-linux-gnueabi \
		linux-arm:linux-armhf:arm-linux-gnueabihf \
		linux-aarch64:linux-arm64:aarch64-linux-gnu; do \
		fbc_target=$${spec%%:*}; \
		rest=$${spec#*:}; \
		dir=$${rest%%:*}; \
		target_triplet=$${rest#*:}; \
		echo "==> Generating bootstrap archive for $$dir"; \
		$(MAKE) bootstrap-dist-target \
			BOOT_FBC="$(BOOTSTRAP_MATRIX_FBC)" \
			FBC_TARGET="$$fbc_target" \
			FBTARGET_DIR_OVERRIDE="$$dir" \
			TARGET_TRIPLET="$$target_triplet"; \
	done

##############################################################################
# Full bootstrap matrix
##############################################################################

bootstrap-dist-all:
	@set -e; \
	test -n "$(BOOTSTRAP_MATRIX_FBC)" || { echo "ERROR: bootstrap distribution matrix requires a runnable host compiler"; exit 1; }; \
	for spec in $(BOOTSTRAP_MATRIX); do \
		fbc_target=; \
		dir=; \
		target_triplet=; \
		case "$$spec" in \
		*:*:*) \
			fbc_target=$${spec%%:*}; \
			rest=$${spec#*:}; \
			dir=$${rest%%:*}; \
			target_triplet=$${rest#*:}; \
			;; \
		*:*) \
			fbc_target=$${spec%%:*}; \
			dir=$${spec#*:}; \
			;; \
		*) \
			dir=$$spec; \
			;; \
		esac; \
		test -n "$$dir" || { echo "ERROR: empty bootstrap matrix directory in '$$spec'"; exit 1; }; \
		echo "==> Generating bootstrap archive for $$dir"; \
		test -z "$$fbc_target" || echo "==> fbc target: $$fbc_target"; \
		test -z "$$target_triplet" || echo "==> target triplet: $$target_triplet"; \
		make_args="FBTARGET_DIR_OVERRIDE=$$dir"; \
		test -z "$$fbc_target" || make_args="$$make_args FBC_TARGET=$$fbc_target"; \
		test -z "$$target_triplet" || make_args="$$make_args TARGET_TRIPLET=$$target_triplet"; \
		$(MAKE) bootstrap-dist-target BOOT_FBC="$(BOOTSTRAP_MATRIX_FBC)" $$make_args; \
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
