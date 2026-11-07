<!--
Project: FreeBASIC C backend
File: c-emitter-callback-conventions.md
Purpose: Record effective callback conventions and reproducible long-double temporaries.
Responsibilities: Distinguish address transfers, actual ABI differences and qualification.
This file intentionally does NOT claim a fresh complete corpus census.
-->

# Callback conventions and temporary names, November 5, 2026

The shared callback representation helper now recognizes ordinary CDECL,
STDCALL and Windows-MS tags that the C emitter does not distinguish outside
x86. A generic callback remains a stored code address. Its later typed call
still owns the argument/result ABI. Real x86 convention changes, typed variance
and static initializers retain their existing paths and diagnostics.

CDECL also selects native long-double arguments and results. The helper checks
that separate ABI property before accepting a convention-tag difference; those
actual long-double boundaries remain visible. The original examples and library
callbacks keep their signatures.

The boundary probe exposed three existing temporary-name defects in ir-hlc.bas.
symbUniqueId returns a pointer to generated text; STR(pointer) used the address
of its shared buffer as a suffix. Native arguments, native results and return
values now copy the identifier text and request names valid in BASIC and C.
The existing counter is still consumed once per name. Independent compiler
processes now emit identical C for all three paths.

The permanent c-generic-callback-values.bas fixture adds ordinary STDCALL and
Windows-MS coverage where those tags share the native convention. It checks
foreign dispatch, address identity, typed restoration, nulls, one-time operand
evaluation and lazy branches. Its one completion marker has a narrow fblint
annotation; GCC and Clang diagnostics remain unsuppressed.

The new tests/compiler-support/c-longdouble-temporary-names.py guard is called
by the normal backend runner. It checks independent C emissions and portable
identifiers, and reports which conversion paths the target actually emitted.
It needs the compiler, repository SDK and an existing scratch parent; no private
runtime archive is needed because it does not link. For example, from this tree:

```text
python -B tests/compiler-support/c-longdouble-temporary-names.py --fbc <compiler> --prefix <toolchain-prefix> --work-dir <existing-scratch-directory>
```

Main compiler 753e8335 and experimental 4d0bcc3b each reproduce all 224 compiler
C units and their qualified native objects at zero GCC and 1,506 Clang compiler
observations. A separate 224-unit main-source binding removes the qualification
overlay and verifies the actual promoted Root helper. The experimental compiler
retains its private source providers and the existing RTL-only _setjmp policy.
Its source/runtime consolidation and clean external rebuild remain unproved.

All eight affected original profiles per cut pass unchanged aggressive GCC and
Clang flags. Each cut removes twelve Clang strict function-pointer cast
observations, with no warning-class increase. GCC counts remain unchanged.
These are cohort results; the earlier complete census remains its dated cut.

Qualification also passes:

- 64 generic-callback C executions across the two compiler cuts.
- 64 long-double value C executions using the unchanged existing value fixture.
- Eight repeated exact long-double C identities covering all three naming paths.
- Thirteen boundary cases and 52 object compilations across Win64 and Win32.
  Eleven preserve exact before/after C. The two long-double cases preserve all
  code except the intended temporary identifiers, and independently reproduce
  the candidate C exactly. Their real ABI-cast diagnostics stay present.
- Four complete backend suites, two namespace suites, fourteen existing private
  semantic contracts and the structural check. The new name guard participates
  in each backend suite.
- Strict source-only lint of the three changed BAS/BI files: zero errors and
  warnings. The Python guard and runner execute successfully in those suites.

Existing long-double startup, exact-value-comparison and C90 declaration-style
diagnostics remain visible where emitted by the unchanged value/boundary
fixtures. Both supported C dialects, GNU11 and GNU23, accept the actual C.
Installed binaries, archives, valid examples and unrelated dirty work remain
preserved. The 10,899-file frozen-source check separates twelve accumulated repair
revisions from a later concurrent inc/fbgfx.bi revision. That unrelated header
edit is preserved. Fresh compiler and cohort C bindings retain the same exact
outputs; the concurrent graphics feature changes are outside this qualification.

Receipts and preimages are retained in the existing E-drive crt-candidate area
under E:/fbc-c-audit-20261105-forward-views-r2-qualification. The callback-* files
separate this component from immutable earlier results. Each candidate reuses
223 objects. Retained data is bounded at 96 MiB, temporary outputs stay on E,
and at most two qualification controllers run concurrently. No dated directory
ladder or installed replacement is created. The broader cleanup remains active.

<!-- end of c-emitter-callback-conventions.md -->
