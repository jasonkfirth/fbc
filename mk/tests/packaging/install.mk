##############################################################################
# tests/packaging/install.mk
#
# Install / uninstall verification tests
##############################################################################

TEST_STAGE_BINDIR := $(prefixbindir)
ifneq ($(filter win32 dos,$(TARGET_OS)),)
TEST_STAGE_BINDIR := $(patsubst $(prefix)%,%,$(prefixbindir))
endif

.PHONY: install-test
install-test:
	$(call _mt_echo,Testing DESTDIR install)
	@rm -rf "$(TEST_TMP)"
	@mkdir -p "$(TEST_TMP)"
	@test -x "$(FBC_EXE)" || { echo "ERROR: compiler missing before install-test"; exit 1; }
	$(call _mt_run,$(MAKE) DESTDIR=$(TEST_TMP) install)
	@test -f "$(TEST_TMP)$(TEST_STAGE_BINDIR)/fbc$(EXEEXT)" || { echo "Install failed"; exit 1; }

##############################################################################
# Manifest uninstall verification
##############################################################################

.PHONY: uninstall-test
uninstall-test:
	$(call _mt_echo,Testing manifest uninstall)
	$(call _mt_run,$(MAKE) DESTDIR=$(TEST_TMP) uninstall)
	@test ! -f "$(TEST_TMP)$(TEST_STAGE_BINDIR)/fbc$(EXEEXT)" || { echo "ERROR: uninstall left compiler binary behind"; exit 1; }
	@test ! -f "$(TEST_TMP).install_manifest" || { echo "ERROR: uninstall left install manifest behind"; exit 1; }
	$(call _mt_cleanup_success)

##############################################################################
# end of tests/packaging/install.mk
##############################################################################
