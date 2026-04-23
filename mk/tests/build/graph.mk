##############################################################################
# tests/build/graph.mk
#
# Build graph verification tests
##############################################################################

.PHONY: build-graph-test
build-graph-test: | prereqs
	$(call _mt_echo,Testing independent build targets)
	$(call _mt_run,$(MAKE) test-clean)
	$(call _mt_run,$(MAKE) compiler)
	$(call _mt_run,$(MAKE) rtlib)
	$(call _mt_run,$(MAKE) fbrt)
	$(call _mt_run,$(MAKE) gfxlib2)

##############################################################################
# Parallel build verification
##############################################################################

.PHONY: parallel-build-test
parallel-build-test:
	$(call _mt_echo,Parallel build)
	$(call _mt_run,$(MAKE) test-clean)
	$(call _mt_run,$(MAKE) -j4 all)

##############################################################################
# Rebuild correctness test
##############################################################################

.PHONY: rebuild-test
rebuild-test:
	$(call _mt_echo,Rebuild correctness)
	$(call _mt_run,$(MAKE) test-clean)
	$(call _mt_run,$(MAKE) all)
	$(call _mt_run,$(MAKE) all)

##############################################################################
# Build configuration matrix test
##############################################################################

.PHONY: matrix-test
matrix-test:
	$(call _mt_echo,Build matrix test)
	$(call _mt_run,$(MAKE) test-clean)
	$(call _mt_run,$(MAKE) all ENABLE_STANDALONE=1)
	$(call _mt_run,$(MAKE) test-clean)
	$(call _mt_run,$(MAKE) all ENABLE_LIB64=1)

##############################################################################
# end of tests/build/graph.mk
##############################################################################
