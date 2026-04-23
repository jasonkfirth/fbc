##############################################################################
# tests/build/example-artifacts.mk
#
# Detect generated example outputs that should not live in the source tree.
#
# The examples/ tree intentionally contains some hand-written C files for
# interop documentation. Those are valid sources and must not be deleted.
# This test only flags files whose stem matches a sibling .bas or .bi source,
# which is the pattern produced by example builds.
##############################################################################

.PHONY: example-artifact-test
example-artifact-test:
	$(call _mt_echo,Checking examples/ for generated build artifacts)
	@artifacts="$$( $(MAKE) --no-print-directory -s list-example-artifacts )"; \
	if [ -n "$$artifacts" ]; then \
		echo "ERROR: generated example artifacts present in examples/"; \
		printf '%s\n' "$$artifacts"; \
		exit 1; \
	fi
	$(call _mt_echo,examples/ tree is clean)

##############################################################################
# end of tests/build/example-artifacts.mk
##############################################################################
