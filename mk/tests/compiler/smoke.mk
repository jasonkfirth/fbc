##############################################################################
# tests/compiler/smoke.mk
#
# Compiler smoke test
##############################################################################

.PHONY: compiler-smoke compiler-indirect-goto-smoke compiler-riscv32-smoke compiler-riscv64-smoke compiler-s390x-smoke compiler-loongarch64-smoke compiler-mips-smoke compiler-ppc-smoke compiler-ppc64-smoke compiler-ppc64le-smoke compiler-riscos-smoke
compiler-smoke: libs
	$(call _mt_echo,Compiler smoke test)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" 'print "ok"' > "$(TEST_TMP)/smoke.bas"
	$(call _mt_run,$(TEST_FBC_CMD) "$(TEST_TMP)/smoke.bas" -x "$(TEST_TMP)/smoke$(EXEEXT)")
ifneq ($(CAN_RUN),)
	@./"$(TEST_TMP)/smoke$(EXEEXT)" >/dev/null 2>&1 && echo "==> RUN OK"
endif
	$(call _mt_cleanup_success)

compiler-indirect-goto-smoke:
	@test -n "$(TEST_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	$(call _mt_echo,Indirect goto C backend smoke test)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" \
		'sub probe()' \
		'    dim a() as integer' \
		'    redim a(0 to 15)' \
		'end sub' \
		'probe()' \
		> "$(TEST_TMP)/indirect-goto.bas"
	$(call _mt_run,$(TEST_FBC_CMD) -gen gcc -e -r "$(TEST_TMP)/indirect-goto.bas" -x "$(TEST_TMP)/indirect-goto")
	@grep -Fq 'goto *' "$(TEST_TMP)/indirect-goto.c" || { echo "ERROR: indirect goto C output was not produced"; exit 1; }
	@grep -Fq '_llvmbug18658' "$(TEST_TMP)/indirect-goto.c" || { echo "ERROR: clang indirect goto workaround missing"; exit 1; }
	$(call _mt_run,$(CC) -x c -c "$(TEST_TMP)/indirect-goto.c" -o "$(TEST_TMP)/indirect-goto.o")
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)"

