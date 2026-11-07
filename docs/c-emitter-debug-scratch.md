<!--
Project: FreeBASIC C backend
File: c-emitter-debug-scratch.md
Purpose: Explain dead scratch removal in debug output and its qualification.
Responsibilities: Distinguish compiler storage from source debug bindings and
record the complete diagnostic counts, native controls and remaining boundaries.
This file intentionally does NOT equate retained warnings with compiler faults.
-->

# Compiler scratch storage in debug output

Debug mode previously disabled scalar dead-store analysis for the entire
procedure. That preserves named BASIC variables for a debugger, but also kept
unread compiler temporaries and their assignments. GCC consequently diagnosed
internal storage that had no source-level identity.

`c-unused-scalar-stores.bi` now admits compiler scratch candidates in debug mode
when the procedure has no opaque assembly. Named variables, formal parameters,
function results, escaping addresses, hidden return storage and owned lifetimes
retain their existing contracts. Other passes still see the conservative debug
policy: this change does not enable general local, descriptor or parameter
removal. Unread scratch stores use the existing dependency and evaluation proof,
including retention of calls, conversions and object construction/destruction.

No warning attribute, diagnostic suppression or dummy read is introduced. BASIC
diagnostics 54 through 57 remain enabled under their established warning levels.
Programs and library sources in the corpus are unchanged.

## Complete current results

The selected development compiler is `build/main-gcc/bin/fbc.exe`, SHA256
`838b6658a6ae3099839beba3e819b15b27886c316de44141d889d23859ea5a3a`.
The preceding 5C78E4AD compiler is retained for immediate rollback. Installed
release binaries and normal ST/MT archives are unchanged.

| Group | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Compiler | 224 | 0 | 1,338 |
| fbctests | 703 | 0 | 777 |
| Broad production | 1,129 | 73 | 953 |
| Broad additional | 239 | 140 | 1,654 |
| Exampleageddon checked | 1,775 | 3 | 2,727 |
| Exampleageddon default | 1,775 | 3 | 2,606 |
| Total | 5,845 | 219 | 10,055 |

The complete GCC sweep removes ten unused-store observations. Only five
production/additional C outputs change; 1,319 others, all 703 fbctests outputs
and all 3,490 emitted Exampleageddon outputs match the preceding complete
census. The separate 1,368-profile normal sweep retains exactly three warnings
and its complete C/native-diagnostic binding is unchanged. The compiler reaches
a fixed point across 224 emitted-C units with GCC0; 223 units match 5C78E4AD.

Both complete native diagnostic queues pass all 5,739 open C definitions.
Clang removes the same ten unused-store observations. Compared with the last
complete BF344DD7 census, its total falls by four because the preceding source
diagnostic repair also exposed six existing-layout padding observations.
Neither this repair nor those padding observations add a new warning class.
Both compiler C generators retain 1,338 Clang observations each.

## Runtime and debug controls

The new public regression reuses six existing BASIC controls without changing
them. It passes 192 native GCC/Clang checks, 192 native value runs and twenty
debug binding checks across both C generators, O0/O2, checked builds and ST/MT.
Win32 objects are compiled but not executed. Named WATCHONLY and USERVALUE
bindings remain present in emitted C and in GCC's O0 DWARF information.

The controls verify call counts and order, integer/float conversion, UTF-8 line
input, one tracked allocation, constructor/destructor ownership, references,
static arrays, incoming branches and opaque assembly. The Win32 parser check
recognizes its stdcall spelling without modifying the compiled C. Native flags
include `-masm=intel`, matching the BASIC assembly selection.

An older graphics-free debug driver could not link with the minimal qualified
prefix because its object carries an unused fbgfx link directive. The new driver
links the actual native dependencies with the same qualified runtime archives.
No graphics library, stub or alternate runtime is substituted, and the actual
native value controls pass. This resolves that test-driver boundary explicitly.

The complete GCC backend suite passes in 172.111 seconds without a whole-suite
deadline, including all 420 source-warning controls. Strict lint reports zero
errors and warnings for the changed BASIC include. Compiler structure passes
408 files and 21 host graphs; unchanged runtime structure remains qualified.
The selected public path also passes automatic SDK emission, GCC0, linking and
runtime checks with no explicit include or prefix arguments.

From the repository root on the qualified Windows toolchain:

```powershell
python -B build_scripts/test-debug-scratch.py --fbc build/main-gcc/bin/fbc.exe --prefix build/main-gcc --tool-dir C:/freebasic-js/toolchain/ucrt64/bin
```

Use `--reference <previous-fbc.exe>` to compare native warning classes against
another compiler. One temporary directory is reused and removed for the run.

## Remaining work

The 219 GCC observations remain counted: 176 unused parameters, 24 unused
variables, five unused-store observations, four unused functions, one range
comparison, one containing-record copy observation, two checked formatter
observations and six deliberate exception-demo accesses. Compiler temporaries
are kept conservatively where assembly, descriptors or other ownership contracts
remain unproved. The named unread static retains its opt-in BASIC warning.

The previous source-warning review still distinguishes source mistakes from
implicit parameters, intentional feature stubs and verified native false
positives. PNG, native runtime and closed DOS assembler work remain excluded.
No corpus application or deliberate fault example was executed. The broader
warning campaign remains active.

Receipts use the `generated-c-debug-scratch-*` prefix in the existing canonical
work directory. Historical diagnostic and object bytes remain reconstructible;
lossless shared compression and retirement of the older development image keep
retained storage below 160 MiB. No source-tree copy is made.

<!-- end of c-emitter-debug-scratch.md -->
