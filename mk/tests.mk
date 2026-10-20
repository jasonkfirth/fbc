##############################################################################
# tests.mk
#
# Unit / log / warning test harness
##############################################################################

.PHONY: unit-tests log-tests warning-tests clean-tests ustring-test serialization-test jni-test big-int-test quicklz-test

TESTS_FBC := $(if $(LOCAL_FBC),$(abspath $(LOCAL_FBC)),$(AVAILABLE_FBC))
TESTS_TOOLCHAIN_BINDIR := $(call tool_bindir,$(CC))
# C++ fixtures link Apple's libc++, which is available from macOS 10.7.
# Keep every test compiler stage on the same floor, including the FBC command
# whose toolchain environment would otherwise restore the older build target.
TESTS_DARWIN_ENV :=
ifeq ($(TARGET_OS),darwin)
export TESTS_DARWIN_DEPLOYMENT_TARGET ?= $(shell printf '%s\n' '$(DARWIN_DEPLOYMENT_TARGET)' | awk -F. '{ if (($$1+0 < 10) || ($$1+0 == 10 && $$2+0 < 7)) print "10.7"; else print; }')
TESTS_DARWIN_ENV := MACOSX_DEPLOYMENT_TARGET='$(TESTS_DARWIN_DEPLOYMENT_TARGET)'
endif

TESTS_FBC_ENV := env \
	$(TOOLCHAIN_FBC_ENV) \
	$(TESTS_DARWIN_ENV) \
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
		$(TESTS_FBC_ENV) FBC="$(TESTS_FBC)" bash ./test.sh

##############################################################################
# USTRING needs the compiler and runtime built from the same source tree.
# The runner keeps its unit library, examples, and sanitizer artifacts in a
# temporary directory, so it can be used alongside the normal test harness.
##############################################################################

ustring-test: compiler rtlib gfxlib2 gfxlib3 sfxlib
	$(TESTS_FBC_ENV) python3 "$(rootdir)/build_scripts/test-ustring.py" --root "$(rootdir)" --fbc "$(abspath $(FBC_EXE))"

# Logical serialization owns no transport and needs only compiler and rtlib.
serialization-test: compiler rtlib
	$(TESTS_FBC_ENV) python3 "$(rootdir)/build_scripts/test-serialization.py" --root "$(rootdir)" --fbc "$(abspath $(FBC_EXE))"

# JNI is an optional native SDK check. The JDK owns its platform headers and
# VM library; this target does not fetch or install one on the test machine.
jni-test: | maybe-build-fbc $(TESTS_RUNTIME_LIBS)
	@test -n "$(JAVA_HOME)" || { echo "ERROR: set JAVA_HOME to a native JDK"; exit 1; }
	$(TESTS_FBC_ENV) python3 "$(rootdir)/build_scripts/test-jni.py" --java-home "$(JAVA_HOME)" --fbc "$(TESTS_FBC)" --cc "$(CC)"

# big_int is an optional release-matched C ABI check. Keep the historical
# SDK outside the installed compiler and supply its headers explicitly.
big-int-test: | maybe-build-fbc $(TESTS_RUNTIME_LIBS)
	@test -n "$(BIG_INT_HEADERS)" -a -n "$(BIG_INT_LIBDIR)" || { echo "ERROR: set BIG_INT_HEADERS and BIG_INT_LIBDIR"; exit 1; }
	$(TESTS_FBC_ENV) python3 "$(rootdir)/build_scripts/test-big-int.py" --headers "$(BIG_INT_HEADERS)" --libdir "$(BIG_INT_LIBDIR)" --fbc "$(TESTS_FBC)" --cc "$(CC)"

# QuickLZ state sizes depend on the source release and build settings. The
# native test checks the library's settings before lending it typed buffers.
quicklz-test: | maybe-build-fbc $(TESTS_RUNTIME_LIBS)
	@test -n "$(QUICKLZ_HEADERS)" -a -n "$(QUICKLZ_LIBDIR)" || { echo "ERROR: set QUICKLZ_HEADERS and QUICKLZ_LIBDIR"; exit 1; }
	$(TESTS_FBC_ENV) python3 "$(rootdir)/build_scripts/test-quicklz.py" --headers "$(QUICKLZ_HEADERS)" --libdir "$(QUICKLZ_LIBDIR)" --fbc "$(TESTS_FBC)" --cc "$(CC)"

# Guest runners consume the output names and retained resource paths from the
# host inventory. Check that contract without requiring an SDK or emulator.
.PHONY: exampleageddon-contract-test
quick-test full-test: exampleageddon-contract-test
exampleageddon-contract-test:
	$(PYTHON) "$(rootdir)/build_scripts/test-exampleageddon-contracts.py"

# Source ownership and platform replacements are checked without a cross SDK.
.PHONY: rtlib-structure-test
rtlib-structure-test:
	$(PYTHON) "$(rootdir)/build_scripts/check-rtlib-structure.py" --root "$(rootdir)" --make "$(MAKE)"

##############################################################################
# END tests.mk
##############################################################################
