##############################################################################
# tests/packaging/packaging.mk
#
# Packaging verification tests
##############################################################################

.PHONY: packaging-test
packaging-test:
	$(call _mt_echo,Testing binary distribution)
	$(call _mt_run,$(MAKE) dist)
	@test -f freebasic-dist.tar || { echo "ERROR: dist did not produce freebasic-dist.tar"; exit 1; }

##############################################################################
# Tar / zip packaging helpers
##############################################################################

.PHONY: pkg-test
pkg-test:
	$(call _mt_echo,Testing packaging helpers)
	$(call _mt_run,$(MAKE) pkg-tar)
	$(call _mt_run,$(MAKE) pkg-zip)

##############################################################################
# end of tests/packaging/packaging.mk
##############################################################################
