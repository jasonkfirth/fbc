##############################################################################
# tests/build/structure.mk
#
# Verifies the mk/ directory structure matches the refactored layout
##############################################################################

.PHONY: mk-structure-test
mk-structure-test:
	$(call _mt_echo,Verifying $(mkpath)/ structure)

	@test -f $(rootdir)/GNUmakefile || { echo "ERROR: missing $(rootdir)/GNUmakefile"; exit 1; }
	@test -d $(mkpath) || { echo ""; echo "ERROR: mk directory missing: $(mkpath)"; echo ""; exit 1; }

	@test -f $(mkpath)/platform.mk        || { echo "ERROR: missing $(mkpath)/platform.mk"; exit 1; }
	@test -f $(mkpath)/source-graph.mk    || { echo "ERROR: missing $(mkpath)/source-graph.mk"; exit 1; }
	@test -f $(mkpath)/layout.mk          || { echo "ERROR: missing $(mkpath)/layout.mk"; exit 1; }
	@test -f $(mkpath)/build-layout.mk    || { echo "ERROR: missing $(mkpath)/build-layout.mk"; exit 1; }
	@test -f $(mkpath)/toolchain-flags.mk || { echo "ERROR: missing $(mkpath)/toolchain-flags.mk"; exit 1; }
	@test -f $(mkpath)/android/toolchain-flags.mk || { echo "ERROR: missing Android toolchain-flags.mk"; exit 1; }

	@test -d $(mkpath)/build || { echo "ERROR: missing $(mkpath)/build directory"; exit 1; }

	@test -f $(mkpath)/build/build-targets.mk    || { echo "ERROR: missing build-targets.mk"; exit 1; }
	@test -f $(mkpath)/build/compile-rules.mk    || { echo "ERROR: missing compile-rules.mk"; exit 1; }
	@test -f $(mkpath)/build/archive-rules.mk    || { echo "ERROR: missing archive-rules.mk"; exit 1; }
	@test -f $(mkpath)/build/dependency-rules.mk || { echo "ERROR: missing dependency-rules.mk"; exit 1; }
	@test -f $(mkpath)/build/clean-rules.mk      || { echo "ERROR: missing clean-rules.mk"; exit 1; }

	@test -f $(mkpath)/inst_uninst.mk || { echo "ERROR: missing $(mkpath)/inst_uninst.mk"; exit 1; }
	@test -z "$$(find $(mkpath) -type f \( -name '*.bak' -o -name '*~' \) -print -quit)" || { \
		echo "ERROR: unexpected backup file under $(mkpath)"; \
		find $(mkpath) -type f \( -name '*.bak' -o -name '*~' \); \
		exit 1; \
	}

	@arm_flags="$$( $(MAKE) --no-print-directory -s print-config \
		TARGET_TRIPLET=armv7a-linux-androideabi \
		TARGET=armv7a-linux-androideabi CC="$(CC)" CXX="$(CXX)" \
		| sed -n 's/^ALLCFLAGS=//p' )"; \
	printf '%s\n' "$$arm_flags" | grep -F -- '-mfloat-abi=softfp' >/dev/null || { \
		echo "ERROR: Android ARMv7 C flags lost the softfp ABI baseline"; \
		exit 1; \
	}; \
	printf '%s\n' "$$arm_flags" | grep -F -- '-mfpu=vfpv3-d16' >/dev/null || { \
		echo "ERROR: Android ARMv7 C flags lost the non-NEON baseline"; \
		exit 1; \
	}

	$(call _mt_echo,$(mkpath)/ structure OK)

##############################################################################
# end of tests/build/structure.mk
##############################################################################