compiler-riscv32-smoke:
	@test -n "$(TEST_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	$(call _mt_echo,RISC-V 32 compiler target smoke test)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" \
		'#if not defined(__FB_LINUX__)' \
		'#error expected linux target' \
		'#endif' \
		'#if not defined(__FB_RISCV32__)' \
		'#error expected riscv32 target' \
		'#endif' \
		'print "riscv32 ok"' \
		> "$(TEST_TMP)/riscv32-smoke.bas"
	$(call _mt_run,$(TEST_FBC_TRIPLET_CMD) -target riscv32-linux-gnu -r "$(TEST_TMP)/riscv32-smoke.bas" -x "$(TEST_TMP)/riscv32-smoke")
	@test -s "$(TEST_TMP)/riscv32-smoke.c" || { echo "ERROR: riscv32 C output was not produced"; exit 1; }
	@if command -v riscv32-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "==> riscv32-linux-gnu-gcc found; compiling riscv32 object"; \
		$(TEST_FBC_TRIPLET_CMD) -target riscv32-linux-gnu -c "$(TEST_TMP)/riscv32-smoke.bas" -o "$(TEST_TMP)/riscv32-smoke.o"; \
		readelf -h "$(TEST_TMP)/riscv32-smoke.o" | grep -q 'Machine:.*RISC-V' || { echo "ERROR: object is not RISC-V"; exit 1; }; \
		echo "==> RISCV32 OBJECT OK"; \
	else \
		echo "==> SKIP: riscv32-linux-gnu-gcc not found; target C emission only"; \
	fi
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)"

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
	$(call _mt_run,$(TEST_FBC_TRIPLET_CMD) -target riscv64-linux-gnu -r "$(TEST_TMP)/riscv64-smoke.bas" -x "$(TEST_TMP)/riscv64-smoke")
	@test -s "$(TEST_TMP)/riscv64-smoke.c" || { echo "ERROR: riscv64 C output was not produced"; exit 1; }
	@if command -v riscv64-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "==> riscv64-linux-gnu-gcc found; compiling riscv64 object"; \
		$(TEST_FBC_TRIPLET_CMD) -target riscv64-linux-gnu -c "$(TEST_TMP)/riscv64-smoke.bas" -o "$(TEST_TMP)/riscv64-smoke.o"; \
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
	$(call _mt_run,$(TEST_FBC_TRIPLET_CMD) -target s390x-linux-gnu -r "$(TEST_TMP)/s390x-smoke.bas" -x "$(TEST_TMP)/s390x-smoke")
	@test -s "$(TEST_TMP)/s390x-smoke.c" || { echo "ERROR: s390x C output was not produced"; exit 1; }
	@$(TEST_FBC_TRIPLET_CMD) -target s390x-linux-gnu -v -c "$(TEST_TMP)/s390x-smoke.bas" -o "$(TEST_TMP)/s390x-smoke.o" > "$(TEST_TMP)/s390x-gcc.args" 2>&1 || true
	@grep -q -- '-march=z900' "$(TEST_TMP)/s390x-gcc.args" || { echo "ERROR: s390x gcc command did not use -march=z900"; exit 1; }
	@! grep -q -- '-march=s390x' "$(TEST_TMP)/s390x-gcc.args" || { echo "ERROR: s390x gcc command used invalid -march=s390x"; exit 1; }
	@if command -v s390x-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "==> s390x-linux-gnu-gcc found; compiling s390x object"; \
		$(TEST_FBC_TRIPLET_CMD) -target s390x-linux-gnu -c "$(TEST_TMP)/s390x-smoke.bas" -o "$(TEST_TMP)/s390x-smoke.o"; \
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
	$(call _mt_run,$(TEST_FBC_TRIPLET_CMD) -target loongarch64-linux-gnu -r "$(TEST_TMP)/loongarch64-smoke.bas" -x "$(TEST_TMP)/loongarch64-smoke")
	@test -s "$(TEST_TMP)/loongarch64-smoke.c" || { echo "ERROR: loongarch64 C output was not produced"; exit 1; }
	@if command -v loongarch64-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "==> loongarch64-linux-gnu-gcc found; compiling loongarch64 object"; \
		$(TEST_FBC_TRIPLET_CMD) -target loongarch64-linux-gnu -c "$(TEST_TMP)/loongarch64-smoke.bas" -o "$(TEST_TMP)/loongarch64-smoke.o"; \
		readelf -h "$(TEST_TMP)/loongarch64-smoke.o" | grep -q 'Machine:.*LoongArch' || { echo "ERROR: object is not LoongArch"; exit 1; }; \
		echo "==> LOONGARCH64 OBJECT OK"; \
	else \
		echo "==> SKIP: loongarch64-linux-gnu-gcc not found; target C emission only"; \
	fi
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)"

compiler-mips-smoke:
	@test -n "$(TEST_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	$(call _mt_echo,MIPS32 and MIPS64 compiler target smoke test)
	@mkdir -p "$(TEST_TMP)"
	@set -e; \
	for spec in \
		mips-linux-gnu:ELF32:big:mips32:32 \
		mipsel-linux-gnu:ELF32:little:mips32:32 \
		mips64-linux-gnuabi64:ELF64:big:mips64:64 \
		mips64el-linux-gnuabi64:ELF64:little:mips64:64; do \
		oldifs=$$IFS; IFS=:; set -- $$spec; IFS=$$oldifs; \
		target=$$1; elfclass=$$2; endian=$$3; march=$$4; abi=$$5; \
		stem="$(TEST_TMP)/$$target-smoke"; \
		cp tests/mips/target-defines.bas "$$stem.bas"; \
		$(TEST_FBC_TRIPLET_CMD) -target $$target -r "$$stem.bas" -x "$$stem"; \
		test -s "$$stem.c" || { echo "ERROR: $$target C output was not produced"; exit 1; }; \
		$(TEST_FBC_TRIPLET_CMD) -target $$target -v -c "$$stem.bas" -o "$$stem.o" > "$$stem.args" 2>&1 || true; \
		grep -q -- "-march=$$march" "$$stem.args" || { echo "ERROR: $$target did not select -march=$$march"; exit 1; }; \
		grep -q -- "-mabi=$$abi" "$$stem.args" || { echo "ERROR: $$target did not select -mabi=$$abi"; exit 1; }; \
		cc="$$target-gcc"; \
		if command -v "$$cc" >/dev/null 2>&1; then \
			$(TEST_FBC_TRIPLET_CMD) -target $$target -c "$$stem.bas" -o "$$stem.o"; \
			readelf -h "$$stem.o" | grep -q "Class:.*$$elfclass" || { echo "ERROR: $$target object has the wrong ELF class"; exit 1; }; \
			readelf -h "$$stem.o" | grep -q "Data:.*$$endian endian" || { echo "ERROR: $$target object has the wrong byte order"; exit 1; }; \
			readelf -h "$$stem.o" | grep -q 'Machine:.*MIPS' || { echo "ERROR: $$target object is not MIPS"; exit 1; }; \
		fi; \
	done
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)"

