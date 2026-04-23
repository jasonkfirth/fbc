##############################################################################
# tests/bootstrap/dist.mk
#
# Bootstrap distribution generation tests
##############################################################################

.PHONY: bootstrap-dist-test
bootstrap-dist-test:
	$(call _mt_echo,Testing bootstrap distribution generation)
	@rm -f FreeBASIC-*.tar.xz 2>/dev/null || true
	@ARCHIVE="$$( $(call _mt_find_new_archives,$(MAKE) bootstrap-dist-target) )"; \
	echo "==> Produced archive: $$ARCHIVE"; \
	[ -n "$$ARCHIVE" ] || { echo "ERROR: bootstrap-dist-target did not produce an archive"; exit 1; }; \
	if tar -tf "$$ARCHIVE" | grep -E '(^|/)\.[^/]+/|(^|/)(stage|dist|out|packages|package-root[^/]*|pkgroot[^/]*)/'; then \
	        echo ""; \
	        echo "ERROR: bootstrap archive contains excluded generated directories"; \
	        exit 1; \
	fi
	$(call _mt_cleanup_success)

##############################################################################
# Bootstrap rebuild test
#
# IMPORTANT:
# The bootstrap tarball naming uses the *packaging* identity (FBPACK_DIR),
# not the runtime identity (FBTARGET / FBC_TARGET). So we must search by
# FBPACK_DIR here.
##############################################################################

.PHONY: bootstrap-rebuild-test
bootstrap-rebuild-test:
	$(call _mt_echo,Testing rebuild from bootstrap tarball)

	@rm -f FreeBASIC-*.tar.xz 2>/dev/null || true; \
	ARCHIVE="$$( $(call _mt_find_new_archives,$(MAKE) bootstrap-dist-target) )"; \
	[ -n "$$ARCHIVE" ] || { echo "ERROR: bootstrap-dist-target did not produce an archive"; exit 1; }; \
	echo "==> Using archive: $$ARCHIVE"; \
	rm -rf "$(TEST_TMP)/bootstrap-rebuild"; \
	mkdir -p "$(TEST_TMP)/bootstrap-rebuild"; \
	tar -xf "$$ARCHIVE" -C "$(TEST_TMP)/bootstrap-rebuild"; \
	SRCDIR=$$(tar -tf "$$ARCHIVE" | head -n1 | cut -d/ -f1); \
	[ -n "$$SRCDIR" ] || { echo "ERROR: could not determine extracted top-level directory"; exit 1; }; \
	cd "$(TEST_TMP)/bootstrap-rebuild/$$SRCDIR" && \
	$(MAKE) bootstrap-minimal && \
	$(MAKE) compiler
	$(call _mt_cleanup_success)

##############################################################################
# end of tests/bootstrap/dist.mk
##############################################################################
