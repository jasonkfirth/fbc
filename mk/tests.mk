##############################################################################
# tests.mk
#
# Unit / log / warning test harness
##############################################################################

.PHONY: unit-tests log-tests warning-tests clean-tests

TESTS_FBC := $(if $(LOCAL_FBC),$(abspath $(LOCAL_FBC)),$(AVAILABLE_FBC))
TESTS_TOOLCHAIN_BINDIR :=
ifneq ($(findstring /,$(CC))$(findstring \,$(CC)),)
TESTS_TOOLCHAIN_BINDIR := $(patsubst %/,%,$(dir $(CC)))
endif
TESTS_FBC_ENV := env \
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

##############################################################################
# Unit tests
##############################################################################

unit-tests:
	@test -n "$(TESTS_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	cd tests && $(MAKE) unit-tests \
		FBC="$(TESTS_FBC_CMD) -i $(rootdir)/inc"

##############################################################################
# Log tests
##############################################################################

log-tests:
	@test -n "$(TESTS_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	cd tests && $(MAKE) log-tests \
		FBC="$(TESTS_FBC_CMD) -i $(rootdir)/inc"

##############################################################################
# Warning tests
##############################################################################

warning-tests:
	@test -n "$(TESTS_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	@chmod +x tests/warnings/test.sh 2>/dev/null || true
	cd tests/warnings && \
		FBC="$(TESTS_FBC_CMD)" ./test.sh

##############################################################################
# END tests.mk
##############################################################################
