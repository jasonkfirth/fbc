<!--
Project: FreeBASIC C backend qualification
File: c-emitter-byte-memory.md
Purpose: Record containing-object addresses at native byte-memory calls.
Responsibilities: Bind compiler changes, native controls and production counts.
Application build and runtime qualification retain their own receipts.
-->

# Native byte-memory arguments

Both C emitters retain the enclosing object when a field address is passed to
the matching native `memcpy`, `memmove` or `memset` declaration. A scalar field
can begin a larger byte range within its owner. Emitting its scalar subobject
address caused GCC to report an incorrect array bound for that range.

The matcher checks the direct native name, C calling convention, three BYVAL
parameters, pointer result, pointer arguments, signed 32-bit fill value and
unsigned pointer-width length. Existing constant internal-memory spans retain
their range checks. Unknown call lengths acquire no additional range proof.

At offset zero the address uses the evaluated owner directly. At a nonzero
offset an anonymous byte-record view supplies the displacement and a flexible
byte tail. The flexible tail preserves the actual allocation extent, including
larger allocations that begin with the declared record. Const sources qualify
the enclosing byte view. The origin is detached before the old expression is
released and is evaluated once. Counts, values and native declarations retain
their original semantics.

## Qualification

The fixed-point compiler `6634540c` is selected through
`build/main-gcc/bin/fbc.exe`. Forty-four public-path regression executions and
four default-prefix/include executions pass. Protected installed products and
runtime archives retain their recorded hashes. All 224 compiler C units reach an exact
fixed point with zero GCC warnings. Compiler Clang retains 1,340 observations.
All 703 fbctests retain exact C identities and GCC 0 / Clang 771. The complete
backend suite passes in 635.37 seconds.

The registered BASIC/C regression checks guarded byte ranges, overlapping
moves, native returned pointers and origin evaluation counts. It also checks
a 24-byte operation inside an actual 32-byte allocation containing a smaller
record prefix. The 128 native executions perform 16,512 transactions across
Windows and Linux, both pointer widths, both producers, GCC and Clang, O0/O2,
and ordinary/debug builds. All 128 diagnostic compilations preserve rejection
of a genuine fixed-object overrun. Ninety-six actual cross-target objects cover
twelve ELF, COFF and Mach-O contexts.

Intermediate candidates exposed lost overrun diagnostics, const propagation,
Clang pointer/index observations and an incorrect fixed tail bound. Those
candidates remain unselected. The flexible-tail implementation passes GCC
14/16 and Clang 19/22 controls. Source-only lint retains the regression's
required PASS output marker as its single style finding. A separate default
compiler-backed lint invocation reports unavailable initializer/type-hint
closure facts and eight findings. Compiler-backed consumer qualification remains
pending. The source-only result has zero errors and one finding.

## Production replay

All 1,053 port emissions preserve their FBC warning lines. Exactly 1,036 retain
their previous C identity. Fresh paired Linux native compilation of all seventeen
changed units completes without errors under the original unsuppressed options.

| Queue | Profiles | GCC | Clang |
| --- | ---: | ---: | ---: |
| Coreutils | 498 | 187 | 1,830 |
| SQLite | 555 | 21 | 7,696 |

SQLite loses two false array-bound observations in `sqlite3_bulk_where_begin`
and `sqlite3_where_clause`. Coreutils loses one Clang byte-address observation.
Every other native warning class is preserved. All seventeen Clang objects and
fourteen GCC objects remain byte-identical.

The three differing GCC objects reproduce their recorded hashes. The SQLite
instruction change exchanges signed comparison operands and the corresponding
branch direction. Coreutils head/tail change one and two generated `memcpy`
address lines respectively. Their zero-offset buffer addresses now retain the
same evaluated enclosing origins. GCC's stack/register allocation changes when
those owner addresses escape. The archived C and disassembly differences bind
this review. Full application execution remains pending.

Templari's preceding paired production graphs cover 661 profiles with GCC 2 and
Clang 52,665 under either C producer. That source cut preserves all 1,172 opened inputs
and a ten-file project delta. Twenty-eight original-path predecessor controls
cover fourteen changed/new profiles. The two compiler-changed emissions have
fresh native controls with no added warning classes.

Concurrent edits overtook seven further controls. Fourteen predecessor/current
comparisons reconstruct their exact captured source bytes in a temporary tree
and produce identical C within that common context. Relocation changes filename
constants and temporary identities, so those fourteen comparisons qualify the
relocated source context. Their original recorded C identities remain separate.
The first relocation checks correctly rejected an exact-identity claim.

The subsequent [ABI observation qualification](c-emitter-abi-observations.md)
refreshes the normal current graph to 664 profiles at GCC 2 / Clang 53,111.
It captures all 1,177 opened inputs and checks all current padding/registration
observations. The selected compiler image retains the same `6634540c` identity.

The [additional Windows project refresh](c-emitter-additional-projects.md)
completes Midisoft 146, Rotator3d 16, FS4 125 and HART 153 standalone objects
through this selected compiler. All 440 have zero GCC warnings. The audit
recovers five HART public-include contexts previously refused before GCC ran
and verifies one generated implementation body through seven public readers.
The portable source cut contains all 977 opened inputs and recipe files.

The wider replay completes all 6,419 configured profiles through this compiler
using streamed parent evidence and a 256 MiB owned-artifact limit. It records
GCC 12 / Clang 15,862 with no stale sources or new frontend regressions. Its
owned peak is 132,636,482 bytes. Existing PNG exclusions remain in force.
Finite/runtime format contracts, current whole-runtime rebuilding and the
compiler-backed semantic consumer remain in the cleanup queue.

## Receipts

Receipts reside under `build/c-emitter-forward-views-20261105`.
The `byte-arguments-selfhost-v8`, `suite-v4`, `native-*-v6`, `cross-targets-v1`,
`production-linux-v1`, `object-review-v1` and `port-replay-v2` receipts dated
`20261115` bind this batch. Source delivery, development-main selection and
ordinary default-prefix entry checks use the same `byte-arguments` prefix.

<!-- end of c-emitter-byte-memory.md -->
