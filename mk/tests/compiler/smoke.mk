##############################################################################
# tests/compiler/smoke.mk
#
# Compiler smoke test
##############################################################################

.PHONY: compiler-smoke compiler-riscv64-smoke compiler-s390x-smoke compiler-loongarch64-smoke compiler-ppc64le-smoke
compiler-smoke: libs
	$(call _mt_echo,Compiler smoke test)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" 'print "ok"' > "$(TEST_TMP)/smoke.bas"
	$(call _mt_run,$(TEST_FBC_CMD) "$(TEST_TMP)/smoke.bas" -x "$(TEST_TMP)/smoke$(EXEEXT)")
ifneq ($(CAN_RUN),)
	@./"$(TEST_TMP)/smoke$(EXEEXT)" >/dev/null 2>&1 && echo "==> RUN OK"
endif
	$(call _mt_cleanup_success)

compiler-riscv64-smoke:
	@test -n "$(TEST_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	$(call _mt_echo,RISC-V 64 compiler target smoke test)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" \
		'#if not defined(__FB_LINUX__)' \
		'#error expected linux target' \
		'#endif' \
		'#if not defined(__FB_RISCV64__)' \
		'#error expected riscv64 target' \
		'#endif' \
		'print "riscv64 ok"' \
		> "$(TEST_TMP)/riscv64-smoke.bas"
	$(call _mt_run,$(TEST_FBC_CMD) -target riscv64-linux-gnu -r "$(TEST_TMP)/riscv64-smoke.bas" -x "$(TEST_TMP)/riscv64-smoke")
	@test -s "$(TEST_TMP)/riscv64-smoke.c" || { echo "ERROR: riscv64 C output was not produced"; exit 1; }
	@if command -v riscv64-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "==> riscv64-linux-gnu-gcc found; compiling riscv64 object"; \
		$(TEST_FBC_CMD) -target riscv64-linux-gnu -c "$(TEST_TMP)/riscv64-smoke.bas" -o "$(TEST_TMP)/riscv64-smoke.o"; \
		readelf -h "$(TEST_TMP)/riscv64-smoke.o" | grep -q 'Machine:.*RISC-V' || { echo "ERROR: object is not RISC-V"; exit 1; }; \
		echo "==> RISCV64 OBJECT OK"; \
	else \
		echo "==> SKIP: riscv64-linux-gnu-gcc not found; target C emission only"; \
	fi
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)"

compiler-s390x-smoke:
	@test -n "$(TEST_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	$(call _mt_echo,S390X compiler target smoke test)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" \
		'#if not defined(__FB_LINUX__)' \
		'#error expected linux target' \
		'#endif' \
		'#if not defined(__FB_S390X__)' \
		'#error expected s390x target' \
		'#endif' \
		'print "s390x ok"' \
		> "$(TEST_TMP)/s390x-smoke.bas"
	$(call _mt_run,$(TEST_FBC_CMD) -target s390x-linux-gnu -r "$(TEST_TMP)/s390x-smoke.bas" -x "$(TEST_TMP)/s390x-smoke")
	@test -s "$(TEST_TMP)/s390x-smoke.c" || { echo "ERROR: s390x C output was not produced"; exit 1; }
	@mkdir -p "$(TEST_TMP)/fakebin"
	@printf "%s\n" \
		'#!/bin/sh' \
		'printf "%s\n" "$$@" > "$(SRC_ROOT)/$(TEST_TMP)/s390x-gcc.args"' \
		'exit 1' \
		> "$(TEST_TMP)/fakebin/s390x-linux-gnu-gcc"
	@chmod +x "$(TEST_TMP)/fakebin/s390x-linux-gnu-gcc"
	@PATH="$(SRC_ROOT)/$(TEST_TMP)/fakebin:$$PATH" "$(TEST_FBC)" -target s390x-linux-gnu -c "$(TEST_TMP)/s390x-smoke.bas" -o "$(TEST_TMP)/s390x-smoke.o" >/dev/null 2>&1 || test -s "$(TEST_TMP)/s390x-gcc.args"
	@grep -q -- '-march=z900' "$(TEST_TMP)/s390x-gcc.args" || { echo "ERROR: s390x gcc command did not use -march=z900"; exit 1; }
	@! grep -q -- '-march=s390x' "$(TEST_TMP)/s390x-gcc.args" || { echo "ERROR: s390x gcc command used invalid -march=s390x"; exit 1; }
	@if command -v s390x-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "==> s390x-linux-gnu-gcc found; compiling s390x object"; \
		$(TEST_FBC_CMD) -target s390x-linux-gnu -c "$(TEST_TMP)/s390x-smoke.bas" -o "$(TEST_TMP)/s390x-smoke.o"; \
		readelf -h "$(TEST_TMP)/s390x-smoke.o" | grep -q 'Machine:.*IBM S/390' || { echo "ERROR: object is not S390"; exit 1; }; \
		echo "==> S390X OBJECT OK"; \
	else \
		echo "==> SKIP: s390x-linux-gnu-gcc not found; target C emission only"; \
	fi
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)"

