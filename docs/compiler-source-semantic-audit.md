<!--
Project: FreeBASIC compiler verification
File: compiler-source-semantic-audit.md
Purpose: Record the compiler-source semantic audit and the defects it exposed.
Responsibilities: Identify coverage, fixes, verified binaries, and retained evidence.
This file intentionally does NOT contain compiler implementation or a cross-platform build claim.
-->

# Compiler source semantic audit

Verified on October 1, 2026, following the
[OMA semantic audit](semantic-sidecar-verification.md). All 166 compiler BASIC
source files in the frozen tree were checked with GCC, LLVM, and Clang emission.
The Linux x86-64 Make graph selects 164 files. The remaining DOS floating-point
policy and RISC OS floating-point representation files were checked using their
own Make-selected x86 and ARM target configurations.

Both an optimized compiler and a compiler built with assertions were used.
Sources, headers, Make files, compilers, and independent validators were frozen
before each run. Recorded input hashes were checked again after the audits.

## Defects found and fixed

### Reused callback signatures lost their preliminary declarations

Parameter declarations can receive semantic bindings before their anonymous
procedure-pointer header is interned. When an equivalent signature already
existed, the unused header and its parameters never entered the final symbol
inventory. Their exported identities consequently lacked completed metadata.
Callback tables in `backend/ir.bi` exposed this across all three backends and
both compiler builds. The independent reader rejected the original probes with
`Symbol 2501 lacks completed metadata` despite their valid completion footers.

`symbAddProcPtr()` now reports the selected canonical signature before returning
it. The exporter records canonical relationships for the unused header and its
corresponding formal parameters. It checks parameter counts and table lengths,
and does not describe an unfinished header as a completed ABI signature.

The dedicated regression checks repeated signatures, BYVAL/BYREF/array formals,
variadic signatures, and different default arguments. It fails on the original
compiler and passes after the correction.

### LLVM emitted uninitialized literal padding

The first complete audit found a separate failure in `symbols/symb-define.bas`:
full export changed a wide literal's emitted LLVM initializer. Its last four
bytes contained stale buffer contents instead of zeroes. Both compiler builds
reproduced the difference.

The LLVM emitter used the symbol's storage length as the decoded input length.
The shared unescape buffer initializes decoded characters and their terminator;
storage reserved beyond those characters must be supplied by the emitter.
Literal emission and static string initialization now use the decoder's actual
length. Remaining wide and zero-terminated storage is explicitly zero-padded;
fixed byte strings retain their space padding.

A small LLVM regression checks literal bytes, declared sizes, a static wide
initializer, and identical code across all three export modes. On glibc,
`MALLOC_PERTURB_=165` exposes padding reads that fresh zero-filled pages can
hide. The original compiler emitted allocator fill bytes; the corrected one
passes with this setting. The complete compiler-source audit was then rerun
with the final binaries.

## Results

| Check | Optimized compiler | Compiler with assertions |
| --- | ---: | ---: |
| Native module/backend cases | 492/492 | 492/492 |
| Additional DOS and RISC OS module/backend cases | 6/6 | 6/6 |
| Distinct compiler BASIC files | 166 | 166 |
| Compiler-source invocations across three export modes | 1,494 | 1,494 |
| Semantic contract test methods | 47 passed | 47 passed |
| OMA program/backend cases | 36/36 | 36/36 |
| OMA invocations across three export modes | 108 | 108 |
| Existing escape/fixed-string tests, per backend | 16 passed | 16 passed |
| Assertions in those string tests, per backend | 388 passed | 388 passed |

Every compiler-source case validates full and compact completion, identity and
metadata closure, dependencies, physical source ranges and UTF-16 columns,
module identity, backend context, and full/compact expression parity. Emitted
code is byte-identical with export disabled, full export, and compact export.
Together the source cases exercise 16 exported symbol classes and 30 lowered
AST kinds. Counts include repeated headers in independent module models.

The new Make entrypoint was exercised with the previously failing LLVM module.
The compiler structure check also passed for 226 documented files and 21 host
source graphs. The existing test-harness status/failure/timeout checks passed.

Verified compiler SHA-256 values:

```text
optimized:  75be16918826a188b8393d7a08cd10bc4fd1f9069ee3147681acdaca7f3d29c4
assertions: 826a77504c15dc03570a6c67704e83448c5240c66602771e6e6c1d3b9eb60d44
```

## Repeat the audit

```sh
make compiler-semantic-self-test
```

The runner queries Make for its source graph and compiler flags. It uses
independent backend source trees and worker processes to keep generated files
and reader state separate. Large artifacts can be retained in compressed form:

```sh
python3 build_scripts/test-compiler-semantic-self.py --fbc bin/fbc \
    --output out/compiler-semantic-self --jobs 4 --compress-artifacts
```

`--unit` and `--backend` select smaller runs. `--target-triplet` selects other
host policies through Make. The Make target accepts these options through
`COMPILER_SEMANTIC_SELF_FLAGS`. For example:

```sh
python3 build_scripts/test-compiler-semantic-self.py --fbc bin/fbc \
    --target-triplet arm-unknown-riscos --unit platform/riscos/fp-bits.bas
```

Retained evidence is under `out/compiler-semantic-audit-20261001/`:

- `initial-probe-failures.json`, original sidecars, and before/after regressions;
- `native-*-symb-define.diff`, showing the original LLVM padding discrepancy;
- `final-native-*`, `final-dos-*`, and `final-riscos-*` input hashes, commands,
  code hashes, sidecars, and per-case results;
- `verified-compiler-audit-summary.json`, the checked source coverage and totals;
- `contracts-final-*`, `final-oma-*`, and `string-regressions` evidence;
- `final-make-integration.log`, `final-structure.log`, and `harness.log`.

## Coverage limits

These are semantic and emission checks for the selected source configurations.
They do not link an LLVM-built compiler or execute DOS/RISC OS binaries. Other
host policies and inactive preprocessor branches require separate target runs.
Passing these models is not a proof for every possible BASIC program. The
native string regression programs were separately linked and executed on Linux.

<!-- end of compiler-source-semantic-audit.md -->
