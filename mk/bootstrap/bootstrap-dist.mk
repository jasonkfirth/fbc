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
		if [ "$(BOOTSTRAP_DIST_WORKTREE)" = "1" ]; then \
			echo "==> Packaging sources via git archive plus worktree changes"; \
			changed="$$(mktemp)"; \
			git archive --format=tar --prefix="$(BOOTSTRAP_TITLE)/" HEAD | tar -C "$(BOOTSTRAP_STAGE_ROOT)" -xf -; \
			git diff --name-only -z --diff-filter=ACMRT HEAD -- > "$$changed"; \
			if [ -s "$$changed" ]; then \
				tar -cf - --null -T "$$changed" | tar -C "$(BOOTSTRAP_STAGE_DIR)" -xf -; \
			fi; \
			rm -f "$$changed"; \
		else \
			echo "==> Packaging sources via git archive"; \
			git archive --format=tar --prefix="$(BOOTSTRAP_TITLE)/" HEAD | tar -C "$(BOOTSTRAP_STAGE_ROOT)" -xf -; \
		fi; \
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
	@set -e; \
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
		$(MAKE) bootstrap-emit $$make_args; \
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
			FBC_TARGET="$$fbc_target" \
			FBTARGET_DIR_OVERRIDE="$$dir" \
			TARGET_TRIPLET="$$target_triplet"; \
	done

##############################################################################
# Full bootstrap matrix
##############################################################################

bootstrap-dist-all:
	@set -e; \
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
		$(MAKE) bootstrap-dist-target $$make_args; \
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
