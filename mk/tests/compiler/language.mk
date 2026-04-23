##############################################################################
# tests/compiler/language.mk
#
# FreeBASIC language test suite integration
##############################################################################

.PHONY: tests-test
tests-test:
	$(call _mt_echo,Running language tests)
	@mkdir -p $(LOG_DIR)
	@set -e; \
	status=0; \
	$(MAKE) unit-tests    > $(LOG_DIR)/unit-tests.log 2>&1 || status=1; \
	$(MAKE) log-tests     > $(LOG_DIR)/log-tests.log 2>&1 || status=1; \
	$(MAKE) warning-tests > $(LOG_DIR)/warning-tests.log 2>&1 || status=1; \
	if [ "$$status" -ne 0 ]; then \
		echo "ERROR: one or more language test suites failed"; \
		exit $$status; \
	fi

##############################################################################
# end of tests/compiler/language.mk
##############################################################################