compiler-loongarch64-smoke:
	@test -n "$(TEST_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	$(call _mt_echo,LoongArch64 compiler target smoke test)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" \
		'#if not defined(__FB_LINUX__)' \
		'#error expected linux target' \
		'#endif' \
		'#if not defined(__FB_LOONGARCH64__)' \
		'#error expected loongarch64 target' \
		'#endif' \
		'print "loongarch64 ok"' \
		> "$(TEST_TMP)/loongarch64-smoke.bas"
	$(call _mt_run,$(TEST_FBC_CMD) -target loongarch64-linux-gnu -r "$(TEST_TMP)/loongarch64-smoke.bas" -x "$(TEST_TMP)/loongarch64-smoke")
	@test -s "$(TEST_TMP)/loongarch64-smoke.c" || { echo "ERROR: loongarch64 C output was not produced"; exit 1; }
	@if command -v loongarch64-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "==> loongarch64-linux-gnu-gcc found; compiling loongarch64 object"; \
		$(TEST_FBC_CMD) -target loongarch64-linux-gnu -c "$(TEST_TMP)/loongarch64-smoke.bas" -o "$(TEST_TMP)/loongarch64-smoke.o"; \
		readelf -h "$(TEST_TMP)/loongarch64-smoke.o" | grep -q 'Machine:.*LoongArch' || { echo "ERROR: object is not LoongArch"; exit 1; }; \
		echo "==> LOONGARCH64 OBJECT OK"; \
	else \
		echo "==> SKIP: loongarch64-linux-gnu-gcc not found; target C emission only"; \
	fi
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)"

compiler-ppc64le-smoke:
	@test -n "$(TEST_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	$(call _mt_echo,PowerPC64LE compiler target smoke test)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" \
		'#if not defined(__FB_LINUX__)' \
		'#error expected linux target' \
		'#endif' \
		'#if not defined(__FB_PPC__)' \
		'#error expected ppc target' \
		'#endif' \
		'#if not defined(__FB_64BIT__)' \
		'#error expected 64-bit target' \
		'#endif' \
		'print "ppc64le ok"' \
		> "$(TEST_TMP)/ppc64le-smoke.bas"
	$(call _mt_run,$(TEST_FBC_CMD) -target powerpc64le-linux-gnu -r "$(TEST_TMP)/ppc64le-smoke.bas" -x "$(TEST_TMP)/ppc64le-smoke")
	@test -s "$(TEST_TMP)/ppc64le-smoke.c" || { echo "ERROR: ppc64le C output was not produced"; exit 1; }
	@if command -v powerpc64le-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "==> powerpc64le-linux-gnu-gcc found; compiling ppc64le object"; \
		$(TEST_FBC_CMD) -target powerpc64le-linux-gnu -c "$(TEST_TMP)/ppc64le-smoke.bas" -o "$(TEST_TMP)/ppc64le-smoke.o"; \
		readelf -h "$(TEST_TMP)/ppc64le-smoke.o" | grep -q 'Machine:.*PowerPC64' || { echo "ERROR: object is not PowerPC64"; exit 1; }; \
		echo "==> PPC64LE OBJECT OK"; \
	else \
		echo "==> SKIP: powerpc64le-linux-gnu-gcc not found; target C emission only"; \
	fi
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)"

##############################################################################
# end of tests/compiler/smoke.mk
##############################################################################
