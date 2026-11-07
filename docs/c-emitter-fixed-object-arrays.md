<!--
Project: FreeBASIC fixed managed object-array qualification
File: c-emitter-fixed-object-arrays.md
Purpose: Record the private fixed-array lifetime contract and real parser qualification.
Responsibilities: Preserve source semantics, diagnostic counts and remaining boundaries.
This file intentionally does NOT claim canonical promotion or general object unwinding.
-->

# Fixed managed object arrays, November 5, 2026

The private C owner now retires fixed arrays of zero-safe records whose element
constructors and destructors are compiler generated. This covers the remaining
real Scorched Earth cleanup guard: the RTF parser's 64-element
`RtfViewParserState groupStack` array. Its owning field is `table_font_name As
String`. The parser and all existing examples remain unchanged.

Private compiler `4675f92a80acf999aa22c4ab2ec0939db38eb57c5024870d4ee6e4891875978c`
extends the [scalar copy candidate](c-emitter-copy-owners.md). A separate field
walk excludes user constructors, including nested records. Existing exclusions
for user destructors, inheritance, unions, references, uninitialized owning
fields and dynamic object fields remain. Dynamic object arrays have a separate
allocation and initialized-element contract and stay guarded.

The owner clears both fixed payload and descriptor before registration. Heap
captures already use zeroed allocation; borrowed captures explicitly clear both.
An error before the declaration sees an empty descriptor. Once the declaration
establishes the descriptor, generated destruction can visit every physical slot:
unfinished fields are still zero-safe. This also covers an initializer interrupted
while ERASE reconstructs an array after retiring its old contents.

Retirement uses the ordinary AST variable-destructor builder and C array callback
adapter. It therefore retains the existing generic descriptor view, typed element
method and erased runtime callback ABI. Statement normalization stays in the AST
procedure unit, where `astUpdate` is private. The C emitter receives the prepared
tree. The runtime implementation and public record layout are unchanged.

A local handler can leave an inner scope before reaching its ordinary destructor
call. The owner performs a final generated-array sweep before leaving its frame.
Generated field retirement is idempotent and enters no user destructor body; a
normal scope exit has already cleared its owning fields. Scalar lifetime states
and constructor hooks apply only to complete scalar objects, not array elements.

The permanent regression checks a two-dimensional array with nonzero lower
bounds, errors before declaration and at each of 24 generated field initializers,
outer and local handlers, ordinary exits, repeated scopes and fixed-array ERASE.
Generated controls add a first child at offset zero, FIELD=1/2/4/8 layouts and
interruption during ERASE reconstruction. The real-record control retains the
parser's exact declaration and bound; its emitted size is 216 bytes. It checks
partial population and independent strings against an unchanged source record.
These controls do not parse documents, initialize graphics or load images.

Qualification receipts are in `build/c-emitter-fixed-object-arrays-20261105`:

- The complete 224-unit C fixed point retains 0 GCC and 1,652 Clang observations.
- The main 64-run matrix is warning-free under both C tools, GNU11/GNU23, O0/O2,
  checked modes and ST/MT links. It handles 13,312 errors and balances 1,027,584
  allocations/releases. One storage-guard stop resumes 33 completed executions
  and runs the remaining 31; it does not duplicate or discard the completed work.
- Eight instrumented permanent-fixture runs balance 143,152 allocations.
- Fifty-six instrumented layout/ERASE runs balance 1,250,896 allocations, with
  another 28 after-only Win32 program objects. Their unsuppressed ABI and GNU
  extension observations remain counted.
- Eight instrumented real-record runs handle 8,448 errors and balance 1,168,976
  allocations. Their source record remains unchanged.
- Thirty-four newly emitted whole-C identities bind 272 inherited scalar and
  real-copy executions to their unchanged inputs. These are inherited evidence.
  ERROR-suite scoped-array C changes, so that suite requires fresh execution
  instead of an identity claim.
- The fresh 140-expectation ERROR matrix passes: 132 successful executions and
  eight expected illegal-RESUME exits, without added warning classes.
- Nine remaining resource classes preserve 18 exact predecessor/current C
  identities and 144 expected refusals. These are not successful unwinds.
- All 32 paired program objects, including 16 Win32 objects, preserve every
  warning map. The real object control checks 27 native integers, matching
  216 bytes on Win64 and 108 bytes on Win32.
- Both backend suites, mixed-header LTO controls, 14 semantic tests and the
  canonical structure guard pass. Strict lint of eleven compiler/test providers
  reports zero findings. Eighteen generated controls retain nine required-output
  recommendations and two formatting recommendations from embedding the exact
  CRLF parser declaration in their LF templates; no findings are suppressed.

The fresh successful total is 268, excluding smoke runs and expected refusals;
72 runs are instrumented. Directly observed array/record matrices balance
3,590,608 allocations/releases. The 272 inherited scalar/record executions are
reported separately. There are 32 paired objects and 28 additional after-only
Win32 objects; Win32 execution is not claimed.

Instrumented runs cover the program and four explicitly linked native units for
error frames, array retirement and the bounded allocation observer. Eight pinned
instrumented native objects are reused. The complete runtime archive is not
instrumented. ST/MT links are single-threaded controls, not concurrent execution.

The real graphics translation retains 352 cleanup callbacks. A full inventory of
plain and attributed definitions confirms one guarded callback becomes zero;
the five earlier scalar-copy repairs stay supported. GCC completes in 53.827
seconds with zero warnings, and Clang in 36.095 seconds with its existing 74
padding and one reserved-identifier observation. Both use the original 90-second
deadline and unchanged aggressive diagnostics. No causal speedup is claimed.

The candidate remains private. User element lifetimes, dynamic object arrays,
inheritance/unions, aliases, broader constructor shapes, native/platform partial
construction, profiling and actual concurrent execution remain guarded or
unqualified. Win32 evidence covers objects, not execution. No new whole-queue
census is inferred from this focused qualification. PNG and user-closed
optional/native ERROR investigations remain excluded.

Dirty canonical compiler/library sources, installed binaries and project sources
are preserved. Scratch and compressed receipts stay under the stage's 32 MiB
retained and 64 MiB transient physical allocation limits, with a 512 MiB free
space floor. The large real translation is serialized before the smaller
qualification matrix resumes.

<!-- end of c-emitter-fixed-object-arrays.md -->
