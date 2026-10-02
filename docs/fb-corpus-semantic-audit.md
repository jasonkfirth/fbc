<!--
Project: FreeBASIC compiler verification
File: fb-corpus-semantic-audit.md
Purpose: Record semantic audits of external projects with working GCC builds.
Responsibilities: Describe coverage, reproduced defects, fixes, and audit limits.
This file intentionally does NOT claim that every corpus program or platform works.
-->

# External project semantic audit

Verified October 2, 2026, against `/home/jkfirth/fb_corpus`.

The subsequent [source-to-export content review](fb-corpus-semantic-content-review.md)
compares real BASIC constructs with their records and identifies missing
information. The matrix below checks artifact validity and emission parity;
semantic sufficiency is assessed separately in that review.

| Emission backend | Complete audit passes | Full-export capacity failures | Compact export passes |
| --- | ---: | ---: | ---: |
| GCC | 354/357 | 3 | 357/357 |
| LLVM | 354/357 | 3 | 357/357 |
| Clang | 354/357 | 3 | 357/357 |

These are 1,071 invocation/backend cases and 3,213 compiler invocations across
disabled, full, and compact export. All nine full-export failures are the
documented staging limit in AZdecrypt, Tiko, and fb-linter. After controlling
fb-linter's timestamp, every compact model passes with unchanged emitted code.
No other model or code-parity failures remain in the final matrix.

## GCC build controls

The current sources build 82 of 103 selected standalone targets with GCC.
The same targets pass before and after the compiler fixes below. Another 21
targets fail their ordinary source, dependency, or backend build controls and
are excluded from semantic-export comparisons.

All six normal project builds pass with the fixed compiler:

| Project | Normal build | Captured source invocations |
| --- | --- | ---: |
| fbfrog | Make build | 24 |
| fb-ext-lib | Serial Make build, 64-bit | 152 |
| FBSound | Linux 64-bit Make build | 4 |
| fb-VNC | Make all | 1, containing 7 modules |
| OpenSesh, midisoft-fb | Editor build script | 1, containing 42 modules |
| OHRRPGCE | SCons game and custom, debug=3 | 93 |

Together with the linked targets, these controls select 357 compiler
invocations, 413 module occurrences, and 389 distinct root translation units
across 35 projects. Different game/editor definitions remain separate cases.
OHR's generated BASIC inputs are retained alongside its ordinary modules.
Its known GCC 15 optimized-build failure is outside the debug=3 control.

The source snapshot contains selected project directories and their maintained
headers/assets. It excludes Git metadata and previous build products. OHR's
supported source-archive `revision.txt` supplies revision 14331, calculated
before discarding Git metadata. Empty object directories required by fbfrog's
Make recipe are prepared before its first build.

## Defects reproduced and fixed

### Wide symbol tables exhausted a nesting stack

Hackit includes enough Windows prototypes to exhaust the exporter's 4,096-entry
pending-table stack. The old traversal queued every sibling's child table, so
this limit measured table breadth as well as nesting.

The symbol walker now visits each child immediately and saves the parent's
continuation. Its existing allocation and nesting bound remain checked. The
new 4,500-prototype regression fails with the old compiler and passes with the
fix, checking every procedure signature and formal parameter. Hackit passes
the GCC, LLVM, and Clang semantic checks after this change.

### Temporary initializer scopes lost their metadata

OHR modules exposed symbol references to temporary initializer scopes that had
already been unlinked from the symbol table. They had identity records but no
completed metadata, so the independent reader correctly rejected the model.

The AST scope code now exports those scopes and their contents while ownership
is still live. A small default UDT argument reproduces the missing metadata
with the old compiler. The regression and focused OHR checks pass with the fix.

### Legacy bytes produced incorrect editor columns

AZdecrypt contains legacy text bytes that resemble UTF-8 leading bytes. The
old counter treated a lone byte such as `F0` as two UTF-16 units and skipped
some malformed continuation prefixes, shifting subsequent source ranges.

