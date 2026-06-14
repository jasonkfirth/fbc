##############################################################################
# tests.mk
#
# Unit / log / warning test harness
##############################################################################

.PHONY: unit-tests log-tests warning-tests clean-tests

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
# END tests.mk
##############################################################################
