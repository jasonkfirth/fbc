<!--
Project: FreeBASIC compiler qualification
File: c-emitter-anonymous-layout.md
Purpose: Record anonymous container and mixed C declaration layout repair.
Responsibilities: Bind source, ABI, runtime, diagnostics and permanent regression coverage.
This file intentionally does NOT declare the whole warning campaign complete.
-->

# Anonymous C record layout, November 4, 2026

Valid materialized records fail emitted-C size or field-offset guards when they
combine managed fields with anonymous containers, or give nested anonymous
containers their own FIELD=N. The previous flat repair excludes any anonymous
member, and the older field writer applies outer packing to every promoted leaf.
Both GCC and Clang reproduce failures under natural alignment and FIELD=1/2/4/8.
SIZEOF-only tests can miss the defect because no C record needs to be emitted.

The compiler now separates field spelling from explicit layout ownership.
`c-explicit-record-layout.bi` walks the original parent links of promoted leaves,
accounts for each anonymous child's complete extent, and preserves that child's
own packing. Every named field and existing size/offset assertion remains.
Bitfield byte bodies retain their existing guarded path. BASIC layout calculations,
constructors, destructors, native libraries and installed binaries are unchanged.

Natural C alignment is computed before writing a container. Packing is applied
only when the emitted members require it. A named gap is added only when C's
implicit padding would place the next member at the wrong offset. This preserves
the required descriptor gaps without changing otherwise compatible C member lists.
An earlier representation passed layout/value and native ABI checks but failed
GCC LTO against an ordinary C header because of an unnecessary padding member.
That representation is superseded; its logs and source preimages remain separate.

The final canonical compiler is
`7ec02c4e90ccc91192b2b4180d71e0cc42fe0494d525bbbb569fa0c26f939ed4`.
The full 224-unit compiler C fixed point changes only the HLC object and retains
zero GCC / 1,652 Clang warnings. No private ERROR ownership provider is included.

Qualification covers:

- 80 fresh alignment-instrumented Win64 executions of the final representation,
  across natural alignment, four packings, both C producers, GCC/Clang, O0/O2
  and ST/MT links, with zero alignment reports;
- 88 Win32 native objects and 40 Win32 program objects; execution is not claimed;
- 40 fresh C/GAS64 mixed-module executions, in both directions, checking four
  argument types and three return types, including small trivial records;
- 20 mixed-header GCC LTO executions, treating type mismatches as errors;
- 110 raw C variants and 20 C-header fixture variants freshly bound to the
  canonical rebuild, including the prior 50 unchanged flat-record variants;
- both regular backend suites, seven retained procedure-state checks, the
  compiler structure guard and strict BASIC lint on the compiler and fixtures.

The permanent `c-packed-anonymous-descriptors.bas` fixture materializes inherited
and explicit anonymous layouts, multiple unions, managed and trivial values,
initialized storage, wrappers and fixed/dynamic arrays. The permanent
`c-anonymous-header` BAS/C/Python fixtures exchange a small record with an ordinary
C declaration under GCC LTO before the normal runtime link. This reproduces the
earlier mismatch and passes with the final compiler. Both C producers run from
the regular backend runner. Clang LTO is not claimed. MT tests use one thread.

All audit C diagnostics remain aggressive and unsuppressed. Real ABI padding,
intentionally underaligned packed members and GNU control-flow observations remain
visible. The final matrix has no unnecessary-packed warning. Matrix observations
are not whole-corpus counts. The [paired census](c-emitter-packed-queues.md) still
belongs to bf893b74/f6b0905a; its private ordinary GCC time gate remains failed.

General managed/partial construction, private ERROR ownership, user constructor
and destructor behavior during cross-frame errors, alias activation and dynamic
object-array construction progress remain open. This component does not qualify
all combinations involving named bitfield children, C++ adapters or unselected
platform SDKs. PNG and user-closed optional/native ERROR checks remain excluded.
Concurrent Optical edits are preserved under their separate census boundary.

The class comparison still prioritizes continuation ownership and the shared
unused-parameter family. Legitimate ABI padding and required native spellings
must not be removed just to lower a count. No example or project source is edited.

Receipts, exact preimages and compressed logs are in
`build/c-emitter-anonymous-layout-20261104`. One early controller receipt was
saved under a distinct R3 name after completion and its original predecessor
restored byte-for-byte. Historical backend-runner bytes are archived because the
permanent LTO gate was added after compiler and semantic qualification.

Outputs and temporaries use E:. Aggregate retained stage storage, including
all preserved candidates and failed controls, is capped at 48 MiB across both
volumes, with at most two heavy workers and a 512 MiB free-space floor. Existing
compiler objects are reused; no runtime archive or asset tree is copied.

<!-- end of c-emitter-anonymous-layout.md -->
