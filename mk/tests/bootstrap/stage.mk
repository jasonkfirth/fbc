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
	export SOURCE_DATE_EPOCH="$${SOURCE_DATE_EPOCH:-1}"; \
	echo "==> Building stage1 compiler"; \
	$(TEST_TOOLCHAIN_ENV) $(MAKE) -C "$(SRC_ROOT)" compiler; \
	cp "$(SRC_ROOT)/$(FBC_EXE)" "$$STAGE_DIR/stage1-fbc"; \
	echo "==> Building stage2 compiler using stage1"; \
	$(TEST_TOOLCHAIN_ENV) $(MAKE) -C "$(SRC_ROOT)" clean-compiler; \
	$(TEST_TOOLCHAIN_ENV) $(MAKE) -C "$(SRC_ROOT)" compiler BUILD_FBC="$$STAGE_DIR/stage1-fbc"; \
	cp "$(SRC_ROOT)/$(FBC_EXE)" "$$STAGE_DIR/stage2-fbc"; \
	echo "==> Building stage3 compiler using stage2"; \
	$(TEST_TOOLCHAIN_ENV) $(MAKE) -C "$(SRC_ROOT)" clean-compiler; \
	$(TEST_TOOLCHAIN_ENV) $(MAKE) -C "$(SRC_ROOT)" compiler BUILD_FBC="$$STAGE_DIR/stage2-fbc"; \
	cp "$(SRC_ROOT)/$(FBC_EXE)" "$$STAGE_DIR/stage3-fbc"; \
	echo "==> Comparing stage2 and stage3 compilers"; \
	if cmp -s "$$STAGE_DIR/stage2-fbc" "$$STAGE_DIR/stage3-fbc"; then \
	        echo "==> Bootstrap comparison PASSED"; \
	else \
	        strip_tool="$${STRIP:-strip}"; \
	        stripped_stage2="$$STAGE_DIR/stage2-fbc.stripped"; \
	        stripped_stage3="$$STAGE_DIR/stage3-fbc.stripped"; \
	        cp "$$STAGE_DIR/stage2-fbc" "$$stripped_stage2"; \
	        cp "$$STAGE_DIR/stage3-fbc" "$$stripped_stage3"; \
	        if ! "$$strip_tool" --strip-all "$$stripped_stage2" "$$stripped_stage3"; then \
	                echo ""; \
	                echo "ERROR: bootstrap comparison could not normalize compiler binaries"; \
	                echo "strip tool failed: $$strip_tool"; \
	                exit 1; \
	        fi; \
	        if cmp -s "$$stripped_stage2" "$$stripped_stage3"; then \
	                echo "==> Bootstrap comparison PASSED"; \
	                echo "==> Non-runtime symbol metadata differs"; \
	                echo "==> Stripped compiler images match"; \
	        else \
	                echo ""; \
	                echo "ERROR: bootstrap comparison FAILED"; \
	                echo "stage2 and stage3 compilers differ"; \
	                exit 1; \
	        fi; \
	fi
	$(call _mt_cleanup_success)

##############################################################################
# end of tests/bootstrap/stage.mk
##############################################################################
