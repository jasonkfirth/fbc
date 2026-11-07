<!--
Project: FreeBASIC compiler verification
File: c-emitter-readonly-entries.md
Purpose: Explain separate reachability for private implementations and ABI entries.
Responsibilities: Record graph ownership, preserved calls and diagnostic evidence.
This file intentionally does NOT claim full warning or runtime qualification.
-->

# Private read-only entries, November 2, 2026

The C backend can place a proven read-only body behind a const-qualified internal
entry and forward the original mutable-pointer entry to that body. Direct calls
use the internal entry; callbacks and external callers retain the original ABI.
The old private reachability graph treated those two requirements as one. It
therefore emitted private forwarding wrappers after all surviving direct calls
had been redirected, producing 30 Clang unused-function warnings in the compiler.

Each private graph edge now records whether it needs the original ABI entry.
A rewritten direct call retains the implementation only. Original calls,
procedure addresses and typed callback adapters retain both. Reachability
propagates the ABI requirement even when another edge has already reached the
same implementation. A self-address edge is retained; a redirected recursive
implementation call does not require another worklist visit.

Original ABI prototype spans are distinguished from internal implementation
prototype spans. Both the unused private wrapper and its original prototype
are omitted. Public, exported, startup, virtual and debug entries retain their
existing ownership. Private CDECL carriers continue to bypass the read-only
transformation. The source procedure's BASIC types and callbacks are unchanged.

Opaque assembly retains the conservative graph fallback. Private forwarding
entries in that fallback carry `used` so their out-of-line assembler symbols
survive optimization. This expresses a required object definition for assembly
references that the C compiler cannot see.

## Qualification

`build/c-emitter-readonly-entries-20261102/current/receipt.json` records the
complete 224-module compiler build, with unsuppressed GCC and Clang checks for
changed C outputs and hash reuse for unchanged C. Clang's compiler warning count
drops from 4,830 to 4,800: exactly 30 unused-function warnings disappear. Other
compiler warning groups are unchanged; GCC remains at zero.

The final compiler SHA-256 is
`162ca7ee28f77926de3acdf68785f54d857f8a6c6ca4861bc6262c5671be4903`.
The native matrix uses that final compiler, rather than the initial one-object
bootstrap candidate.

`checks/receipt.json` records 144 old/new native runs and 176 object checks across
GCC/Clang, GNU11/GNU23 and O0/O2. Controls cover direct calls, dead address edges,
live callbacks, a procedure's own address, shared statics, mutable/escaping
arguments, public methods called from native C, long-double guards and opaque
assembly. Win32 objects are checked without claiming foreign execution.

One existing opaque fixture contains a hard-coded `lea rax` instruction and
cannot assemble as Win32. Its 64-bit cases pass. The new fixture's portable
opaque variant supplies the corresponding Win32 object coverage. The initial
wrong-context failure and a fixture corrected to use the transformation's BASIC
calling convention are retained separately from the completed qualification.

The backend runner includes the new fixture and checks the generated C on
Win64, where the normal BASIC convention permits private read-only inference.
Runtime checks remain available on the other supported hosts.

The new generated-C guard passes with both C producers and rejects the previous
compiler at the expected private-entry check. The existing private-procedure
reachability suite also passes, including opaque assembly, deep/reverse call
chains, destructor ownership, copyback and all cross-target object checks.

The later [native wide-call repair](c-emitter-wide-libc.md) supersedes this
checkpoint table with 104 fewer Clang warnings.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 4,800 |
| Unit tests / fbcunit | 703 | 16 | 14,347 |
| Production corpus/projects | 1,117 | 490 | 11,222 |
| Additional projects | 239 | 1,810 | 41,138 |
| Total | 2,283 | 2,316 | 71,507 |

Exactly 39 Clang unused-function warnings are removed: 30 compiler, seven unit
and two additional-project occurrences. GCC counts and other Clang groups are
unchanged. Counts include repeated source contexts and do not represent unique
source defects. Twelve PNG profiles remain excluded.

The full selected queue refresh and compiler C fixed-point witness are recorded
in `audit/receipt.json`. Its diagnostics retain their original unsuppressed flags.
Installed compiler/runtime binaries and unrelated dirty source work are preserved.
The repair uses reused scratch outputs and compressed logs with a 64 MiB stage cap.

## Remaining work

The separate checked-I/O experiment in
`build/c-emitter-error-dispatch-20261102/STATUS.md` rejects replacing the shared
error dispatcher with inline indirect jumps. That change leaves the terrain
writer's possible-uninitialized warnings unchanged and increases extension
warnings. No compiler or game source was changed for that experiment.

Nonlocal ERROR ownership, descriptor/pointer contracts, other warnings,
unselected source contexts and compiler-backed semantic lint remain open.
PNG work remains excluded.

<!-- end of c-emitter-readonly-entries.md -->
