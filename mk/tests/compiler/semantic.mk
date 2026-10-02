##############################################################################
# Project: FreeBASIC compiler tests
# File: tests/compiler/semantic.mk
# Purpose: Run the semantic sidecar contract and compiler regression suite.
# Responsibilities: Build the compiler and invoke tests with private artifacts.
# This file intentionally does NOT contain: fixtures or sidecar parsing.
##############################################################################

PYTHON ?= python3
COMPILER_SEMANTIC_SELF_FLAGS ?=
COMPILER_SEMANTIC_PROJECTS_FLAGS ?=

.PHONY: compiler-semantic-model-test compiler-semantic-corpus-test compiler-semantic-self-test compiler-semantic-projects-test
compiler-semantic-model-test: compiler | maybe-build-fbc $(RTL_LIBS)
	$(if $(CAN_RUN),,$(call _mt_echo,Skipping semantic sidecar execution for a cross build))
ifneq ($(CAN_RUN),)
	$(FBC_TOOL_ENV) $(PYTHON) "$(rootdir)/build_scripts/test-compiler-semantic-model.py" --root "$(rootdir)" --fbc "$(abspath $(FBC_EXE))"
endif

compiler-semantic-corpus-test: compiler | maybe-build-fbc $(RTL_LIBS)
	$(if $(CAN_RUN),,$(call _mt_echo,Skipping semantic corpus execution for a cross build))
ifneq ($(CAN_RUN),)
	$(FBC_TOOL_ENV) $(PYTHON) "$(rootdir)/build_scripts/test-compiler-semantic-corpus.py" --root "$(rootdir)" --fbc "$(abspath $(FBC_EXE))"
endif

compiler-semantic-self-test: compiler | maybe-build-fbc $(RTL_LIBS)
	$(if $(CAN_RUN),,$(call _mt_echo,Skipping compiler semantic audit execution for a cross build))
ifneq ($(CAN_RUN),)
	$(FBC_TOOL_ENV) $(PYTHON) "$(rootdir)/build_scripts/test-compiler-semantic-self.py" --root "$(rootdir)" --fbc "$(abspath $(FBC_EXE))" $(COMPILER_SEMANTIC_SELF_FLAGS)
endif

# External project builds are captured separately. Replaying their snapshot
# preserves the GCC controls, project options, and generated BASIC inputs.
compiler-semantic-projects-test:
	$(PYTHON) "$(rootdir)/build_scripts/test-compiler-semantic-projects.py" $(COMPILER_SEMANTIC_PROJECTS_FLAGS)

##############################################################################
# end of tests/compiler/semantic.mk
##############################################################################
