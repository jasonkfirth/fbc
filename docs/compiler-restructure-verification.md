<!--
Project: FreeBASIC compiler
File: compiler-restructure-verification.md
Purpose: Record the compiler restructuring and the scope of its verification.
Responsibilities: Identify the changes, test inputs, results, and remaining limits.
This file intentionally does NOT contain: a claim of complete backend or platform parity.
-->

# Compiler restructuring and verification

Date: 2026-10-01. Execution host: Linux x86-64.

The compiler audit covered all 208 original compiler files, including the
license, totaling 173,164 lines. The make audit covered 48 makefiles totaling
8,905 lines. The original file inventories, hashes, and relocation map are
retained with the local verification artifacts.

## Structure and ownership

The directory map and compilation flow are in
[the compiler structure guide](../src/compiler/README.md). The changes include:

- Subsystem directories for the driver, lexer, preprocessor, parser families,
  symbols, AST construction and optimization, runtime call builders, emitters,
  host policies, support storage, and compiler tooling.
- Includes rooted at `src/compiler`, so an include identifies its owning
  subsystem. Source headers describe purpose, responsibilities, and exclusions;
  footers identify their files.
- A driver split into separately compiled option, file, tool, compilation,
  link, help, and target hook modules. The main driver went from 5,851 lines
  to 303 lines. Invocation state and the tool catalog each have one owner.
  The private driver header is unavailable to parser and backend modules.
- Small DOS and RISC OS arithmetic policies in place of duplicated AST,
  casting parser, driver, and helper implementations. Compiler host arithmetic
  is selected independently of the target of the user's program.
- One canonical make source graph in `mk/compiler-sources.mk`, shared by
  native, variant, and bootstrap builds. It rejects duplicate basenames before
  flat object or bootstrap output names can select the wrong source.
- Checked allocation arithmetic shared by containers and compiler character
  buffers. Growth from zero or one item, pointer alignment, length addition,
  capacity rounding, and terminator storage have explicit checks.
- A public semantic observation header and a private serialization header.
  Parser, symbol, and AST code report compiler-selected facts through the public
  hooks; exporter buffers and traversal helpers remain inside `tooling`.

The container review retained distinct lifetime policies. A reusable node
pool, an append-only arena, a LIFO stack, a size-class allocator, a hash table,
and an ordered string set are used differently. Their shared allocation checks
now live outside the list implementation. Replacing them all with one list
would change invalidation and ownership rules.

The string review separated compiler character buffers from language types.
Dynamic strings, fixed strings, terminated byte strings, wide strings, and
the incoming UTF-8 string implementation have different storage and runtime
contracts. Byte and wide compiler buffers share capacity checks while retaining
their element widths and terminators.

## Build and focused regression coverage

The structural checker validates documented source coverage, include resolution,
private header boundaries, and the actual make source graph for 20 hosts.
The latest working-tree check covers 225 compiler `.bas` and `.bi` files.

The permanent make checks are:

```sh
make compiler-structure-test
make compiler-host-policy-test
make compiler-storage-test
make compiler-test-harness-test
make compiler-backends-test
make compiler-semantic-model-smoke
```

The first five are part of the quick and full make workflows. Numeric policy
tests exercise three implementations with each available native backend.
Storage tests cover small-pool growth, reset and reuse, aligned payloads,
string buffers, and seven rejected size cases per backend. Harness tests prove
that failed, incomplete, and timed out language-test logs fail make.

The backend checks compile and execute typed C aliases, builtin addresses,
straight-line assembly, large C aggregate calls, and `va_list` calls. Aggregate
tests run at both the default optimization and `-O 3`, including values from
packed fields. The LLVM check also inspects retained IR to require aligned
storage for `byval` source pointers. Five Clang target checks validate generated
C builtin prototypes for Linux, Darwin, FreeBSD, OpenBSD, and NetBSD.

DOS and RISC OS bootstrap emission each produced all 164 selected compiler
C files. These are source emission checks; cross-platform execution was not
performed. RISC OS target smoke checks also inspect emitted target code.

## Fixes found during verification