The lexer and source-bound checker now share the byte-column counter. It
validates continuation sequences, overlong encodings, surrogate encodings, and
the Unicode maximum. Valid UTF-8 retains UTF-16 columns; malformed prefixes
count each legacy byte separately. Regression coverage checks full, bindings,
and compact exports, exact following identifier positions, and code parity.

## Validation and remaining limits

The matrix emits each invocation through GCC, LLVM, and Clang with export
disabled, full export, and compact expression export. It checks the independent
reader, metadata/reference closure, dependency closure, source bounds, module
identity, backend identity, full/compact expression agreement, and exact emitted
code hashes within each backend. Backend artifacts are private to each case.
Captured working directories and explicit main-module roles are preserved.

The final source revision check verifies 167,196 exported file revisions from
2,133 published full/compact models against 1,392 distinct frozen files. Their
byte counts and SHA-256 hashes match. All 389 root source hashes and 16 generated
BASIC/include hashes also remain unchanged after the matrix.

AZdecrypt, Tiko, and fb-linter exceed the documented 256 MiB full-export staging
budget. The compiler rejects these models instead of publishing false
completion. Their ordinary GCC builds pass; compact export is checked
separately. These capacity failures are retained as failures of full export.

fb-linter embeds `__DATE__` and `__TIME__` in its banner. Its first comparison
observed different timestamps. A separate GCC build and three-backend audit
hold `SOURCE_DATE_EPOCH=0`; the new runner sets a fixed epoch for future runs.
Time changes are not classified as compiler code-generation regressions.
The primary evidence retains the uncontrolled comparisons. `verified-results.json`
uses the separately verified fixed-timestamp linter results; it still counts
those full-export capacity failures.

The frozen 74-method semantic contract suite passes with both optimized and
assertion-enabled fixed compilers. Compiler structure checks pass for 235
documented files and 21 host source graphs. Focused compiler-source and OMA
checks also pass after extending the shared audit helper.

Runtime smoke checks build and execute fbJson's self-test and BASICVERSE's
headless renderer with GCC and LLVM. All four runs return success. The three
rendered PPM files have identical bytes between the two backends. The larger
matrix stops at compiler emission; GUI behavior and every project's runtime
are outside this audit.

## Reproduce

Capture fresh GCC controls and audit the successful projects:

```sh
make compiler-semantic-projects-test \
  COMPILER_SEMANTIC_PROJECTS_FLAGS="--corpus /home/jkfirth/fb_corpus --fbc ./bin/fbc --output out/fb-corpus-audit --jobs 3"
```

The recipes live in `build_scripts/fb-corpus-semantic-projects.json`. Select a
project with `--project`, capture controls alone with `--prepare-only`, or
replay a frozen `--snapshot`. The runner accepts `--backend`, `--case`, and
`--resume`; resume requires the same compiler, validator, helper, plan,
selections, and timestamp setting. Completed models and emissions are compressed
without changing their bytes.

Evidence is retained under `out/fb-corpus-semantic-20261002/`:

- GCC build logs, captured invocations, exact flags, and linked binaries;
- source/header/runtime/validator snapshots and SHA-256 manifests;
- before/after regressions and debugger traces;
- `final-audit/` models, emissions, logs, and per-mode results;
- `verified-summary.json`, `verified-results.json`, and `project-summary.csv`;
- fixed-timestamp fb-linter controls and runtime smoke results;
- private optimized/assertion compiler builds and contract-suite logs.

The final optimized compiler has SHA-256
`2cbccd0248e197c3787b84915a0c72d33fbc7d89dd15cf4eb7058aa92452dbc8`.
Separate manifests retain the assertion compiler, earlier binaries, and their
source/runtime inputs. These are private audit builds; the report's results
identify these binaries rather than an independently changing system compiler.

Earlier interrupted attempts retain their own evidence. Their main-role and
working-directory harness failures are excluded from final compiler findings.
Concurrent compiler work is isolated through the frozen inputs.

<!-- end of fb-corpus-semantic-audit.md -->
