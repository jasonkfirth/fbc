##############################################################################
# tests.mk
#
# Unit / log / warning test harness
##############################################################################

.PHONY: unit-tests log-tests warning-tests clean-tests ustring-test

TESTS_FBC := $(if $(LOCAL_FBC),$(abspath $(LOCAL_FBC)),$(AVAILABLE_FBC))
TESTS_TOOLCHAIN_BINDIR := $(call tool_bindir,$(CC))
TESTS_FBC_ENV := env \
	$(TOOLCHAIN_FBC_ENV) \
	PATH='$(if $(strip $(TESTS_TOOLCHAIN_BINDIR)),$(TESTS_TOOLCHAIN_BINDIR):)'"$$PATH" \
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
TESTS_FBC_CMD := $(TESTS_FBC_ENV) "$(TESTS_FBC)"

# Recursive C/C++ fixtures must use the same deployment floor as fbc. The
# standalone harness reads this setting for both compiler commands; passing it
# only inside FBC leaves clang++ targeting the current SDK and mixes object ABIs.
ifeq ($(TARGET_OS),darwin)
export TESTS_DARWIN_DEPLOYMENT_TARGET ?= $(DARWIN_DEPLOYMENT_TARGET)
endif

# Tests that compile and link non-fbcunit sources expect the full platform runtime
# artifacts to exist in the active $(libdir) layout.
TESTS_RUNTIME_LIBS := $(RTL_LIBS) $(FBRTL_LIBS) $(GFX_LIBS) $(SFX_LIBS)

##############################################################################
# Unit tests
##############################################################################

unit-tests: | maybe-build-fbc $(TESTS_RUNTIME_LIBS)
	@test -n "$(TESTS_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	cd tests && $(MAKE) unit-tests \
		FBC="$(TESTS_FBC_CMD) -i $(rootdir)/inc"

##############################################################################
# Log tests
##############################################################################

log-tests: | maybe-build-fbc $(TESTS_RUNTIME_LIBS)
	@test -n "$(TESTS_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	cd tests && $(MAKE) log-tests \
		FBC="$(TESTS_FBC_CMD) -i $(rootdir)/inc"

##############################################################################
# Warning tests
##############################################################################

warning-tests: | maybe-build-fbc $(TESTS_RUNTIME_LIBS)
	@test -n "$(TESTS_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	@chmod +x tests/warnings/test.sh 2>/dev/null || true
	cd tests/warnings && \
		FBC="$(TESTS_FBC_CMD)" ./test.sh

##############################################################################
# USTRING needs the compiler and runtime built from the same source tree.
# The runner keeps its unit library, examples, and sanitizer artifacts in a
# temporary directory, so it can be used alongside the normal test harness.
##############################################################################

ustring-test: compiler rtlib gfxlib2 gfxlib3 sfxlib
	$(TESTS_FBC_ENV) python3 "$(rootdir)/build_scripts/test-ustring.py" --root "$(rootdir)" --fbc "$(abspath $(FBC_EXE))"

##############################################################################
# END tests.mk
##############################################################################