compiler-ppc-smoke:
	@test -n "$(TEST_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	$(call _mt_echo,PowerPC compiler target smoke test)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" \
		'#if not defined(__FB_LINUX__)' \
		'#error expected linux target' \
		'#endif' \
		'#if not defined(__FB_PPC__)' \
		'#error expected ppc target' \
		'#endif' \
		'#if not defined(__FB_BIGENDIAN__)' \
		'#error expected big-endian target' \
		'#endif' \
		'print "ppc ok"' \
		> "$(TEST_TMP)/ppc-smoke.bas"
	$(call _mt_run,$(TEST_FBC_TRIPLET_CMD) -target powerpc-linux-gnu -r "$(TEST_TMP)/ppc-smoke.bas" -x "$(TEST_TMP)/ppc-smoke")
	@test -s "$(TEST_TMP)/ppc-smoke.c" || { echo "ERROR: ppc C output was not produced"; exit 1; }
	@if command -v powerpc-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "==> powerpc-linux-gnu-gcc found; compiling ppc object"; \
		$(TEST_FBC_TRIPLET_CMD) -target powerpc-linux-gnu -c "$(TEST_TMP)/ppc-smoke.bas" -o "$(TEST_TMP)/ppc-smoke.o"; \
		readelf -h "$(TEST_TMP)/ppc-smoke.o" | grep -q 'Machine:.*PowerPC' || { echo "ERROR: object is not PowerPC"; exit 1; }; \
		echo "==> PPC OBJECT OK"; \
	else \
		echo "==> SKIP: powerpc-linux-gnu-gcc not found; target C emission only"; \
	fi
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)"

compiler-ppc64-smoke:
	@test -n "$(TEST_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	$(call _mt_echo,PowerPC64 compiler target smoke test)
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
		'#if not defined(__FB_BIGENDIAN__)' \
		'#error expected big-endian target' \
		'#endif' \
		'print "ppc64 ok"' \
		> "$(TEST_TMP)/ppc64-smoke.bas"
	$(call _mt_run,$(TEST_FBC_TRIPLET_CMD) -target powerpc64-linux-gnu -r "$(TEST_TMP)/ppc64-smoke.bas" -x "$(TEST_TMP)/ppc64-smoke")
	@test -s "$(TEST_TMP)/ppc64-smoke.c" || { echo "ERROR: ppc64 C output was not produced"; exit 1; }
	@if command -v powerpc64-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "==> powerpc64-linux-gnu-gcc found; compiling ppc64 object"; \
		$(TEST_FBC_TRIPLET_CMD) -target powerpc64-linux-gnu -c "$(TEST_TMP)/ppc64-smoke.bas" -o "$(TEST_TMP)/ppc64-smoke.o"; \
		readelf -h "$(TEST_TMP)/ppc64-smoke.o" | grep -q 'Machine:.*PowerPC64' || { echo "ERROR: object is not PowerPC64"; exit 1; }; \
		echo "==> PPC64 OBJECT OK"; \
	else \
		echo "==> SKIP: powerpc64-linux-gnu-gcc not found; target C emission only"; \
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
	$(call _mt_run,$(TEST_FBC_TRIPLET_CMD) -target powerpc64le-linux-gnu -r "$(TEST_TMP)/ppc64le-smoke.bas" -x "$(TEST_TMP)/ppc64le-smoke")
	@test -s "$(TEST_TMP)/ppc64le-smoke.c" || { echo "ERROR: ppc64le C output was not produced"; exit 1; }
	@if command -v powerpc64le-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "==> powerpc64le-linux-gnu-gcc found; compiling ppc64le object"; \
		$(TEST_FBC_TRIPLET_CMD) -target powerpc64le-linux-gnu -c "$(TEST_TMP)/ppc64le-smoke.bas" -o "$(TEST_TMP)/ppc64le-smoke.o"; \
		readelf -h "$(TEST_TMP)/ppc64le-smoke.o" | grep -q 'Machine:.*PowerPC64' || { echo "ERROR: object is not PowerPC64"; exit 1; }; \
		echo "==> PPC64LE OBJECT OK"; \
	else \
		echo "==> SKIP: powerpc64le-linux-gnu-gcc not found; target C emission only"; \
	fi
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)"

