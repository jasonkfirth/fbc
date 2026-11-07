<!--
Project: FreeBASIC compiler verification
File: c-emitter-managed-fields.md
Purpose: Record generated aggregate cleanup and the remaining packed-layout defect.
Responsibilities: Separate completed runtime evidence from unfinished compiler acceptance.
This file intentionally does NOT claim complete managed ownership or promotion.
-->

# Generated managed fields, November 4, 2026

The private compiler candidate can retire generated managed aggregates when
construction stops partway through. Its eligibility check requires generated
special members, a single direct default-constructor call and fields which can
be destroyed from zero storage. It excludes user special members, references,
uninitialized fields, unions, inheritance and dynamic arrays with element
constructors or destructors.

The caller already zeroes owned storage before frame registration. A generated
destructor can therefore release initialized string and array fields while
untouched owning fields remain empty. The cleanup callback now accepts the
initializing state for these aggregates and clears ownership before invoking
destruction. Existing plain objects keep their live-only rule. Normal scope
destruction retains its existing behavior, including successful array erasure
setting `ERR` to zero. Independent predecessor/current controls establish that
behavior rather than changing the compiler to satisfy an incorrect test.

The implementation remains private under
`build/c-emitter-managed-fields-20261104/source/`. Candidate `88308429` closes
the complete 224-unit compiler fixed point. Five units change; diagnostics stay
at zero GCC and 1,652 Clang warnings, with aggressive diagnostics enabled.

## Completed checks

The main matrix passes 64 executions with both C emitters and compilers,
GNU11/GNU23, O0/O2, `-e`/`-exx` and ST/MT runtime links. It checks 7,168 handled
errors, including 2,560 outer transfers and 2,560 local exits during partial
construction. All 89,984 recorded allocation events have matching releases.
There are no outstanding blocks and no emitted-C warnings. MT links are not
claimed as concurrent execution.

The permanent fixtures are
`tests/compiler-support/c-error-managed-fields.bas` and its C allocation
observer. Sixteen fresh executions include eight trap-instrumented runs and
eight runs with array storage allocated by generated field initializers before
a later initializer fails. Four Win32 program objects pass; execution is not
claimed. Direct packed managed fields also pass four instrumented executions
and four Win32 object checks.

Six guarded resource classes retain twelve exact predecessor C identities and
96 expected unsupported-cleanup exits. Those refusals are not successful
unwinds. Two backend gates, seven procedure-state tests and strict lint of
three BASIC files pass. Thirty-two ERROR-suite C identities bind the previously
executed 140 expectations to this compiler and the same repaired runtime; they
are not fresh executions in this batch.

The runtime tests explicitly override the repaired error-frame and string/object
retirement objects. Archives and installed binaries are unchanged. Before
controls still reach the old unsupported-resource exit with the valid aggregate
fixture. Source preimages, raw statuses and superseded test drafts are retained.

## Unfinished packed layout

A nested packed managed record exposes an independent structural defect. The
minimal `nested-packed-layout.bas` has no ERROR handler or partial construction.
Both canonical and predecessor native compilers execute it successfully with
an inner size of 152, outer size of 154 and child offset one. Canonical,
predecessor and candidate C output all fail the C size/field-offset guards
under both GCC and Clang: six confirmed compilation failures.

The emitter skips a nested member's packing attribute when the child's declared
`FIELD=N` matches its parent. Managed descriptor fields can give the actual C
child type a larger alignment. The emitted parent then places that child at a
different offset from the compiler's symbol layout. The larger managed probe
shows child offsets 25 versus 32 and fixed-child-array offsets 249 versus 256.
The guards are doing their job and must remain enabled.

This defect predates the owner candidate. The candidate remains unpromoted and
full managed/packed acceptance is unfinished. The next repair must preserve
the declared layout and verify descriptor accesses in nested packed storage.
Changing valid BASIC or weakening the layout checks would conceal the defect.
General user-constructor progress and dynamic object-array construction also
remain open.

The latest complete broad and Exampleageddon counts still belong to predecessor
`ffeefd3e`: 247 GCC/6,501 Clang and 126/7,747 respectively. They are not new
candidate counts. An attempted archived-C cohort inventory found missing C
payloads in an older receipt. Its incomplete inventory is not accepted coverage.
A later normal refresh can compare pinned C hashes and compile changed units;
first finish the shared packed-layout repair to avoid another premature sweep.

## Storage and resource handling

C: filled during the first compiler link, while this batch retained about
6.6 MB. Compiler work stopped. A closed 1.1 GB folder of compressed results
and JSON receipts was relocated to E:, with all 2,361 hashes verified and a
junction preserving its original path. There is one retained physical copy.
Source trees and installed binaries were not moved. The active compiler work
folder and tool temporaries also use E: through a preserved path.

The recovered build re-emits every compiler unit, rebuilds the five changed
units and proves a subsequent full fixed point. Its retained stage-zero
compiler hash, failed-link receipt and separate recovery logs are recorded.
The component retains a 32 MiB cap, one heavy worker and a 512 MiB free-space
floor on the source volume. Existing examples are unchanged. PNG and the
user-closed optional/native ERROR investigations remain excluded.

## Later packed-descriptor repair

The nested 152/154-byte layout and ordinary descriptor-access failure recorded
above are resolved by the [compiler/library follow-up](c-emitter-packed-descriptors.md).
That repair is in canonical source and excludes the private ERROR ownership
providers. This historical candidate receipt remains unchanged; general
partial construction and ownership promotion are still open.

<!-- end of c-emitter-managed-fields.md -->
