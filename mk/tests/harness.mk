##############################################################################
# tests/harness.mk
#
# Shared test harness utilities
##############################################################################

##############################################################################
# Paths
##############################################################################

TEST_TMP := .maketests-tmp
TEST_HOST_DIR := .maketests-host
LOG_DIR  := maketests-log
SRC_ROOT := $(CURDIR)

TEST_INPUT_FBC := $(strip $(or $(BUILD_FBC),$(LOCAL_FBC),$(SYSTEM_FBC),$(AVAILABLE_FBC)))
TEST_HOST_FBC := $(abspath $(TEST_HOST_DIR)/fbc$(EXEEXT))
TEST_HOST_BINDIR := $(patsubst %/,%,$(dir $(TEST_HOST_FBC)))
TEST_FBC := $(if $(CROSS_BUILD),$(TEST_HOST_FBC),$(if $(wildcard $(FBC_EXE)),$(abspath $(FBC_EXE)),$(TEST_HOST_FBC)))
TEST_FBC_TARGET_ARGS := $(if $(strip $(BUILD_FBC_TARGET)),-target $(BUILD_FBC_TARGET))
TEST_FBC_BUILDPREFIX_ARGS := $(if $(strip $(BUILD_FBC_BUILDPREFIX)),-buildprefix $(BUILD_FBC_BUILDPREFIX))
TEST_TOOLCHAIN_BINDIR := $(call tool_bindir,$(CC))
TEST_TOOLCHAIN_ENV := env \
	BUILD_FBC="$(TEST_HOST_FBC)" \
	PATH="$(TEST_HOST_BINDIR):$(if $(strip $(TEST_TOOLCHAIN_BINDIR)),$(TEST_TOOLCHAIN_BINDIR):)$$PATH" \
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
TEST_FBC_CMD := $(TEST_TOOLCHAIN_ENV) "$(TEST_FBC)" $(TEST_FBC_TARGET_ARGS) $(TEST_FBC_BUILDPREFIX_ARGS)

##############################################################################
# Helpers
##############################################################################

define _mt_echo
	@echo "==> $(1)"
endef

define _mt_run
	@echo "==> RUN: $(1)"
	@$(TEST_TOOLCHAIN_ENV) sh -ec '$(1)'
endef

define _mt_fail
	@echo ""
	@echo "ERROR: $(1)"
	@echo ""
	@exit 1
endef

define _mt_cleanup_success
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)" "stage"
	@rm -f "$(TEST_TMP).install_manifest" FreeBASIC-*source-bootstrap-*.tar.xz
endef

##############################################################################
# Preserve the host compiler used by destructive test runs
##############################################################################

.PHONY: maketests-preserve-host-fbc
maketests-preserve-host-fbc: | prereqs-fbc
	$(call _mt_echo,Preserving host FreeBASIC compiler)
	@test -n "$(TEST_INPUT_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	@test -x "$(TEST_INPUT_FBC)" || { echo "ERROR: fbc is not executable: $(TEST_INPUT_FBC)"; exit 1; }
	@mkdir -p "$(TEST_HOST_DIR)"
	@src="$(TEST_INPUT_FBC)"; \
	dst="$(TEST_HOST_FBC)"; \
	src_dir=$$(cd "$$(dirname "$$src")" && pwd -P); \
	dst_dir=$$(cd "$$(dirname "$$dst")" && pwd -P); \
	if [ "$$src_dir/$$(basename "$$src")" != "$$dst_dir/$$(basename "$$dst")" ]; then \
		cp -f "$$src" "$$dst"; \
	fi
	@chmod +x "$(TEST_HOST_FBC)"
	@$(TEST_TOOLCHAIN_ENV) "$(TEST_HOST_FBC)" -version >/dev/null

##############################################################################
# Run a command and detect which FreeBASIC-*.tar.xz archive was created
##############################################################################

define _mt_find_new_archives
	tmpdir="$(SRC_ROOT)/$(TEST_TMP)"; \
	mkdir -p "$$tmpdir"; \
	before="$$tmpdir/archives.before"; \
	after="$$tmpdir/archives.after"; \
	new="$$tmpdir/archives.new"; \
	ls -1 FreeBASIC-*.tar.xz 2>/dev/null | sort >"$$before" || true; \
	$(TEST_TOOLCHAIN_ENV) sh -ec '$(1)' >&2; \
	ls -1 FreeBASIC-*.tar.xz 2>/dev/null | sort >"$$after" || true; \
	comm -13 "$$before" "$$after" >"$$new" || true; \
	ARCHIVE=$$(head -n1 "$$new"); \
	if [ -z "$$ARCHIVE" ]; then \
	        echo "" >&2; \
	        echo "ERROR: no new FreeBASIC-*.tar.xz produced" >&2; \
	        echo "" >&2; \
	        ls -1 >&2; \
	        exit 1; \
	fi; \
	printf '%s\n' "$$ARCHIVE"
endef

##############################################################################
# Environment detection
##############################################################################

HOST_DUMPMACHINE := $(shell $(CC) -dumpmachine 2>/dev/null || echo)

ifeq ($(origin CROSS_BUILD),undefined)
CROSS_BUILD := $(and $(strip $(TARGET_TRIPLET)),$(filter-out $(strip $(TARGET_TRIPLET)),$(strip $(HOST_DUMPMACHINE))))
endif

RUNNABLE_OS := linux darwin freebsd netbsd openbsd dragonfly solaris illumos win32 cygwin dos
CAN_RUN := $(and $(filter $(TARGET_OS),$(RUNNABLE_OS)),$(if $(CROSS_BUILD),,yes))

##############################################################################
# Test-safe cleaning
##############################################################################

.PHONY: test-clean
test-clean:
	$(call _mt_echo,Test-safe clean (preserve compiler + bootstrap))
	@$(MAKE) clean-libs
	@$(MAKE) clean-compiler
	@$(MAKE) clean-all-objects

##############################################################################
# Cleaning test artifacts
##############################################################################

.PHONY: clean-maketests
clean-maketests:
	$(call _mt_echo,Cleaning test artifacts)
	@rm -rf "$(TEST_TMP)" "$(TEST_HOST_DIR)" "$(LOG_DIR)"

.PHONY: clean-maketests-success
clean-maketests-success:
	$(call _mt_echo,Cleaning successful test artifacts)
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)" "stage"
	@rm -f "$(TEST_TMP).install_manifest" FreeBASIC-*source-bootstrap-*.tar.xz

.PHONY: clean-maketests-host
clean-maketests-host:
	$(call _mt_echo,Cleaning preserved test compiler)
	@rm -rf "$(TEST_HOST_DIR)"

##############################################################################
# end of tests/harness.mk
##############################################################################