compiler-riscos-smoke:
	@test -n "$(TEST_FBC)" || { echo "ERROR: no usable fbc found"; exit 1; }
	$(call _mt_echo,RISC OS compiler target smoke test)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" \
		"'' FreeBASIC RISC OS compiler smoke source" \
		"'' Generated by mk/tests/compiler/smoke.mk to validate target wiring." \
		'#if not defined(__FB_RISCOS__)' \
		'#error expected RISC OS target' \
		'#endif' \
		'#if not defined(__FB_UNIX__)' \
		'#error expected UnixLib/Unix target layer' \
		'#endif' \
		'#if not defined(__FB_ARM__)' \
		'#error expected ARM target' \
		'#endif' \
		'#if defined(__FB_64BIT__) or defined(__FB_BIGENDIAN__)' \
		'#error expected 32-bit little-endian target' \
		'#endif' \
		'#include once "crt/stdio.bi"' \
		'#include once "crt/time.bi"' \
		'#include once "crt/sys/socket.bi"' \
		'#include once "crt/unistd.bi"' \
		'#include once "crt/fcntl.bi"' \
		'#include once "crt/errno.bi"' \
		'#include once "crt/wchar.bi"' \
		'#include once "crt/netinet/in.bi"' \
		'#if EAGAIN <> 35 or O_CREAT <> &h200 or CLOCKS_PER_SEC <> 100 or L_tmpnam <> 255' \
		'#error expected GCCSDK UnixLib constants' \
		'#endif' \
		'#assert _SC_PHYS_PAGES = 11' \
		'#assert _SC_NPROCESSORS_ONLN = 12' \
		'#assert sizeof(wchar_t) = 4' \
		'#assert sizeof(flock) = 16' \
		'#assert sizeof(timespec) = 8' \
		'#assert sizeof(tm) = 44' \
		'#assert sizeof(mbstate_t) = 8' \
		'#assert sizeof(sockaddr) = 16' \
		'#assert sizeof(sockaddr_storage) = 128' \
		'#assert sizeof(cmsgcred) = 84' \
		'type RiscosByteStruct' \
		'    value as ubyte' \
		'end type' \
		'type RiscosLongintStruct' \
		'    tag as ubyte' \
		'    value as longint' \
		'end type' \
		'type RiscosPackedByteStruct field = 1' \
		'    value as ubyte' \
		'end type' \
		'#assert sizeof(RiscosByteStruct) = 1' \
		'#assert offsetof(RiscosLongintStruct, value) = 4' \
		'#assert sizeof(RiscosLongintStruct) = 12' \
		'#assert sizeof(RiscosPackedByteStruct) = 1' \
		'dim crt_file as FILE ptr' \
		'dim crt_time as tm' \
		'dim crt_address as sockaddr_in' \
		'dim crt_process_group as long = setpgrp(0, 0)' \
		'print "riscos ok"' \
		"'' end of riscos-smoke.bas" \
		> "$(TEST_TMP)/riscos-smoke.bas"
	$(call _mt_run,$(TEST_FBC_CMD) -i "$(rootdir)/inc/riscos" -i "$(rootdir)/inc" -target arm-unknown-riscos -r "$(TEST_TMP)/riscos-smoke.bas" -x "$(TEST_TMP)/riscos-smoke")
	@test -s "$(TEST_TMP)/riscos-smoke.c" || { echo "ERROR: RISC OS C output was not produced"; exit 1; }
	@printf "%s\n" \
		"'' FreeBASIC RISC OS CRT include-order smoke source" \
		"'' Generated by mk/tests/compiler/smoke.mk to catch duplicate declarations." \
		'#include once "crt/unistd.bi"' \
		'#include once "crt/sys/socket.bi"' \
		'dim close_result as long = close_(-1)' \
		"'' end of riscos-include-order-smoke.bas" \
		> "$(TEST_TMP)/riscos-include-order-smoke.bas"
	$(call _mt_run,$(TEST_FBC_CMD) -i "$(rootdir)/inc/riscos" -i "$(rootdir)/inc" -target arm-unknown-riscos -r "$(TEST_TMP)/riscos-include-order-smoke.bas" -x "$(TEST_TMP)/riscos-include-order-smoke")
	@test -s "$(TEST_TMP)/riscos-include-order-smoke.c" || { echo "ERROR: reverse-order CRT C output was not produced"; exit 1; }
	@env -u GCC -u CLANG "$(TEST_FBC)" -i "$(rootdir)/inc/riscos" -i "$(rootdir)/inc" -target arm-unknown-riscos -v -c "$(TEST_TMP)/riscos-smoke.bas" -o "$(TEST_TMP)/riscos-smoke.o" > "$(TEST_TMP)/riscos-gcc.args" 2>&1 || true
	@grep -q 'arm-unknown-riscos-gcc' "$(TEST_TMP)/riscos-gcc.args" || { echo "ERROR: GCCSDK compiler driver was not selected"; exit 1; }
	@grep -q -- '-march=armv4' "$(TEST_TMP)/riscos-gcc.args" || { echo "ERROR: RISC OS did not use its ARMv4 compatibility baseline"; exit 1; }
	@env -u GCC -u CLANG "$(TEST_FBC)" -i "$(rootdir)/inc/riscos" -i "$(rootdir)/inc" -target riscos-arm -v -c "$(TEST_TMP)/riscos-smoke.bas" -o "$(TEST_TMP)/riscos-canonical.o" > "$(TEST_TMP)/riscos-canonical-gcc.args" 2>&1 || true
	@grep -q 'arm-unknown-riscos-gcc' "$(TEST_TMP)/riscos-canonical-gcc.args" || { echo "ERROR: canonical RISC OS target did not select GCCSDK"; exit 1; }
	@grep -q -- '-march=armv4' "$(TEST_TMP)/riscos-canonical-gcc.args" || { echo "ERROR: canonical RISC OS target did not use ARMv4"; exit 1; }
	@if command -v arm-unknown-riscos-gcc >/dev/null 2>&1; then \
		echo "==> GCCSDK found; compiling RISC OS object"; \
		env -u GCC -u CLANG "$(TEST_FBC)" -i "$(rootdir)/inc/riscos" -i "$(rootdir)/inc" -target arm-unknown-riscos -c "$(TEST_TMP)/riscos-smoke.bas" -o "$(TEST_TMP)/riscos-smoke.o"; \
		readelf -h "$(TEST_TMP)/riscos-smoke.o" | grep -q 'Machine:.*ARM' || { echo "ERROR: object is not ARM"; exit 1; }; \
		echo "==> RISC OS OBJECT OK"; \
	else \
		echo "==> SKIP: arm-unknown-riscos-gcc not found; target C emission only"; \
	fi
	@rm -rf "$(TEST_TMP)" "$(LOG_DIR)"

##############################################################################
# end of tests/compiler/smoke.mk
##############################################################################
