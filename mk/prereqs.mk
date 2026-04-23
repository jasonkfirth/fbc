##############################################################################
# prereqs.mk
#
# Extended prerequisite checks
#
# Philosophy:
#   - Checks run ONLY when "make prereqs" is invoked
#   - Avoid heavy shell execution during Makefile parsing
#   - Clear error reporting
##############################################################################

HAVE_PREREQS_MK := yes

##############################################################################
# Platform detection (lightweight)
##############################################################################

UNAME_S ?= $(shell uname -s 2>/dev/null || echo unknown)

IS_WINDOWS := $(filter MINGW% MSYS% CYGWIN%,$(UNAME_S))
IS_DARWIN  := $(filter Darwin,$(UNAME_S))
IS_BSD     := $(filter FreeBSD OpenBSD NetBSD DragonFly,$(UNAME_S))
IS_HAIKU   := $(filter Haiku,$(UNAME_S))

##############################################################################
# Helper macros
##############################################################################

define pr_error_block
	{ \
		echo ""; \
		echo "ERROR: $(1)"; \
		echo ""; \
		exit 1; \
	}
endef

define pr_check_cmd
	command -v $(1) >/dev/null 2>&1 || \
	$(call pr_error_block,Missing required tool: $(1))
endef

define pr_check_pc
	pkg-config --exists $(1) >/dev/null 2>&1 || \
	$(call pr_error_block,Missing required library via pkg-config: $(1))
endef

define pr_check_hdr
	printf "%s\n" "#include <$(1)>" | $(PR_CC) -E - >/dev/null 2>&1 || \
	$(call pr_error_block,Missing required header: $(1))
endef

##############################################################################
# Required tools
##############################################################################

PR_COMMON_TOOLS := \
	make \
	ar \
	as \
	ld \
	ranlib \
	tar \
	xz \
	gzip \
	sed \
	awk \
	grep \
	find \
	rsync

# Normalize compiler command (handles CC="gcc -m32" etc)
PR_CC_RAW := $(strip $(if $(CC),$(CC),gcc))
PR_CC     := $(firstword $(PR_CC_RAW))

