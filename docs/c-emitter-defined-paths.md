<!--
Project: FreeBASIC compiler verification
File: c-emitter-defined-paths.md
Purpose: Record the remaining compiler-source uninitialized-path repairs.
Responsibilities: Explain source ownership, failure handling and qualified scope.
This file intentionally does NOT claim the overall C warning cleanup is complete.
-->

# Defined compiler paths

The canonical compiler source now has explicit failure/absence paths for the
remaining uninitialized-value diagnostics exposed by the C audit. The full
189-module compiler graph has zero such GCC or Clang diagnostics with the
aggressive unsuppressed profiles. Other warning families remain required work.

## Source contracts

| Area | Change |
| --- | --- |
| Runtime INPUT, PRINT, WRITE and string conversions | Reject unsupported integer widths before constructing a call with an unassigned runtime procedure. Unsupported raw-string descriptor requests return the existing NULL failure result. |
| INPUT argument ownership | Calculate string length before consuming the destination AST. Finish scalar input on its own path, so no scalar path can read a nonexistent length argument. |
| Call AST loading | Keep argument AST nodes until register-argument type reads finish. Give an indirect call its own optional operand and capture the dispatch choice before releasing the function-pointer expression. |
| LLVM call results | Start the optional result operand from the supplied result vreg. Allocate a replacement only when required. |
| Procedure headers and parameters | Initialize absent previous declarations/operator metadata and period position for unnamed parameters. |
| Parser context | Capture the previous expression context before nested parsing and restore it on the same existing option path. |
| Symbol lookup | Return directly on a matching accessible symbol, with NULL for a missing chain; no separate match flag controls a later candidate read. |
| Internal discriminants | Reject unknown C IR kinds, literal-token kinds and unsupported method-owner classes rather than continuing with unassigned values. The C error path retains a traversable recovery expression after reporting an internal error. |

The changes are focused source decisions. They do not blanket-initialize
uninitialized BASIC storage or add a C-warning suppression. Unsupported internal
IR/token/owner guards are defensive paths; arbitrary corrupt-IR execution is not
claimed as qualified. All supported paths are exercised below.

## Full compiler qualification

Receipts are in `build/c-emitter-audit-20261022/defined-paths-v47/`.
Twelve changed BASIC compiler modules compile with both C compilers. Both
complete 189-object compiler executables link with the same pinned complete v43
standard Win64 runtime. The current-source comparison has 756 accepted C object
stages, reusing 354 unchanged baseline controls and 348 byte-identical after-C
objects. All 378 complete public-symbol sets match.

The before/after producer comparison uses the same revised BASIC source graph.
Its 374 C pairs are byte identical. The other four differ only in explicit
build-date literals in `fbc-help` and `symb-define` after the calendar changed
from October 24 to October 25. Those exact differences are retained and verified;
no instructions or other C text are normalized away.

The source-repair delta from the preserved v46 graph is:

| Diagnostic | v46 source | v47 source |
| --- | ---: | ---: |
| GCC maybe-uninitialized | 9 | 0 |
| Clang conditional-uninitialized | 9 | 0 |
| Clang sometimes-uninitialized | 10 | 0 |
| GCC total warnings | 30,634 | 30,625 |
| Clang total warnings | 63,459 | 63,442 |

Clang also reports two additional C90 declaration-after-statement observations
for the new call-state locals. They remain in the totals and are not suppressed.
The general C declaration-layout warning family remains unfinished.

## Runtime, recovery and project checks

- Sixteen old/new native O0/O2 executions pass under matching GCC and Clang.
  They cover INPUT/PRINT/WRITE scalar widths, dynamic/fixed/zero/wide/Unicode
  strings, string-to-number conversions, callbacks, record results, constructor
  and destructor counts, operator casts, properties and unnamed prototypes.
- Eight Win32 C object checks preserve four public-symbol pairs, including the
  register-argument source path. No Win32 runtime execution is claimed.
- Eight source-model packets validate, and all four before/after pairs are byte
  identical. The two full C compiler builds also emit identical regression C.
- Sixteen malformed-source checks retain identical diagnostics and ordinary
  failure exits for unnamed UDT formals, invalid operators, missing constructor
  prototypes and unavailable casts. The constructor fixture was corrected to
  include record storage, so its final check reaches declaration recovery rather
  than an earlier empty-type rejection; the earlier receipt is retained.
- Normal, escaped and literal string-token preprocessing produces identical
  output under the old, prototype and both full compiler builds.
- Thirty-two exact-C comparisons carry the already-qualified runtime/Win32,
  fbsound, assembly and four real fbfrog-module checks to the final compilers.
  Their sources, C hashes, tools and runtime pins are retained. Duplicate C
  copies were removed only after proving identical preserved originals.
- The two adopted regressions reproduce their qualified C byte-for-byte. Eight
  executions of the corresponding qualified binaries pass after adoption.
- Strict nonsemantic Windows lint covers all fourteen changed/new BASIC files
  with zero errors, warnings or informational findings.

An initial source-preparation error lost three BASIC comment markers. Six BASIC
emission checks failed before producing C. The markers were repaired, their logs
were retained and the eighteen successful unchanged compiler objects were reused.
No failed check is treated as an accepted compiler result.

## Ownership and remaining work

`source-adoption.json` records exact preimages for twelve compiler source files
and the new `tests/quirk/c-defined-runtime.bas` and `c-defined-calls.bas` tests.
The compiler and runtime installations, canonical AST procedure source and error
ABI remain protected. No commit, push, installation or corpus recopy was done.
The private work stays below 128 MiB. At the final build boundary no large compiler
cohort remains running.

The repository structure guard still reports eleven existing marker failures in
seven separate semantic-tooling files. None belongs to the files changed here.
Alignment, qualifiers, C declaration layout and other C warnings remain, along
with array-origin contracts, general error-owner/lifetime lowering, remaining
assembly cases and broader project/platform qualification. The private v43 error
outliner must be rebased onto the current v47 source, retaining the intervening
assembly, identifier, dispatch and defined-path repairs.

<!-- end of c-emitter-defined-paths.md -->
