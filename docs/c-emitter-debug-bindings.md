<!--
Project: FreeBASIC C backend
File: c-emitter-debug-bindings.md
Purpose: Record compiler-owned debug storage cleanup and its qualification.
Responsibilities: Explain ownership, debugger contracts and reproducible checks.
This file intentionally does NOT classify named unread source values as temporaries.
-->

# Compiler-owned debug bindings

The C backend extends its existing scratch-storage proof to compiler-generated
scoped variables. `FB_SYMBSTATS_IMPLICIT` identifies compiler-owned symbols;
ordinary implicit declarations in legacy BASIC do not carry that flag. Named
source storage, formal parameters, reference bindings and function results keep
their existing debugger contracts. Opaque assembly retains its conservative path.

An unreferenced implicit descriptor for a referenced private static fixed array
can also disappear under debug output. The named data, element type and bounds
remain available to the debugger. A descriptor passed to another procedure is
referenced and remains present. Dynamic arrays and lifetime wrappers are retained.

Reachable string initialization, copyback and cleanup count as storage uses.
Unreachable generated strings have no lifetime to run. An earlier candidate
incorrectly retained nineteen such declarations and is rejected; a dedicated
unreachable-string check now protects the established behavior. No warning
attribute, diagnostic suppression or dummy read is added.

## Selected compiler and qualification

Selected compiler `3b736d245e30fc79e34e386aca28eea7617ecb7cc3d1dc5e92771cb2133cd2b1`
self-hosts 224 units at an emitted-C fixed point with zero GCC warnings. Two
unrelated source updates encountered during qualification were preserved and
included in that build's input closure.

All 288 native checks and 288 value runs pass across both C generators, GCC and
Clang, O0/O2, runtime checking and normal ST/MT archives. Win32 objects are checked
but not executed. Fifty-two debug binding checks and eight named DWARF type/bounds
comparisons pass. The complete GCC backend suite passes in 166.272 seconds without
a whole-suite timeout, including all 420 source-warning controls. Strict lint is
clean across four BASIC files; compiler and runtime structural guards pass.

The fresh 61-profile GCC audit falls from 219 to 195 observations. All 24 remaining
unused generated-variable observations disappear, with no new warning class.
The complete 5,845-profile GCC replay passes at 195 observations. Compiler
and fbctests remain GCC-clean; production has 65, additional has 124, and
Exampleageddon has three per configuration. All 10,932 source pins agree.
The development compiler is selected, with 838B6658 retained for rollback.
The separate normal sweep remains at three warnings with an identical
C/native binding. Installed artifacts and normal archives are unchanged.
The complete Clang refresh follows GCC promotion and passes at 10,027
observations, 28 fewer: 24 unused generated variables and four unreachable
observations in intentional navigation feature stubs. No warning class is
added. All 5,739 open C definitions pass both native compilers. See
[current status](c-emitter-current-status.md).

The five remaining set-but-not-read observations refer to named source values,
including a macro-local `TMP`. All match FBC warning 55 by source file, name and
declaration location. Earlier summaries incorrectly called four compiler
temporaries. The 176 parameter observations retain their established dispositions:
141 match FBC warning 54, nine are implicit `THIS`, and 26 are intentional stubs.

After selection, from the repository root with the qualified compiler:

```powershell
python -B build_scripts/test-debug-bindings.py --fbc build/main-gcc/bin/fbc.exe --prefix build/main-gcc --tool-dir C:/freebasic-js/toolchain/ucrt64/bin
```

For the before/after comparison, additionally supply `--reference` with the
retained 838B6658 compiler. The driver reuses one temporary directory and removes
its C, object and executable slots. Candidate receipts use
`generated-c-debug-bindings-v3-*` in the existing canonical work directory;
`coverage-final` supersedes the earlier high-level variable classification.

<!-- end of c-emitter-debug-bindings.md -->
