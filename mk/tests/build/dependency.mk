##############################################################################
# tests/build/dependency.mk
#
# Dependency tracking verification tests
##############################################################################

.PHONY: dependency-test
dependency-test:
	$(call _mt_echo,Testing dependency tracking)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" "#include \"dep.h\"" "int main(void){return TESTVAL;}" > "$(TEST_TMP)/dep.c"
	@printf "%s\n" "#define TESTVAL 0" > "$(TEST_TMP)/dep.h"
	@printf "%s\n" \
"all: dep.o" \
"-include dep.d" \
"dep.o: dep.c dep.h" \
"	\$$(CC) -MMD -MP -c dep.c -o dep.o" \
	> "$(TEST_TMP)/Makefile"
	$(call _mt_run,$(MAKE) -C "$(TEST_TMP)" CC="$(CC)" all)
	@cp "$(TEST_TMP)/dep.o" "$(TEST_TMP)/dep-before.o"
	@printf "%s\n" "#define TESTVAL 1" > "$(TEST_TMP)/dep.h"
	$(call _mt_run,$(MAKE) -C "$(TEST_TMP)" CC="$(CC)" all)
	@if cmp -s "$(TEST_TMP)/dep-before.o" "$(TEST_TMP)/dep.o"; then \
		echo "ERROR: dependency rebuild did not update the object"; \
		exit 1; \
	fi
	$(call _mt_cleanup_success)

##############################################################################
# end of tests/build/dependency.mk
##############################################################################
