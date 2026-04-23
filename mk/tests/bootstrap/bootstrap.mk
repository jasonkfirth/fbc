##############################################################################
# tests/bootstrap/bootstrap.mk
#
# Bootstrap build verification tests
##############################################################################

.PHONY: bootstrap-test
bootstrap-test:
	$(call _mt_echo,Bootstrap build)
	$(call _mt_run,$(MAKE) test-clean)
	$(call _mt_run,$(MAKE) bootstrap)
	$(call _mt_run,$(MAKE) compiler BUILD_FBC=$(FBC_EXE))

##############################################################################
# Bootstrap emission verification
##############################################################################

.PHONY: bootstrap-emit-test
bootstrap-emit-test:
	$(call _mt_echo,Testing bootstrap emission)
	$(call _mt_run,$(MAKE) bootstrap-emit)
	@test -d bootstrap/$(BOOTSTRAP_DIR) || { echo "ERROR: bootstrap directory missing"; exit 1; }
	@find bootstrap/$(BOOTSTRAP_DIR) -type f \( -name "*.c" -o -name "*.asm" \) | grep -q . || { echo "ERROR: bootstrap sources missing"; exit 1; }

##############################################################################
# end of tests/bootstrap/bootstrap.mk
##############################################################################
