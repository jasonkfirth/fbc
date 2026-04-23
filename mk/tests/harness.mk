##############################################################################
# tests/harness.mk
#
# Shared test harness utilities
##############################################################################

##############################################################################
# Paths
##############################################################################

TEST_TMP := .maketests-tmp
LOG_DIR  := maketests-log
SRC_ROOT := $(CURDIR)

TEST_FBC := $(if $(LOCAL_FBC),$(LOCAL_FBC),$(AVAILABLE_FBC))
TEST_TOOLCHAIN_BINDIR :=
ifneq ($(findstring /,$(CC))$(findstring \,$(CC)),)
TEST_TOOLCHAIN_BINDIR := $(patsubst %/,%,$(dir $(CC)))
endif
TEST_TOOLCHAIN_ENV := env \
	PATH='$(if $(strip $(TEST_TOOLCHAIN_BINDIR)),$(TEST_TOOLCHAIN_BINDIR):)'"$$PATH" \
	AS='$(AS)' \
	AR='$(AR)' \
	LD='$(LD)' \
	GCC='$(CC)' \
	CLANG='$(CLANG)' \
	LLC='$(LLC)' \
	DLLTOOL='$(DLLTOOL)' \
	WINDRES='$(WINDRES)' \
	GORC='$(GORC)' \
	EMAS='$(EMAS)' \
	EMAR='$(EMAR)' \
	EMLD='$(EMLD)' \
	EMCC='$(EMCC)' \
	CXBE='$(CXBE)' \
	DXEGEN='$(DXEGEN)'
TEST_FBC_CMD := $(TEST_TOOLCHAIN_ENV) "$(TEST_FBC)"

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
# Run a command and detect which FreeBASIC-*.tar.xz archive was created
##############################################################################

define _mt_find_new_archives
	@tmpdir="$(SRC_ROOT)/$(TEST_TMP)"; \
	mkdir -p "$$tmpdir"; \
	before="$$tmpdir/archives.before"; \
	after="$$tmpdir/archives.after"; \
	new="$$tmpdir/archives.new"; \
	ls -1 FreeBASIC-*.tar.xz 2>/dev/null | sort >"$$before" || true; \
	sh -ec '$(1)' >&2; \
	ls -1 FreeBASIC-*.tar.xz 2>/dev/null | sort >"$$after" || true; \
	comm -13 "$$before" "$$after" >"$$new" || true; \
	ARCHIVE=$$(head -n1 "$$new"); \
	if [ -z "$$ARCHIVE" ]; then \
	        echo ""; \
	        echo "ERROR: no new FreeBASIC-*.tar.xz produced"; \
	        echo ""; \
	        ls -1; \
	        exit 1; \
	fi; \
	echo "$$ARCHIVE"
endef

##############################################################################
# Environment detection
##############################################################################

HOST_DUMPMACHINE := $(shell $(CC) -dumpmachine 2>/dev/null || echo)

CROSS_BUILD := $(and $(strip $(TARGET_TRIPLET)),$(filter-out $(strip $(TARGET_TRIPLET)),$(strip $(HOST_DUMPMACHINE))))

RUNNABLE_OS := linux darwin freebsd netbsd openbsd dragonfly solaris win32 cygwin dos
CAN_RUN := $(and $(filter $(TARGET_OS),$(RUNNABLE_OS)),$(if $(CROSS_BUILD),,yes))

##############################################################################
# Test-safe cleaning
##############################################################################

.PHONY: test-clean
test-clean:
	$(call _mt_echo,Test-safe clean (preserve compiler + bootstrap))
	@$(MAKE) clean-libs
	@$(MAKE) clean-compiler

##############################################################################
# Cleaning test artifacts
##############################################################################

.PHONY: clean-maketests
clean-maketests:
	$(call _mt_echo,Cleaning test artifacts)
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)"

.PHONY: clean-maketests-success
clean-maketests-success:
	$(call _mt_echo,Cleaning successful test artifacts)
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)" "stage"
	@rm -f "$(TEST_TMP).install_manifest" FreeBASIC-*source-bootstrap-*.tar.xz

##############################################################################
# end of tests/harness.mk
##############################################################################
