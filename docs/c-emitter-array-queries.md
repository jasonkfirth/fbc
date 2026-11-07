<!--
Project: FreeBASIC runtime verification
File: c-emitter-array-queries.md
Purpose: Record physical-storage identity and dimension-query repairs.
Responsibilities: Explain descriptor semantics, regression coverage and remaining limits.
This file intentionally does NOT claim completion of all emitted-C warnings.
-->

# Array queries, November 2, 2026

An allocated byte array can have a virtual DATA address of zero. Its physical
storage remains nonnull. LBOUND, UBOUND and the checked-dimension runtime entry
previously mistook that descriptor for an unallocated array. The ordinary BASIC
reproduction uses REDIM PRESERVE on just two bytes, choosing the physical
address as the lower bound. Both the C API and BASIC reproductions failed
before this repair.

The three native query/check files and their complete BASIC translations in
C:/fb_corpus/fbrtLib now test PTR for allocation. LBOUND and UBOUND also retain
the supplied 1-based dimension until it has been checked. Only a valid positive
dimension is converted to the zero-based descriptor index. The former early
subtraction overflowed for the minimum native signed integer in C.

Dimension zero retains the existing interface: LBOUND returns one, and UBOUND
returns the allocated dimension count or zero for an empty array. Invalid
dimensions still return zero/-one. Descriptor fields, exported names, calling
conventions, index-error messages and dispatch remain unchanged.

## Regression evidence

The permanent driver is `tests/compiler-support/runtime-array-queries.py`, with
matching C and BASIC fixtures. It compiles private query objects before linking
the existing archive, and uses the repaired producer objects for ordinary BASIC
callers. It does not install a compiler or runtime.

The matrix covers native C and complete GCC/Clang BASIC emissions, both C
compilers, GNU11/GNU23, O0/O2 and ordinary/instrumented runs. Native C uses
-fno-wrapv plus pointer/signed-overflow traps. BASIC C retains -fwrapv and uses
pointer-overflow traps. The original aggressive diagnostic flags remain active.

- 336 runtime API runs pass for zero origins, ordinary bounds, empty arrays,
  extreme dimensions, checked dimension identity and existing index checks.
- 24 ordinary BASIC caller runs pass, including -exx checked indexing, preserved
  element bytes, dimension-zero queries, minimum dimensions and ERASE.
- 216 query/fixture object checks pass, including 24 Win32 objects. No Win32
  execution is claimed. Four instrumented native producer objects support the
  BASIC caller checks in addition to that count.
- Seven historical failures are reproduced, with fourteen historical success
  controls. Current and historical exported symbol sets match for nine Clang
  Win64 comparisons. The SDK dependency closure binds 89 headers.

The C error-dispatch observer is single-threaded and does not install a global
runtime handler. Ordinary diagnostic strings exercise the existing formatting
and dispatch interface. This packet does not qualify long-message formatting
in the older BASIC translation.

Strict source-only Windows lint covers the three translated files and the new
BASIC fixture: zero errors and one pre-existing presentation-whitespace finding.
The three saved translation preimages had four findings; adding the required
file footers removes their missing-final-newline findings. No semantic-lint
completion is claimed.

The input-bound receipts, refreshed selected warning counts and reversible
source changes are under `build/c-emitter-array-queries-20261102`. Its summary
separates native runtime C diagnostics from the selected emitted-C queues.
The preceding [producer arithmetic repair](c-emitter-runtime-origins.md) records
the shared representation and allocation/copy checks.

## Running the focused driver

From the repository root, provide a private compiler and runtime prefix, GCC,
Clang, the corpus runtime directory, the original diagnostic profile database,
and the saved query source preimages. The work directory is reused one C file,
object set and executable at a time, with a 32 MiB retained-work guard. Each
subprocess has a timeout and runs below normal priority on Windows.

```powershell
& 'C:/freebasic-js/toolchain/ucrt64/bin/python.exe' `
  tests/compiler-support/runtime-array-queries.py `
  --fbc build/c-emitter-array-origin-20261102/accepted/fbc.exe `
  --prefix build/c-emitter-integration-20261028 `
  --gcc C:/freebasic-js/toolchain/ucrt64/bin/gcc.exe `
  --clang C:/freebasic-js/toolchain/ucrt64/bin/clang.exe `
  --corpus C:/fb_corpus/fbrtLib `
  --work-dir build/c-emitter-array-queries-20261102/contracts `
  --baseline-dir build/c-emitter-array-queries-20261102 `
  --diagnostic-profile build/c-emitter-active-20261101/builtin-current-compiler-audit/results.sqlite3
```

The overall campaign remains open for allocation-size limits, broader pointer
views, cross-procedure ERROR handling, remaining warnings and corpus coverage.
Twelve PNG profiles remain excluded, and installed binaries are unchanged.

<!-- end of c-emitter-array-queries.md -->