| Finding | Change and regression coverage |
| --- | --- |
| QB recursive macro evaluation could hang during error recovery | Return the evaluation error to the outer macro parser instead of attempting token recovery after popping the inner lexer context; the QB language suite exercises both byte and wide lexer fixtures. |
| C aliases could declare incompatible types under the same C identifier | Keep distinct internal identifiers and use the requested external ABI name on the emitted declaration; typed alias tests run with all three backends. |
| Clang rejected addresses of builtin-only identifiers | Use ordinary internal declarations with the original external symbol name; builtin address tests execute on all three backends. |
| Clang inline assembly and builtin prototypes differed from GCC | Correct Intel operand syntax and the native `__builtin_bswap64` declaration policy; assembly and five target syntax checks cover them. |
| Clang's generated-C uninitialized analysis was expensive on large OHR files | Disable that warning after `-Wall`, while allowing an explicit later user flag to enable it; the runner checks flag order. |
| LLVM split consecutive assembly instructions and used address registers for memory operands | Emit a straight-line assembly sequence together, use memory constraints, declare register and memory clobbers, and reserve a frame without a red zone when assembly uses the stack; register and flags tests execute. |
| LLVM passed large System V C aggregates incorrectly | Share parameter policy across declarations, calls, and parameter storage; preserve `va_list` array decay and copy aggregate arguments into aligned local storage. C aggregate and variadic tests execute at two optimization levels. |
| Native `USTRING` interfered with existing macros and user-defined types | Preserve macro shadowing and add `FB_NO_USTRING` for legacy type declarations; macro and global/namespace alias tests are permanent language tests. |
| An incompatible redeclaration of a runtime string descriptor helper caused a null dereference | Propagate conversion failure before inspecting the rewritten argument; separate literal and fixed-string negative tests require a normal compiler diagnostic. The original `fbrtLib` failure now exits with status 1 instead of runtime error 12 on all three backends. |
| Semantic smoke validation expected the older record set and footer width | Validate the added metadata record shapes and footer total while retaining the existing binding, lifetime, and source-range assertions. |

The string descriptor crash also reproduced with the saved compiler from
before the restructure. Its small reproductions remove the builtin declaration,
replace it with an incompatible argument type, and pass a string to `KILL`.
This isolates the compiler failure from `fbrtLib`'s missing Linux headers.

