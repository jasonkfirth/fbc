<!--
Project: FreeBASIC compiler diagnostics
File: compiler-source-warnings.md
Purpose: Describe source feedback retained independently of native C cleanup.
Responsibilities: Explain warning levels, conservative boundaries and qualification.
This file intentionally does NOT classify every native warning as a BASIC error.
-->

# Source warnings behind the GCC cleanup

These diagnostics remain enabled in the [current selected compiler](c-emitter-current-status.md).
The earlier debug-binding census recorded 195 GCC and 10,027 Clang observations
across 5,845 profiles. Both native compilers accepted all 5,739 open C definitions.
Historical diagnostic-only qualification used 5C78E4AD.

The compiler reports these diagnostics before emitting C or assembly. Removing
an unused C binding or simplifying a C expression therefore cannot remove this
source feedback. Existing warning numbers retain their meanings.

| Number | Message | Enabled |
| --- | --- | --- |
| 54 | Parameter is not read | `-w all` |
| 55 | Variable is not read | `-w all` |
| 56 | Comparison result is fixed by the operand range | Default and `-w all` |
| 57 | Memory access through a provably null pointer | Default and `-w all` |

`-w none` disables them. `-w error` promotes enabled warnings through the normal
diagnostic reporter. Required callback parameters and deliberately retained
scratch values remain valid BASIC; their unused-value advisories are opt-in.
Output-only BYREF parameters count as used. Compiler temporaries, implicit
instance parameters, owned local objects and shared storage are excluded.

Unused-value analysis inspects the parsed procedure body before backend storage
cleanup. Address escapes count as observations. A procedure containing opaque
assembly is left alone. Declaration locations are retained separately, so a
warning can name an included file without moving or rescanning the lexer.
Metadata is released at procedure completion, with a separate parser-recovery
cleanup. The pass does not modify AST nodes or procedure signatures.

Range checking covers direct integer variable, field and indexed
operands up to 32 bits, compared with nonnegative integer constants. It uses the
declared range before promotion. It conservatively excludes ambiguous unsigned
conversions, negative constants, wider operands and inferred value ranges. For
example, `Dim height As Long: If height = &h80000000 Then ...` cannot match on
Win64 because the literal is positive 2147483648. The corresponding Win32
comparison has different signedness rules and is not warned about.

The current selected compiler also diagnoses typed pointer
dereferences under warning 56. That check observes the declared type without
evaluating the expression. All six backend/width combinations pass the new
default/all/none, optimization/debug/runtime-check and warning-as-error controls.
The expanded source-warning suite contains 462 checks. Selection and the complete native recount are recorded in the current status.

The C rewrite retains both original integer operands as evaluated expressions,
then produces the proven value or branch. Scalar IIF arms can be selected only
when the condition has become a known branch and both arms assign the same
scalar temporary. Managed results and branch-local destructor wrappers retain
their original conditions. This preserves source feedback and lazy evaluation. Fixed zero-AND and all-ones-OR
results also retain both operands because ordinary bitwise operators evaluate both.

Null-access checking recognizes constant zero and a recent straight-line zero
assignment to an automatic pointer. Calls, aliases, control-flow joins and
unmodeled linked evaluations stop that proof. Address-only expressions, SIZEOF,
OFFSETOF and nonzero absolute addresses do not imply a null memory access. The
warning also works with `-exx`; a runtime check does not erase the source fact.
This is a local diagnostic, not a general alias or control-flow analysis.

## Remaining GCC observations

The current selected compiler's review covers all 61 previously warning-bearing
profiles under their original unsuppressed GCC flags. It counts 219 GCC
observations at the preceding 838B6658 snapshot, including profiles made clean by compiler scratch cleanup. The
complete 5,845-profile census agrees; see the [current totals](c-emitter-current-status.md).
No corpus application was run. The original diagnostic-only qualification below
used 5C78E4AD and retained 229 observations with byte-identical C.

The new FBC diagnostics in those profiles comprise 165 unused-parameter and
89 unread-variable advisories, one impossible comparison and six known-null
accesses. They include source values already removed from C by existing cleanup.
No compiler temporary is exposed as a source warning.

Of GCC's 176 unused-parameter observations, 141 match warning 54 by source file,
procedure header line and decoded parameter name. Nine name implicit `THIS`
parameters. The remaining 26 belong to four intentional feature stubs whose
configured implementation returns immediately while the source retains its
ordinary parameter uses. Those 35 observations do not establish a source defect.

All five set-but-not-read observations refer to named source values and receive
warning 55 under `-w all`. These include a deliberately retained static and a
macro-local named `TMP`. Earlier summaries incorrectly called four compiler
temporaries; matching source diagnostics confirm their source identities. At the
219-observation baseline, all 24 unused-variable observations concern compiler
storage. Four unused-function observations
concern retained library definitions, including two implicit assignment operators.
These must be handled by backend retention policy rather than misleading source
warnings. The two checked formatter paths and the previously qualified Stunts
containing-record copy false positive establish no additional source fault.

The BMP range guard and deliberate exception-demo accesses now have FBC feedback.
Their source files remain unchanged. PNG and closed assembler work stay excluded.

The final review reruns all 420 source-warning controls with selected compiler
838B6658 and verifies the exact compiler, 10,929 corpus source pins, 2,344 compiler
input pins, installed artifacts and normal runtime archives against the complete
qualified runs. The seven correctness observations are covered by default FBC
warnings; unread source values remain opt-in advisories. No additional source
fault is established by the remaining native warning classes.

The subsequent [debug-binding repair](c-emitter-debug-bindings.md) removes those
24 compiler-storage observations in a fresh 61-profile audit. Its complete replay and selection pass at 195 GCC and 10,027 Clang
observations; these current results remain separate from this historical census.

## Reproduction and qualification

From the repository root, with a compiler built from these sources:

```powershell
python -B build_scripts/test-source-warnings.py --fbc bin/fbc.exe --prefix .
```

The command above assumes a normal checkout build. This campaign uses
`build/main-gcc/bin/fbc.exe` with `--prefix build/main-gcc` to preserve the
installed release compiler. The source changes are in the normal compiler tree;
no private binary is required to build or run the regression elsewhere.

The current regression performs 462 checks across Win32/Win64, both C generators and
the native assembly generators, warning levels, O0/O2, debug and runtime checks.
`--reference <previous-fbc.exe>` additionally requires byte-identical emitted
outputs for non-promoted cases. Use a reference with matching code generation;
the indirect-range rewrite intentionally changes some outputs. The earlier
420-check matrix qualified 408 such identities. The fault examples are compiled only. The complete GCC backend
suite invokes this regression and passes without a whole-suite deadline.

## Earlier diagnostic-only qualification

The earlier compiler reached an emitted-C fixed point across 224 units with zero
GCC warnings. Strict lint covered its eleven changed or added BASIC files. Compiler
structural checks pass; unchanged runtime structural results remain qualified.
Both C generators pass Clang's original full compiler diagnostics: 1,338
observations each, comprising 1,325 padding and 13 global-constructor observations.
The six additional padding observations expose existing lexer/file layouts used
for source locations; no layout was changed and no warning class was added.

Durable receipts are named `generated-c-source-warnings-*` under the campaign's
existing canonical work directory. The authoritative build is `build-v3`, the
focused checks and structure receipts end in `-v3`, and the fresh GCC review is
`audit`. Earlier failed checks and retired intermediate compiler recipes remain
explicit. Installed release binaries and normal ST/MT archives are unchanged.

<!-- end of compiler-source-warnings.md -->
