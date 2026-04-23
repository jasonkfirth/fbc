##############################################################################
# tests/bootstrap/stage.mk
#
# Bootstrap stage comparison test (GCC-style)
##############################################################################

.PHONY: bootstrap-stage-test
bootstrap-stage-test:
	$(call _mt_echo,Bootstrap stage comparison test)

	@set -e; \
	STAGE_DIR="$(SRC_ROOT)/$(TEST_TMP)/stage-test"; \
	rm -rf "$$STAGE_DIR"; \
	mkdir -p "$$STAGE_DIR"; \
	mkdir -p "$$STAGE_DIR/../lib/freebasic/$(FBTARGET)"; \
	cp -a "$(SRC_ROOT)/lib/freebasic/$(FBTARGET)/." "$$STAGE_DIR/../lib/freebasic/$(FBTARGET)/"; \
	echo "==> Building stage1 compiler"; \
	$(MAKE) -C "$(SRC_ROOT)" compiler; \
	cp "$(SRC_ROOT)/$(FBC_EXE)" "$$STAGE_DIR/stage1-fbc"; \
	echo "==> Building stage2 compiler using stage1"; \
	$(MAKE) -C "$(SRC_ROOT)" clean-compiler; \
	$(MAKE) -C "$(SRC_ROOT)" compiler BUILD_FBC="$$STAGE_DIR/stage1-fbc"; \
	cp "$(SRC_ROOT)/$(FBC_EXE)" "$$STAGE_DIR/stage2-fbc"; \
	echo "==> Building stage3 compiler using stage2"; \
	$(MAKE) -C "$(SRC_ROOT)" clean-compiler; \
	$(MAKE) -C "$(SRC_ROOT)" compiler BUILD_FBC="$$STAGE_DIR/stage2-fbc"; \
	cp "$(SRC_ROOT)/$(FBC_EXE)" "$$STAGE_DIR/stage3-fbc"; \
	echo "==> Comparing stage2 and stage3 compilers"; \
	if cmp -s "$$STAGE_DIR/stage2-fbc" "$$STAGE_DIR/stage3-fbc"; then \
	        echo "==> Bootstrap comparison PASSED"; \
	else \
	        echo ""; \
	        echo "ERROR: bootstrap comparison FAILED"; \
	        echo "stage2 and stage3 compilers differ"; \
	        exit 1; \
	fi
	$(call _mt_cleanup_success)

##############################################################################
# end of tests/bootstrap/stage.mk
##############################################################################
