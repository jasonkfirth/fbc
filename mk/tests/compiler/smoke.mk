##############################################################################
# tests/compiler/smoke.mk
#
# Compiler smoke test
##############################################################################

.PHONY: compiler-smoke compiler-semantic-model-smoke compiler-indirect-goto-smoke compiler-riscv32-smoke compiler-riscv64-smoke compiler-s390x-smoke compiler-loongarch64-smoke compiler-mips-smoke compiler-ppc-smoke compiler-ppc64-smoke compiler-ppc64le-smoke compiler-riscos-smoke
compiler-smoke: libs
	$(call _mt_echo,Compiler smoke test)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" 'print "ok"' > "$(TEST_TMP)/smoke.bas"
	$(call _mt_run,$(TEST_FBC_CMD) "$(TEST_TMP)/smoke.bas" -x "$(TEST_TMP)/smoke$(EXEEXT)")
ifneq ($(CAN_RUN),)
	@./"$(TEST_TMP)/smoke$(EXEEXT)" >/dev/null 2>&1 && echo "==> RUN OK"
endif
	$(call _mt_cleanup_success)

compiler-semantic-model-smoke:
	$(call _mt_echo,Compiler semantic model smoke test)
	@mkdir -p "$(TEST_TMP)"
	@printf "%s\n" "'' schema 6 dependency inventory smoke" > "$(TEST_TMP)/semantic-model-header.bi"
	@printf "%s\n" \
	'#define SEMANTIC_INCREMENT 2' \
	'#assert __FB_ERR__ = 0' \
	'#if 1 + 2 = 3' \
	'#assert 1 + 2 = 3' \
	'#endif' \
		'declare function semantic_helper() as integer' \
	'sub semantic_probe()' \
	'    dim as integer value = 1' \
	'    value += semantic_helper() + 1' \
	'    value += semantic_helper() + SEMANTIC_INCREMENT' \
	'end sub' \
	'sub semantic_varargs cdecl (byval count as integer, ...)' \
	'    dim as integer value = count' \
	'end sub' \
	'semantic_probe()' \
	'type semantic_probe_type' \
	'    precise as double' \
	'end type' \
	'sub semantic_typeof_probe()' \
	'    dim checked as semantic_probe_type' \
	'    #assert typeof(checked.precise) = typeof(double)' \
	'end sub' \
	'#define D .' \
	'dim array(0 to D D D) as integer => { 1 }' \
	'#include "semantic-model-header.bi"' \
	> "$(TEST_TMP)/semantic-model.bas"
	@printf "%s\n" \
		'function semantic_helper() as integer' \
		'    return 1' \
		'end function' \
		> "$(TEST_TMP)/semantic-model-helper.bas"
	$(call _mt_run,$(TEST_FBC_CMD) -semantic-model "$(TEST_TMP)/semantic-model.tsv" -r "$(TEST_TMP)/semantic-model.bas" "$(TEST_TMP)/semantic-model-helper.bas")
	@awk -F '\t' '\
		NR == 1 { if (NF != 3 || $$1 != "FBCSEM" || $$2 != "6") exit 1; header = 1; next } \
		$$1 == "M" { if (NF != 2) exit 1; modules++; next } \
		$$1 == "D" { if (NF != 2 || $$2 == "" || dependencies[$$2]++) exit 1; if ($$2 ~ /semantic-model-header\.bi$$/) header_dependency = 1; dependency_count++; next } \
		$$1 == "S" { if (NF != 12 || symbol_ids[$$2]++) exit 1; if ($$6 != 0) symbol_refs[++symbol_ref_count] = $$6; if ($$12 != 0) symbol_refs[++symbol_ref_count] = $$12; symbols++; next } \
		$$1 == "P" { if (NF != 9) exit 1; symbol_refs[++symbol_ref_count] = $$2; procedures++; next } \
		$$1 == "V" { if (NF != 5 || $$2 == 0 || $$4 == "" || ($$5 != "pointer" && $$5 != "numeric" && $$5 != "dynamic-string" && $$5 != "fixed-string" && $$5 != "aggregate" && $$5 != "procedure" && $$5 != "other")) exit 1; symbol_refs[++symbol_ref_count] = $$2; typefacts++; next } \
		$$1 == "N" { if (NF != 11 || node_ids[$$2]++) exit 1; if ($$8 != 0) symbol_refs[++symbol_ref_count] = $$8; if ($$9 != 0) symbol_refs[++symbol_ref_count] = $$9; nodes++; next } \
		$$1 == "E" { if (NF != 14 || expression_ids[$$2]++ || ($$3 != "0" && $$3 != "1") || $$4 == "" || $$5 < 1 || $$6 < 0 || $$7 < $$5 || ($$7 == $$5 && $$8 <= $$6) || $$8 < 0 || $$14 == "") exit 1; if ($$3 == "1" && $$5 == 9 && $$6 == 13 && $$7 == 9 && $$8 == 34) direct_range = 1; if ($$3 == "0" && $$5 == 10 && $$6 == 13) expanded_range = 1; if ($$3 == "0" && $$5 == 24 && $$6 == 41 && $$7 == 24 && $$8 == 42) macro_range_withheld = 1; if ($$5 >= 2 && $$5 <= 4) directive_expressions++; if ($$14 == "double") nested_operands[$$5] = 1; if ($$14 == "integer") nested_comparisons[$$5] = 1; if ($$12 != 0) symbol_refs[++symbol_ref_count] = $$12; if ($$13 != 0) symbol_refs[++symbol_ref_count] = $$13; expressions++; next } \
		$$1 == "END" { if (NF != 10 || $$2 != "6" || $$3 != modules || $$4 != procedures || $$5 != symbols || $$6 != typefacts || $$7 != nodes || $$8 != expressions || $$9 != dependency_count || $$10 != "1") exit 1; footer = 1; next } \
		footer { exit 1 } \
		{ exit 1 } \
		END { for (line in nested_operands) if (line in nested_comparisons) nested_typeof = 1; if (!header || !footer || modules != 2 || dependency_count != 3 || !header_dependency || procedures < 2 || symbols < 1 || typefacts < 2 || nodes < 1 || expressions < 2 || directive_expressions < 3 || !direct_range || !expanded_range || !macro_range_withheld || !nested_typeof) exit 1; for (i = 1; i <= symbol_ref_count; i++) if (!(symbol_refs[i] in symbol_ids)) exit 1 }' \
		"$(TEST_TMP)/semantic-model.tsv" || { echo "ERROR: invalid or incomplete compiler semantic model"; exit 1; }
	$(call _mt_run,$(TEST_FBC_CMD) -semantic-model-expressions "$(TEST_TMP)/semantic-expressions.tsv" -r "$(TEST_TMP)/semantic-model.bas" "$(TEST_TMP)/semantic-model-helper.bas")
	@awk -F '\t' '\
		NR == 1 { if (NF != 3 || $$1 != "FBCSEM" || $$2 != "6") exit 1; header = 1; next } \
		$$1 == "M" { if (NF != 2) exit 1; modules++; next } \
		$$1 == "D" { if (NF != 2 || $$2 == "" || dependencies[$$2]++) exit 1; if ($$2 ~ /semantic-model-header\.bi$$/) header_dependency = 1; dependency_count++; next } \
		$$1 == "E" { if (NF != 14 || $$2 != expressions + 1 || $$4 == "" || $$5 < 1 || $$6 < 0 || $$7 < $$5 || ($$7 == $$5 && $$8 <= $$6) || $$8 < 0 || $$12 != 0 || $$13 != 0 || $$14 == "") exit 1; expressions++; next } \
		$$1 == "END" { if (NF != 10 || $$2 != "6" || $$3 != modules || $$4 != 0 || $$5 != 0 || $$6 != 0 || $$7 != 0 || $$8 != expressions || $$9 != dependency_count || $$10 != "1") exit 1; footer = 1; next } \
		footer { exit 1 } \
		{ exit 1 } \
		END { if (!header || !footer || modules != 2 || dependency_count != 3 || !header_dependency || expressions < 2) exit 1 }' \
		"$(TEST_TMP)/semantic-expressions.tsv" || { echo "ERROR: expression-only semantic model contains unrelated or invalid records"; exit 1; }
	@printf "%s\n" \
		'dim as double semantic_factor' \
		'dim as integer semantic_result' \
		'semantic_result = 2 + 3 * semantic_factor' \
		'type SemanticRangeRecord' \
		'    precise as double' \
		'end type' \
	'dim as SemanticRangeRecord semantic_record' \
	'semantic_result = -semantic_factor' \
	'semantic_result = semantic_record.precise' \
	'semantic_result = len("abc")' \
	'semantic_result = sizeof(semantic_factor + 1)' \
	'semantic_result = iif(semantic_factor > 0, semantic_factor + 1, semantic_factor + 2)' \
	'semantic_result = 1 + 2 + semantic_factor + 4' \
	> "$(TEST_TMP)/semantic-intermediate.bas"
	$(call _mt_run,$(TEST_FBC_CMD) -semantic-model-expressions "$(TEST_TMP)/semantic-intermediate.tsv" -c "$(TEST_TMP)/semantic-intermediate.bas" -o "$(TEST_TMP)/semantic-intermediate.o")
	@awk -F '\t' '$$1 == "E" && $$3 == "1" && $$5 == 3 && $$6 == 22 && $$7 == 3 && $$8 == 41 && tolower($$14) == "double" { inner = 1 } $$1 == "E" && $$3 == "1" && $$5 == 3 && $$6 == 18 && $$7 == 3 && $$8 == 41 && tolower($$14) == "double" { outer = 1 } $$1 == "E" && $$3 == "1" && $$5 == 8 && $$6 == 18 && $$7 == 8 && $$8 == 34 && tolower($$14) == "double" { unary = 1 } $$1 == "E" && $$3 == "1" && $$5 == 9 && $$6 == 18 && $$7 == 9 && $$8 == 41 && tolower($$14) == "double" { member = 1 } $$1 == "E" && $$3 == "1" && $$5 == 10 && $$6 == 18 && $$7 == 10 && $$8 == 28 && tolower($$14) == "integer" { call = 1 } $$1 == "E" && $$3 == "1" && $$5 == 11 && $$6 == 18 && $$7 == 11 && $$8 == 45 && tolower($$14) == "integer" { sizeof_result = 1 } $$1 == "E" && $$3 == "1" && $$5 == 11 && $$6 == 25 && $$7 == 11 && $$8 == 44 && tolower($$14) == "double" { sizeof_operand = 1 } $$1 == "E" && $$3 == "1" && $$5 == 12 && $$6 == 18 && $$7 == 12 && $$8 == 84 && tolower($$14) == "double" { iif_result = 1 } $$1 == "E" && $$3 == "1" && $$5 == 12 && $$6 == 22 && $$7 == 12 && $$8 == 41 && tolower($$14) == "integer" { iif_condition = 1 } $$1 == "E" && $$3 == "1" && $$5 == 12 && $$6 == 43 && $$7 == 12 && $$8 == 62 && tolower($$14) == "double" { iif_true = 1 } $$1 == "E" && $$3 == "1" && $$5 == 12 && $$6 == 64 && $$7 == 12 && $$8 == 83 && tolower($$14) == "double" { iif_false = 1 } $$1 == "E" && $$3 == "1" && $$5 == 13 && $$6 == 18 && $$7 == 13 && $$8 == 23 && tolower($$14) == "integer" { folded_prefix = 1 } $$1 == "E" && $$3 == "1" && $$5 == 13 && $$6 == 18 && $$7 == 13 && $$8 == 41 && tolower($$14) == "double" { chained_prefix = 1 } END { if (!inner || !outer || !unary || !member || !call || !sizeof_result || !sizeof_operand || !iif_result || !iif_condition || !iif_true || !iif_false || !folded_prefix || !chained_prefix) exit 1 }' \
		"$(TEST_TMP)/semantic-intermediate.tsv" || { echo "ERROR: expression-only model omitted a typed intermediate expression range"; exit 1; }
	@printf "%s\n" \
		'type SemanticRangeLeaf' \
		'    amount as double' \
		'end type' \
		'type SemanticRangeBranch' \
		'    leaf as SemanticRangeLeaf' \
		'    leaves(0 to 2) as SemanticRangeLeaf' \
		'end type' \
		'declare function semantic_range_factory() as SemanticRangeBranch' \
		'dim as SemanticRangeBranch semantic_branch' \
		'print semantic_branch.leaf.amount' \
		'print semantic_branch.leaves(1).amount' \
		'print semantic_range_factory().leaf.amount' \
		> "$(TEST_TMP)/semantic-prefix.bas"
	$(call _mt_run,$(TEST_FBC_CMD) -semantic-model-expressions "$(TEST_TMP)/semantic-prefix.tsv" -c "$(TEST_TMP)/semantic-prefix.bas" -o "$(TEST_TMP)/semantic-prefix.o")
	@awk -F '\t' '$$1 == "E" && $$3 == "1" && $$5 == 10 && $$6 == 6 && $$7 == 10 && $$8 == 26 && tolower($$14) == "semanticrangeleaf" { direct_member = 1 } $$1 == "E" && $$3 == "1" && $$5 == 10 && $$6 == 6 && $$7 == 10 && $$8 == 33 && tolower($$14) == "double" { direct_result = 1 } $$1 == "E" && $$3 == "1" && $$5 == 11 && $$6 == 6 && $$7 == 11 && $$8 == 31 && tolower($$14) == "semanticrangeleaf" { indexed_member = 1 } $$1 == "E" && $$3 == "1" && $$5 == 11 && $$6 == 6 && $$7 == 11 && $$8 == 38 && tolower($$14) == "double" { indexed_result = 1 } $$1 == "E" && $$3 == "1" && $$5 == 12 && $$6 == 6 && $$7 == 12 && $$8 == 30 && tolower($$14) == "semanticrangebranch" { call = 1 } $$1 == "E" && $$3 == "1" && $$5 == 12 && $$6 == 6 && $$7 == 12 && $$8 == 35 && tolower($$14) == "semanticrangeleaf" { call_member = 1 } $$1 == "E" && $$3 == "1" && $$5 == 12 && $$6 == 6 && $$7 == 12 && $$8 == 42 && tolower($$14) == "double" { call_result = 1 } END { if (!direct_member || !direct_result || !indexed_member || !indexed_result || !call || !call_member || !call_result) exit 1 }' \
		"$(TEST_TMP)/semantic-prefix.tsv" || { echo "ERROR: expression-only model omitted a member, index, or call prefix"; exit 1; }
	@printf "%s\n" \
		'#line 23 "semantic-remapped.bas"' \
		'dim as integer padding = len("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")' \
		'#line 22 "semantic-remapped.bas"' \
		'#define D .' \
		'dim array(0 to D D D) as integer => { 1 }' \
		> "$(TEST_TMP)/semantic-remapped-range.bas"
	$(call _mt_run,$(TEST_FBC_CMD) -semantic-model "$(TEST_TMP)/semantic-remapped-range.tsv" -c "$(TEST_TMP)/semantic-remapped-range.bas" -o "$(TEST_TMP)/semantic-remapped-range.o")
	@awk -F '\t' '$$1 == "E" && $$4 == "semantic-remapped.bas" && $$5 == 24 && $$6 == 41 && $$7 == 24 && $$8 == 42 { if ($$3 != "0") exit 1; found = 1 } END { if (!found) exit 1 }' \
		"$(TEST_TMP)/semantic-remapped-range.tsv" || { echo "ERROR: repeated #line location reused a stale physical-line bound"; exit 1; }
	@{ printf 'Print '; printf '%1500s' ''; printf 'Len("😀") + 1 + 2\n'; } > "$(TEST_TMP)/semantic-long-line.bas"
	$(call _mt_run,$(TEST_FBC_CMD) -semantic-model "$(TEST_TMP)/semantic-long-line.tsv" -c "$(TEST_TMP)/semantic-long-line.bas" -o "$(TEST_TMP)/semantic-long-line.o")
	@awk -F '\t' '$$1 == "E" && $$4 ~ /semantic-long-line\.bas$$/ && $$5 == 1 && $$6 == 1506 && $$7 == 1 && $$8 == 1523 { if ($$3 != "1") exit 1; found = 1 } END { if (!found) exit 1 }' \
		"$(TEST_TMP)/semantic-long-line.tsv" || { echo "ERROR: diagnostic excerpt truncated the semantic physical-line bound"; exit 1; }
	@printf "%s\n" \
		'dim as double recovery_factor = 1' \
		'print recovery_factor * 2 +' \
		'print recovery_factor * 3' \
		> "$(TEST_TMP)/semantic-recovery.bas"
	@set +e; $(TEST_FBC_CMD) -semantic-model-expressions "$(TEST_TMP)/semantic-recovery.tsv" -c "$(TEST_TMP)/semantic-recovery.bas" -o "$(TEST_TMP)/semantic-recovery.o" >/dev/null 2>&1; status=$$?; set -e; \
		test $$status -eq 1 || { echo "ERROR: malformed semantic-recovery fixture did not fail compilation"; exit 1; }
	@awk -F '\t' '\
		NR == 1 { if (NF != 3 || $$1 != "FBCSEM" || $$2 != "6") exit 1; header = 1; next } \
		$$1 == "M" { if (NF != 2) exit 1; modules++; next } \
		$$1 == "D" { if (NF != 2 || $$2 == "" || dependencies[$$2]++) exit 1; dependency_count++; next } \
		$$1 == "E" { if (NF != 14) exit 1; expressions++; if ($$3 == "1" && $$5 == 3 && tolower($$14) == "double") later_expression = 1; next } \
		$$1 == "R" { if (NF != 3 || $$2 != "6" || $$3 != expressions) exit 1; recovered_modules++; next } \
		$$1 == "RECOVERY" { if (NF != 7 || $$2 != "6" || $$3 != modules || $$4 != expressions || $$5 != recovered_modules || $$6 != dependency_count || $$7 != "1") exit 1; footer = 1; next } \
		footer { exit 1 } \
		{ exit 1 } \
		END { if (!header || !footer || modules != 1 || dependency_count != 1 || expressions < 1 || recovered_modules != 1 || !later_expression) exit 1 }' \
		"$(TEST_TMP)/semantic-recovery.tsv" || { echo "ERROR: expression-only recovery snapshot is incomplete or mistyped"; exit 1; }
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
