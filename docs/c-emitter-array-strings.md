<!--
Project: FreeBASIC runtime verification
File: c-emitter-array-strings.md
Purpose: Explain bounded reverse retirement of array string elements.
Responsibilities: Link native/BASIC changes, ABI checks and selected queue evidence.
This file intentionally does NOT claim general object or partial-construction unwinding.
-->

# Array string retirement, November 3, 2026

The native runtime and its corpus BASIC translation now count down the remaining
element count before forming each string descriptor address. The former loop
formed `elements - 1` even for an empty array, and decremented the descriptor
pointer after element zero. That final pointer lies before the allocation even
though the loop does not dereference it again.

The correction keeps deletion in reverse order and preserves every element below
`keep_idx`. A null physical pointer returns before reading the element size.
An empty array or a retained prefix at least as large as the array forms no
element address. Array layout, allocation ownership and exported signatures are
unchanged. This change does not install a runtime or compiler.

Changed implementation files:

- `src/rtlib/arrays/array_destructstr.c`
- `C:/fb_corpus/fbrtLib/array_destructstr.bas`

## Qualification

`tests/compiler-support/runtime-array-strings.py` builds the real native SDK
implementation and both BASIC C emissions. The C observer calls the actual
runtime string-deletion routine. It checks strictly descending deletion order,
exactly the eligible nonempty descriptors, retained string bytes, unchanged
array metadata and storage sentinels. Cases include sparse arrays, physical
storage with zero virtual-origin bits, empty storage, oversized retained
prefixes and `SIZE_MAX`.

The completed matrix contains:

- 30 native runs across GCC/Clang, GNU11/GNU23 and O0/O2, including six runs with
  pointer-overflow, object-size and alignment instrumentation;
- two historical behavior controls, bringing the total to 14,272 API cases;
- 18 Win32 object checks, without Win32 execution;
- identical native/BASIC exported symbol sets under both C tools and widths;
- 89 SDK headers bound through actual dependency output;
- strict source-only Windows lint of the BASIC translation with zero findings.

The historical loops pass the observable behavior checks. Their defect is an
unused invalid pointer formation, so successful executions alone would not
prove its absence. The saved source edits show that both offending formations
are removed. The implementation still relies on a valid array descriptor.
Allocation-size and unrelated descriptor contracts remain separate work.

The native SDK checks retain the unused generic destructor-parameter diagnostic
where that ABI slot needs a name. String arrays use `fb_StrDelete` rather than
that callback. No diagnostic suppression or dummy read was introduced.

## Current selected queues

All 2,283 selected non-PNG profiles were re-emitted with the accepted compiler.
Its 224 compiler outputs and 2,057 other project outputs retain their exact C
hashes. The two ST/MT string-destructor outputs were recompiled under the
original unsuppressed GCC and Clang profiles. Each loses one Clang buffer-use
observation; GCC remains unchanged.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 4,755 |
| Unit tests / fbcunit | 703 | 16 | 14,048 |
| Production corpus/projects | 1,117 | 443 | 10,545 |
| Additional projects | 239 | 1,683 | 38,181 |
| Total | 2,283 | 2,142 | 67,529 |

Twelve PNG profiles remain excluded. Warning occurrences are counted across
build profiles, not distinct source files. Compiler-backed semantic lint and
the broader remaining source/target and ERROR-owner work are not completed by
this repair.

`build/c-emitter-string-retirement-20261102/` contains the source preimages,
ABI/native receipts, complete selected audit and final summary. One C file and
one object are reused by the queue audit. Native tests reuse their small
module/core/observer object set; their individual work directory has an 8 MiB
limit, and the overall stage has a 64 MiB limit. Installed compiler/runtime
hashes are preserved.

<!-- end of c-emitter-array-strings.md -->
