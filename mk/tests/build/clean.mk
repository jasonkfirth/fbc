##############################################################################
# tests/build/clean.mk
#
# Clean rule verification tests
##############################################################################

.PHONY: clean-test
clean-test:
	$(call _mt_echo,Testing clean targets)
	$(call _mt_run,$(MAKE) all)
	$(call _mt_run,$(MAKE) clean)
	@test ! -d "$(fbcobjdir)" || { echo "ERROR: compiler objects not cleaned"; exit 1; }
	@test ! -e "$(FBC_EXE)" || { echo "ERROR: compiler binary not cleaned"; exit 1; }
	@test ! -e "$(libdir)/libfb.a" || { echo "ERROR: runtime archive not cleaned"; exit 1; }

##############################################################################
# Clean idempotence verification
##############################################################################

.PHONY: clean-idempotence
clean-idempotence:
	$(call _mt_echo,Clean idempotence)
	$(call _mt_run,$(MAKE) test-clean)
	$(call _mt_run,$(MAKE) test-clean)

##############################################################################
# end of tests/build/clean.mk
##############################################################################
