<!--
Project: FreeBASIC C backend GCC qualification
File: c-emitter-gcc-terminating-context.md
Purpose: Record the terminating error-location save repair and exact GCC scope.
Responsibilities: Identify source ownership, native evidence and remaining counts.
This file intentionally does NOT claim a fresh complete Clang census.
-->

# Current GCC result after terminating-context cleanup

The selected development compiler is `build/main-gcc/bin/fbc.exe`, SHA256
`d7b308376c5adf7e92d5a5dbbe434332dd579cf396e19fbcee36a5fef518284f`. The installed release binaries and normal ST/MT libraries are unchanged.
All 10,922 source pins agree across the complete GCC queues.

| Group | Configured profiles | GCC warnings |
| --- | ---: | ---: |
| Compiler | 224 | 0 |
| fbctests | 703 | 0 |
| Broad production | 1,129 | 77 |
| Broad additional | 239 | 146 |
| Exampleageddon checked | 1,775 | 3 |
| Exampleageddon default | 1,775 | 3 |
| Total | 5,845 | 229 |

Both generated GCC warnings in `examples/manual/faq/dos/lowmemas.bas` are fixed.
The remaining six Exampleageddon observations are the three deliberate null writes
in `examples/misc/trycatch/test.bas`, checked/default. They remain counted under
unsuppressed diagnostics; the example is unchanged and was not executed.
Both C generators also pass fresh GCC assembly-output-only checks for the DOS
source. The closed DOS assembler is not invoked.

All 224 compiler units self-host at an emitted-C fixed point with zero GCC
warnings. All 703 fbctests outputs match the prior qualified C exactly and retain
zero GCC warnings. The full GCC backend suite passes in 181.521
seconds without a whole-suite timeout. The original context control passes 32
native value runs and four debug C identities. Its final test-only refinement
retains every assertion through declared CRT string comparisons: eight further
native values pass, and both C generators are GCC-clean on Win32/Win64. The
changed suite case is separately requalified. Win32 evidence covers definitions
only. The fixed development SDK view uses include/freebasic junctions to existing
headers, matching this normal build layout. Its canonical header-using regression
emits, compiles with GCC0, links and runs without -prefix or -i. No headers are copied.
Returning and early-returning routines still restore the caller's error location.

The separate normal production/additional replay has 3 GCC observations
across 1,368 profiles. Original debug/source-contract observations stay in the
table. Every prior exclusion/frontend/closed-native disposition is unchanged.
Native runtime C remains the separate deferred ST/MT/startup queue.

The preceding naming repair retains native exports, initialization hooks and the
GUI weak marker: 24 fixture and 88 corpus section/relocation/binding comparisons,
including 70 byte-identical objects, plus 16 native value runs. Differences in
other objects are GCC's COFF external-symbol order/type metadata for asm aliases.
The 22-profile Clang check removes 36 reserved-identifier, four duplicate startup
and one missing-declaration observations. The final GCC repair reuses that backend
object and is additionally covered by the current backend and context suites.

The complete Clang census has not been refreshed for this compiler. Its last
complete result remains 10,491 observations on D7676A44. Focused improvements are
not subtracted from that historical full count. GCC remains the current priority.
The overall warning goal remains ACTIVE. D7676A44 is the immediate rollback.

Details: [repair and evidence](c-emitter-gcc-terminating-context.md).

The AST procedure owner `src/compiler/ast/nodes/ast-node-proc.bas` keeps
both location-setting calls. In a nondebug, nonnaked C body, the existing
label/assembler-aware `astProcMayReturn()` proof can show that caller restoration
is unreachable. Only then are the old-location save slots omitted. Ordinary
returns, early returns, debugger bindings and other backends retain their contract.
No dummy reads, diagnostic attributes or disabled warnings are introduced.

The regression `tests/compiler-support/c-terminating-context.bas` checks current
function/module names, ordinary and early-return restoration, opaque but portable
x86 assembly, and END in a callee. Both C generators and native compilers cover
ST/MT values; debug C matches the predecessor exactly. The runner uses `-exx`.
The reserved-name fixture expects both main and destructor success lines.

Qualification controllers and compressed receipts share the existing
`build/c-emitter-forward-views-20261105` / canonical qualification directories.
The authoritative receipts are `generated-c-terminating-context-build-v3`,
`-values`, `-suite`, `-structure`, `-fbctests-v2`, `-examples-v3`, `-broad-v2` and `-publish-v3`
with `.json.zlib` suffixes. The final support-fixture amendment preserves all
compiled corpus bindings and diagnostics: the changed test is neither a corpus
profile nor included by one. Its current hash is separately qualified. Source edits and runner-only input amendment are
journaled independently. Historical receipt compression reconstructs every
original JSON and compressed-receipt hash; it does not discard diagnostics.

<!-- end of c-emitter-gcc-terminating-context.md -->