The LLVM aggregate alignment change follows the
[LLVM parameter attribute contract](https://llvm.org/docs/LangRef.html#parameter-attributes):
`byval` alignment constrains its source pointer as well as the callee's stack
copy. Successful unaligned reads on x86 are insufficient to validate that IR.

## Verification inputs

The large comparison used a frozen compiler, include tree, runtime archives,
compiler sources, test inputs, examples, OMA sources, and corpus source list.
The compiler banner is `1.20.4-3`. Installed tools were GCC 15.2.0 and
Clang/LLVM 21.1.8.

Final comparison compiler SHA-256:

```text
1f6a6a09eecd8bec4cdc8666787065d241d026383c79d56f83340403df87f6c5
```

Native `USTRING` and semantic exporter work continued in the shared workspace
during the comparison. Later edits were preserved. The saved source manifest
identifies the exact inputs tested; this report does not assign the frozen
matrix results to later feature edits. The public semantic hook boundary and
updated semantic smoke check were additionally built and checked in the live
tree.

Normal project builds were completed with the preceding comparison compiler
`dcf224b92554f27a1438834106a4ed5905ffd0bdff69db35a9184050b3461c7f`.
The final comparison added the checked descriptor conversion failure path
and repeated the main unit, language, example, OMA, and corpus matrix.

## Native backend results

| Check | GCC | LLVM | Clang |
| --- | ---: | ---: | ---: |
| fbcunit tests passed | 2,422 | 2,421 | 2,422 |
| fbcunit assertions passed | 1,616,894 | 1,616,891 | 1,616,894 |
| Language log tests passed | 1,822 | 1,822 | 1,822 |
| Example sources compiled, out of 1,679 | 1,480 | 1,485 | 1,480 |
| Example executions passed | 647 | 647 | 647 |
| OMA build and startup/smoke checks passed | 12 | 12 | 12 |
| Corpus object compilations passed, out of 2,122 | 949 | 952 | 949 |
| Unique corpus programs linked, out of 102 | 82 | 82 | 82 |

Each language total includes 1,728 FB, 86 QB, three fblite, and five deprecated
cases. There are no failed or missing result records. LLVM excludes the existing
unit test that jumps from inline assembly to BASIC labels. It accounts for the
one-test, three-assertion difference.

The example and corpus manifests match between backends. Every GCC success
also succeeds with LLVM and Clang. LLVM additionally compiles five variadic
examples and three VisualFBEditor COM implementation units. These are compile
results; they do not establish working Windows executables on this Linux host.

Exampleageddon reports zero execution failures and zero self-contained failures.
Other examples are classified as platform-specific, interactive, helper modules,
external-library dependent, intentional failures, audio, network, or benchmarks.
The CSV records each classification; the table does not imply that all 1,679
examples compiled or ran.

OMA covers nine games and three kinematics demonstrations. Graphics checks
require a visible window and a live process for four seconds before stopping
the process. Nietzsche and Quest for a King also pass their script tests.
OpenMarket passes its four command-line smoke modes. Screenshots and logs are
retained. This checks startup and the available scripted paths, without a
complete gameplay or visual equivalence claim.

The corpus sweep covers 44 projects. Sources are compiled as separate objects;
many are platform-specific, project fragments, or rely on build-time definitions.
The same command definitions are used for each backend. OHR and VisualFBEditor
receive `FB_NO_USTRING` for their existing `UString` types. This retains the
949 GCC successes from the earlier corpus baseline.

The linked corpus checks use project-aware source groups, language selections,
includes, and libraries. Twenty targets fail for all three backends with shared
source, binding, or dependency problems. They include Oregon Trail and BRPG
syntax/duplicate declarations, outdated raylib bindings, missing Tilengine and
raylib libraries, and existing source names that require `FB_NO_SFXLIB`.
The deterministic `basicverse/world_test.bas` and `fbJson/examples/test.bas`
programs execute successfully on all three backends.

## Normal project builds

| Project | GCC, LLVM, and Clang result |
| --- | --- |
| fbfrog | Build, 5,519 unit tests, and header translation pass. |
| fb-ext-lib | Normal serial library build passes. Test linking reaches the system's missing `-lpcre` dependency. |
| FBSound | Full library build and Linux smoke test pass, including WAV, MP3, Ogg, MOD, and SID paths. |
| fb-VNC | Build and project test pass. |
| OpenSesh | Normal editor build passes. |
| OHRRPGCE | Game and editor build at `debug=3`; both `--version` executions pass. Its wrappers use `FB_NO_USTRING`. |
| fbrtLib | Shared unsupported/missing Linux header definitions block the normal build. Its separate compiler crash is fixed and covered by regression tests. |

OHR's existing GCC 15 optimized-build internal error is outside this result;
`debug=3` verifies the unoptimized project build. `fb-ext-lib` test execution
cannot be claimed while its PCRE link dependency is missing.

## Remaining limits and evidence

LLVM still needs control-flow lowering for assembly that branches to BASIC
labels. The large aggregate ABI coverage is System V x86-64; it does not prove
all small aggregate classifications or other platform calling conventions.
Clang cross-target checks validate C syntax without cross-platform execution.

The live semantic smoke passes after the record-format update. The frozen
compiler's full model checks pass, but its compact multi-module export retains
only one of three expected dependencies and therefore fails that additional
smoke assertion. Later live exporter work fixes that check; its evolving
independent contract suite is recorded separately from the frozen backend matrix.

The separately frozen live followup ran 36 semantic contract test methods and
reported three failures: primitive string storage metadata for x86 and x86-64,
and the static target of a virtual call. These belong to the concurrent exporter
feature work and remain outstanding in that run. Its compiler hash is
`acb8852ee7b09c208331ec44018d279b156da86e7bb895e1195c4499885f64b3`;
the compiler and test inputs are retained alongside its log. The native build,
structural check, and established semantic smoke passed in the live tree.

A subsequent [semantic sidecar audit](semantic-sidecar-verification.md)
verifies fixes for all three failures with newer frozen compilers. It also
records additional exporter and LLVM procedure-address corrections and full
OMA semantic comparisons across GCC, LLVM, and Clang. The older frozen results
above remain evidence for their original compiler versions.

Local evidence is under
`out/compiler-restructure-20261001/verification/`. It includes test summaries,
per-source CSVs, build and run logs, OMA screenshots, the tested compiler,
source hashes, and the compiler source/build snapshot. Normal project logs are
under `out/compiler-restructure-20261001/projects-final/`.

<!-- end of compiler-restructure-verification.md -->
