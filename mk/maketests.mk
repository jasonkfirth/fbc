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
include $(mkpath)/tests/compiler/structure.mk
include $(mkpath)/tests/compiler/semantic.mk

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
# These aggregate workflows include clean and bootstrap operations that mutate
# shared build products. Keep their prerequisite sequence serial under -j.
.NOTPARALLEL: quick-test-body full-test-body
quick-test: quick-test-body
	@$(MAKE) clean-maketests-success
	@$(MAKE) clean-maketests-host

quick-test-body: sanity \
	mk-structure-test \
	compiler-structure-test \
	compiler-host-policy-test \
	compiler-storage-test \
	compiler-test-harness-test \
	compiler-backends-test \
	compiler-semantic-model-test \
	example-artifact-test \
	build-graph-test \
	debian-include-test \
	bootstrap-emit-test \
	compiler-smoke \
	compiler-semantic-model-smoke \
	compiler-indirect-goto-smoke \
	compiler-riscv64-smoke \
	compiler-s390x-smoke \
	compiler-loongarch64-smoke \
	compiler-mips-smoke \
	compiler-ppc-smoke \
	compiler-ppc64-smoke \
	compiler-ppc64le-smoke \
	compiler-riscos-smoke

.PHONY: full-test full-test-body
full-test: full-test-body
	@$(MAKE) clean-maketests-success
	@$(MAKE) clean-maketests-host

full-test-body: sanity \
	mk-structure-test \
	compiler-structure-test \
	compiler-host-policy-test \
	compiler-storage-test \
	compiler-test-harness-test \
	compiler-backends-test \
	compiler-semantic-model-test \
	example-artifact-test \
	build-graph-test \
	debian-include-test \
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
	compiler-semantic-model-smoke \
	compiler-indirect-goto-smoke \
	compiler-riscv64-smoke \
	compiler-s390x-smoke \
	compiler-loongarch64-smoke \
	compiler-mips-smoke \
	compiler-ppc-smoke \
	compiler-ppc64-smoke \
	compiler-ppc64le-smoke \
	compiler-riscos-smoke \
	tests-test

MAKETEST_HOST_FBC_TARGETS := \
	sanity \
	mk-structure-test \
	compiler-structure-test \
	compiler-host-policy-test \
	compiler-storage-test \
	compiler-test-harness-test \
	compiler-backends-test \
	compiler-semantic-model-test \
	compiler-semantic-corpus-test \
	compiler-semantic-self-test \
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
	compiler-semantic-model-smoke \
	compiler-indirect-goto-smoke \
	compiler-riscv64-smoke \
	compiler-s390x-smoke \
	compiler-loongarch64-smoke \
	compiler-mips-smoke \
	compiler-ppc-smoke \
	compiler-ppc64-smoke \
	compiler-ppc64le-smoke \
	compiler-riscos-smoke \
	tests-test

$(MAKETEST_HOST_FBC_TARGETS): | maketests-preserve-host-fbc

# Compiler smoke checks can be requested together with -j. Each owns a separate
# directory so one check's successful cleanup cannot remove another's input.
$(filter compiler-%,$(MAKETEST_HOST_FBC_TARGETS)): private TEST_TMP = .maketests-tmp/$@

##############################################################################
# end of mk/maketests.mk
##############################################################################
