<!--
Project: FreeBASIC runtime verification
File: c-emitter-cleanup-error-state.md
Purpose: Explain error-state preservation during abandoned activation cleanup.
Responsibilities: Record the runtime contract, regression evidence and remaining limits.
This file intentionally does NOT claim complete managed ownership or private promotion.
-->

# Error state during cleanup, November 4, 2026

The runtime now saves the complete error context around each abandoned-frame
cleanup callback. Ordinary array erasure sets `ERR` to zero on success; user
destructors can also change error and source metadata. Those temporary changes
must not replace the handled error which the surviving owner observes.

The snapshot is taken after the frame is popped. This preserves the surviving
handler token rather than restoring a token whose metadata was just retired.
Each callback gets its own snapshot, so later callbacks see the same selected
state. The callback still runs before the one-way owner transfer and can release
the allocation which contained its frame. Normal scope destruction retains its
existing behavior. There are no public layout or signature changes.

The change is in `src/rtlib/errors/error_frame.c`. The matching private provider
under `build/c-emitter-cleanup-error-state-20261104/runtime-source/` applies it
to both the C transfer and owner-metadata cleanup paths. A small outlined helper
keeps the context copies out of the traversal optimizer. The initial inline
GCC probe failed without retaining its terminal status and is superseded;
its exact failure timing is not claimed. Final GCC and Clang probe compiles
take less than one second. No deadline was extended.

## Regression evidence

The original valid BASIC reproducer returns handled error 24 but leaves caller
`ERR` at zero. With the repaired object linked before the unchanged runtime
archive, both C compilers return 24 and retain 24.

The permanent `tests/compiler-support/c-error-cleanup-state.c` fixture checks
reverse callback order, null callbacks, storage freed by callbacks, all error
context fields, nested handlers, intentional post-handler `ERR` changes and
thread-local state. Thirty-two current executions pass across canonical/private
providers, GCC/Clang, GNU11/GNU23, O0/O2 and ST/MT runtimes. They cover 1,536
transfer cases and 192 private owner-metadata cases. Four unchanged-provider
controls fail as expected. No native handler or native jump is invoked by this
fixture.

The BASIC fixture of the same name exercises allocated and empty numeric and
string arrays, fixed string arrays, scalar strings and plain-object destructors.
Sixty-four executions pass with both C emitters and compilers, both standards
and optimization levels, `-e`/`-exx`, and ST/MT runtimes: 8,192 object retirements
and 8,192 handled errors. Their emitted C has zero GCC/Clang warnings. Two old
runtime controls fail at the first caller-visible error assertion.

The test distinguishes legitimate state changes before a throw from cleanup
side effects. Successful nested `REDIM`/`ERASE` changes `ERR` before the inner
throw saves its caller state. The first draft incorrectly expected the earlier
24 in that case; a small diagnostic control established the zero result and
the assertion was corrected. Production code did not change to accommodate it.

The existing `tests/rtlib/error-frame.c` test now deliberately changes `ERR`,
line and module during transfer cleanup, then verifies the original handled
state. Its standard driver passes all 64 object checks and 48 execution
expectations, including threaded and allocation-failure configurations. Four
independent old/current executions also establish that the integrated test
fails with the original runtime and passes with the repair.

The existing private ERROR suite is freshly executed against an observed build
of the new unit: 132 successes and eight expected illegal-RESUME exits. Its 32
current compiler emissions match the qualified C exactly. These are fresh
runtime checks, not inherited executions. They retain retry/next, ordinary
recovery, scope order and shared-code resumption coverage.

Eight matched before/current module comparisons at Win32/Win64 find unchanged
public export sets and no new warning observations. Ninety-one SDK headers
are bound. Four current private/canonical Win32 module objects and the standard
driver's Win32 objects are compile checks; Win32 execution is not claimed.
Strict source-only Windows lint passes both BASIC fixtures.

## Scope and reproduction

The permanent canonical regression runs through the existing driver:

```powershell
python build_scripts/test-error-frame-runtime.py `
  --gcc C:/freebasic-js/toolchain/ucrt64/bin/gcc.exe `
  --clang C:/freebasic-js/toolchain/ucrt64/bin/clang.exe `
  --runtime-dir build/c-emitter-c-gosub-20261103/prefix/lib/freebasic/win64 `
  --output build/error-frame-cleanup-recheck
```

Use a fresh output directory. The stage contains exact commands, compressed
logs, source preimages, diagnostics and input hashes for the expanded API,
language, ABI and ERROR matrices. The qualified private compiler remains
`ffeefd3e` with `error-rebase/prefix-thin-r2`; tests explicitly override the
changed runtime object. Archives and installed binaries are unchanged.

This runtime source repair changes no selected emitted-C recipe. Last complete
private counts remain 247 GCC/6,501 Clang in the stable broad queue and 126/7,747
in Exampleageddon. The earlier object-array corpus source delta retains its
qualified receipt. Queue input and compiler pins are revalidated rather than
repeating a full sweep. Existing SDK warnings stay enabled and are separate
from the selected emitted-C totals.

The stage stays below 32 MiB with one heavy worker, reusable scratch and no
copied assets or archive members. General managed/partial construction and
private ERROR promotion remain open; the unsupported-cleanup boundary is
unchanged. Existing examples are unchanged. PNG and user-closed optional/native
investigations remain excluded.

<!-- end of c-emitter-cleanup-error-state.md -->
