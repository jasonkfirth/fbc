##############################################################################
# mk/maketests.mk
#
# Test harness entrypoint
#
# Loads all test modules from tests/
##############################################################################

##############################################################################
# Harness
##############################################################################

include $(mkpath)/tests/harness.mk

##############################################################################
# Build system tests
##############################################################################

include $(mkpath)/tests/build/structure.mk
include $(mkpath)/tests/build/example-artifacts.mk
include $(mkpath)/tests/build/graph.mk
include $(mkpath)/tests/build/clean.mk
include $(mkpath)/tests/build/dependency.mk

##############################################################################
# Bootstrap tests
##############################################################################

include $(mkpath)/tests/bootstrap/bootstrap.mk
include $(mkpath)/tests/bootstrap/stage.mk
include $(mkpath)/tests/bootstrap/dist.mk
include $(mkpath)/tests/bootstrap/matrix.mk

##############################################################################
# Packaging tests
##############################################################################

include $(mkpath)/tests/packaging/packaging.mk
include $(mkpath)/tests/packaging/install.mk

##############################################################################
# Compiler tests
##############################################################################

include $(mkpath)/tests/compiler/smoke.mk
include $(mkpath)/tests/compiler/language.mk

##############################################################################
# Sanity / configuration test
##############################################################################

.PHONY: sanity
sanity: | prereqs
	$(call _mt_echo,Build identity)
	@echo "TARGET_TRIPLET=$(TARGET_TRIPLET)"
	@echo "TARGET_OS=$(TARGET_OS)"
	@echo "TARGET_ARCH=$(TARGET_ARCH)"
	@echo "FBC_TARGET=$(FBC_TARGET)"
	@echo "FBC_EXE=$(FBC_EXE)"
	@echo "FBTARGET=$(FBTARGET)"
	@echo "FBTARGET_DIR=$(FBTARGET_DIR)"
	@echo "FBPACK_DIR=$(FBPACK_DIR)"
	$(call _mt_run,$(MAKE) print-config)

##############################################################################
# Meta targets
##############################################################################

.PHONY: quick-test quick-test-body
quick-test: quick-test-body
	@$(MAKE) clean-maketests-success

quick-test-body: sanity \
	mk-structure-test \
	example-artifact-test \
	build-graph-test \
	bootstrap-emit-test \
	compiler-smoke

.PHONY: full-test full-test-body
full-test: full-test-body
	@$(MAKE) clean-maketests-success

full-test-body: sanity \
	mk-structure-test \
	example-artifact-test \
	build-graph-test \
	parallel-build-test \
	clean-test \
	clean-idempotence \
	dependency-test \
	rebuild-test \
	bootstrap-test \
	bootstrap-emit-test \
	bootstrap-emit-matrix-test \
	bootstrap-dist-test \
	bootstrap-dist-matrix-test \
	bootstrap-rebuild-test \
	bootstrap-stage-test \
	packaging-test \
	pkg-test \
	install-test \
	uninstall-test \
	matrix-test \
	compiler-smoke \
	tests-test

##############################################################################
# end of mk/maketests.mk
##############################################################################
