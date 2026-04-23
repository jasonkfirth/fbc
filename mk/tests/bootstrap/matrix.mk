##############################################################################
# tests/bootstrap/matrix.mk
#
# Multi-architecture bootstrap distribution matrix test
##############################################################################

BOOTSTRAP_TEST_ARCHES := \
	linux-amd64 \
	linux-i386 \
	linux-arm64 \
	linux-armhf \
	linux-armel \
	linux-powerpc \
	linux-powerpc64 \
	linux-ppc64el \
	linux-riscv64 \
	linux-s390x \
	linux-loongarch64 \
	freebsd-amd64 \
	freebsd-i386 \
	freebsd-arm64 \
	freebsd-powerpc \
	freebsd-powerpc64 \
	darwin-x86_64 \
	darwin-aarch64 \
	haiku-x86_64 \
	haiku-x86 \
	cygwin-x86_64 \
	cygwin-x86 \
	mingw-x86_64 \
	mingw-x86

.PHONY: bootstrap-dist-matrix-test
bootstrap-dist-matrix-test:
	$(call _mt_echo,Testing bootstrap distribution matrix)

	rm -f FreeBASIC-*.tar.xz 2>/dev/null || true
	mkdir -p "$(TEST_TMP)"

	@set -e; \
	out="$(TEST_TMP)/bootstrap-dist-matrix.out"; \
	before="$(TEST_TMP)/archives.before"; \
	after="$(TEST_TMP)/archives.after"; \
	new="$(TEST_TMP)/archives.new"; \
	forbidden='(^|/)\.[^/]+/|(^|/)(stage|dist|out|packages|package-root[^/]*|pkgroot[^/]*)/'; \
	: > "$$out"; \
	for arch in $(BOOTSTRAP_TEST_ARCHES); do \
	        echo "==> Building bootstrap distribution for $$arch"; \
	        ls -1 FreeBASIC-*.tar.xz 2>/dev/null | sort > "$$before" || true; \
	        $(MAKE) bootstrap-dist-target FBTARGET_DIR_OVERRIDE=$$arch; \
	        ls -1 FreeBASIC-*.tar.xz 2>/dev/null | sort > "$$after" || true; \
	        comm -13 "$$before" "$$after" > "$$new" || true; \
	        ARCHIVE=$$(head -n1 "$$new"); \
	        if [ -z "$$ARCHIVE" ]; then \
	                echo ""; \
	                echo "ERROR: no archive produced for $$arch"; \
	                echo ""; \
	                ls -1; \
	                exit 1; \
	        fi; \
	        if tar -tf "$$ARCHIVE" | grep -E "$$forbidden"; then \
	                echo ""; \
	                echo "ERROR: bootstrap archive $$ARCHIVE contains excluded generated directories"; \
	                exit 1; \
	        fi; \
	        echo "$$arch -> $$ARCHIVE" | tee -a "$$out"; \
	done; \
	echo ""; \
	echo "==> Matrix results:"; \
	cat "$$out"
	$(call _mt_cleanup_success)

.PHONY: bootstrap-emit-matrix-test
bootstrap-emit-matrix-test:
	$(call _mt_echo,Testing bootstrap emission matrix)
	@rm -rf bootstrap/linux-emit-matrix-a
	$(call _mt_run,$(MAKE) bootstrap-emit-matrix BOOTSTRAP_MATRIX='linux-emit-matrix-a')
	@test -d bootstrap/linux-emit-matrix-a || { echo "ERROR: bootstrap/linux-emit-matrix-a missing"; exit 1; }
	@find bootstrap/linux-emit-matrix-a -type f \( -name "*.c" -o -name "*.asm" \) | grep -q . || { echo "ERROR: bootstrap/linux-emit-matrix-a sources missing"; exit 1; }
	@rm -rf bootstrap/linux-emit-matrix-a
	$(call _mt_cleanup_success)

##############################################################################
# end of tests/bootstrap/matrix.mk
##############################################################################
