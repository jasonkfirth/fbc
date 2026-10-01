##############################################################################
# FreeBASIC compiler structure tests
#
# File: tests/compiler/structure.mk
#
# Purpose: Validate compiler subsystem coverage and host numeric policies.
# Responsibilities: Connect source graph and internal policy tests to make.
# This file intentionally does NOT define compiler discovery or compile rules.
##############################################################################

PYTHON ?= python3

.PHONY: compiler-structure-test compiler-host-policy-test compiler-test-harness-test compiler-storage-test compiler-backends-test
compiler-structure-test:
	$(PYTHON) "$(rootdir)/build_scripts/check-compiler-structure.py" --root "$(rootdir)"

compiler-host-policy-test: | maybe-build-fbc $(RTL_LIBS)
	$(if $(CAN_RUN),,$(call _mt_echo,Skipping host policy execution for a cross build))
ifneq ($(CAN_RUN),)
	$(FBC_TOOL_ENV) $(PYTHON) "$(rootdir)/build_scripts/test-compiler-host-policy.py" --root "$(rootdir)" --fbc "$(abspath $(FBC_EXE))"
endif

compiler-test-harness-test: | maybe-build-fbc
	$(FBC_TOOL_ENV) $(PYTHON) "$(rootdir)/build_scripts/test-compiler-test-harness.py" --root "$(rootdir)" --fbc "$(TEST_FBC)"

compiler-storage-test: | maybe-build-fbc $(RTL_LIBS)
	$(if $(CAN_RUN),,$(call _mt_echo,Skipping compiler storage execution for a cross build))
ifneq ($(CAN_RUN),)
	$(FBC_TOOL_ENV) $(PYTHON) "$(rootdir)/build_scripts/test-compiler-storage.py" --root "$(rootdir)" --fbc "$(abspath $(FBC_EXE))"
endif

compiler-backends-test: | maybe-build-fbc $(RTL_LIBS)
	$(if $(CAN_RUN),,$(call _mt_echo,Skipping backend execution for a cross build))
ifneq ($(CAN_RUN),)
	$(FBC_TOOL_ENV) $(PYTHON) "$(rootdir)/build_scripts/test-compiler-backends.py" --root "$(rootdir)" --fbc "$(abspath $(FBC_EXE))"
endif

##############################################################################
# end of tests/compiler/structure.mk
##############################################################################