PR_BOOTSTRAP_DIR := $(if $(strip $(FBTARGET_DIR_OVERRIDE)),$(FBTARGET_DIR_OVERRIDE),$(FBTARGET))
PR_BOOTSTRAP_PATH := bootstrap/$(PR_BOOTSTRAP_DIR)
PR_BOOTSTRAP_DIRS := $(patsubst bootstrap/%/,%,$(wildcard bootstrap/*/))
PR_BOOTSTRAP_BSD_DIRS := openbsd-$(TARGET_ARCH) netbsd-$(TARGET_ARCH) freebsd-$(TARGET_ARCH) dragonfly-$(TARGET_ARCH)
PR_BOOTSTRAP_FAMILY_DONORS :=
ifneq ($(filter $(TARGET_OS),openbsd netbsd freebsd dragonfly),)
PR_BOOTSTRAP_FAMILY_DONORS := $(filter $(PR_BOOTSTRAP_BSD_DIRS),$(PR_BOOTSTRAP_DIRS))
endif
PR_BOOTSTRAP_SAME_ARCH_DONORS := $(filter %-$(TARGET_ARCH),$(PR_BOOTSTRAP_DIRS))
PR_BOOTSTRAP_DONOR_CANDIDATES := $(filter-out $(PR_BOOTSTRAP_DIR),$(PR_BOOTSTRAP_FAMILY_DONORS) $(filter-out $(PR_BOOTSTRAP_FAMILY_DONORS),$(PR_BOOTSTRAP_SAME_ARCH_DONORS)))

##############################################################################
# Phony targets
##############################################################################

.PHONY: prereqs prereqs-env prereqs-tools prereqs-libs prereqs-fbc prereqs-print

prereqs: prereqs-env prereqs-tools prereqs-libs

##############################################################################
# Environment notes
##############################################################################

prereqs-env:
	@echo "==> Environment detection"
	@echo "UNAME_S=$(UNAME_S)"
ifneq ($(IS_WINDOWS),)
	@echo "Platform: Windows/MSYS/Cygwin"
endif
ifneq ($(IS_DARWIN),)
	@echo "Platform: macOS"
endif
ifneq ($(IS_BSD),)
	@echo "Platform: BSD"
endif
ifneq ($(IS_HAIKU),)
	@echo "Platform: Haiku (experimental)"
endif

##############################################################################
# Tool checks
##############################################################################

prereqs-tools:
	@echo "==> Checking required tools"
	@$(call pr_check_cmd,$(PR_CC))
	@$(foreach t,$(PR_COMMON_TOOLS),$(call pr_check_cmd,$(t));)

##############################################################################
# Library checks
##############################################################################

prereqs-libs:
	@echo "==> Checking required libraries"

ifneq ($(IS_WINDOWS),)
	@true
else
	@$(call pr_check_cmd,pkg-config)

	@echo "Checking ncurses"
	@$(call pr_check_pc,ncurses)

	@echo "Checking libm"
	@$(call pr_check_hdr,math.h)

	@echo "Checking pthread"
	@$(call pr_check_hdr,pthread.h)
endif

##############################################################################
# FreeBASIC compiler / bootstrap checks
##############################################################################

prereqs-fbc:
	@echo "==> Checking FreeBASIC compiler availability"
	@if [ -n "$(BUILD_FBC)" ] && [ -x "$(BUILD_FBC)" ]; then \
		echo "Using explicit build compiler: $(BUILD_FBC)"; \
	elif [ -n "$(LOCAL_FBC)" ]; then \
		echo "Using local compiler: $(LOCAL_FBC)"; \
	elif [ -n "$(SYSTEM_FBC)" ]; then \
		echo "Using system compiler: $(SYSTEM_FBC)"; \
	elif [ -d "$(PR_BOOTSTRAP_PATH)" ] && find "$(PR_BOOTSTRAP_PATH)" -maxdepth 1 -type f \( -name '*.c' -o -name '*.asm' \) -print -quit | grep -q .; then \
		echo ""; \
		echo "ERROR: no usable FreeBASIC compiler found."; \
		echo "Bootstrap sources are available for $(PR_BOOTSTRAP_DIR)."; \
		echo "Run: make bootstrap-minimal"; \
		echo ""; \
		exit 1; \
	else \
		echo ""; \
		echo "ERROR: no usable FreeBASIC compiler found."; \
		echo "No bootstrap sources are available for $(PR_BOOTSTRAP_DIR)."; \
		if [ -n "$(strip $(PR_BOOTSTRAP_DONOR_CANDIDATES))" ]; then \
			echo "Last resort: make bootstrap-seed-peer"; \
			echo "Candidate donor bootstrap dirs: $(PR_BOOTSTRAP_DONOR_CANDIDATES)"; \
		fi; \
		echo ""; \
		exit 1; \
	fi

##############################################################################
# Optional feature checks (run manually)
##############################################################################

.PHONY: prereqs-optional

prereqs-optional:
	@echo "==> Checking optional libraries"

	@if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists libffi; then \
		echo "libffi detected (FFI enabled)"; \
	else \
		echo "libffi missing (FFI disabled)"; \
	fi

	@if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists x11; then \
		echo "X11 detected"; \
	else \
		echo "X11 missing (graphics features may be reduced)"; \
	fi

##############################################################################
# Debug helper
##############################################################################

prereqs-print:
	@echo "UNAME_S=$(UNAME_S)"
	@echo "CC=$(CC)"
	@echo "PR_CC_RAW=$(PR_CC_RAW)"
	@echo "PR_CC=$(PR_CC)"
	@echo "LOCAL_FBC=$(LOCAL_FBC)"
	@echo "SYSTEM_FBC=$(SYSTEM_FBC)"
	@echo "AVAILABLE_FBC=$(AVAILABLE_FBC)"
	@echo "PR_BOOTSTRAP_DIR=$(PR_BOOTSTRAP_DIR)"
	@echo "PR_BOOTSTRAP_DONOR_CANDIDATES=$(PR_BOOTSTRAP_DONOR_CANDIDATES)"

##############################################################################
# End prereqs.mk
##############################################################################
