<!--
Project: FreeBASIC runtime verification
File: c-emitter-array-messages.md
Purpose: Record bounded append accounting in the BASIC array-error translation.
Responsibilities: Explain the CRT contract, native checks and retained diagnostics.
This file intentionally does NOT claim completion of remaining compiler warnings.
-->

# Array diagnostic append accounting, November 2, 2026

The BASIC translation of array_boundchk.bas added each snprintf return value
directly to its output cursor. snprintf reports the number of characters that
would have been produced, which can exceed the remaining output space. It can
also return a negative value. The native C implementation already handles both
conditions; the BASIC translation now follows the same cursor contract.

A private helper keeps the cursor between zero and FB_ERRMSG_SIZE minus one.
Truncation leaves it at the final terminator, and a negative result terminates
the text at the current cursor. All six append sites use the helper. Formatting
arguments, error numbers, public procedure signatures and dispatch are retained.
The buffer remains local to each call, with no shared formatter state.

The permanent `tests/compiler-support/runtime-array-messages.c` and Python driver
exercise the complete runtime entry points. The matrix covers native C and both
complete BASIC emissions, GCC/Clang C compilation, GNU11/GNU23, O0/O2, and ordinary
and instrumented execution. The native SDK C implementation is an independent
reference for message behavior. Old BASIC long-message paths are not executed.

All 48 native programs pass 1,536 message cases. These cover three error kinds,
null/empty names, short names and lengths around the 1,024-byte output boundary
through 4,096 characters. Full output is compared against correctly truncated
CRT output. The BASIC implementations additionally pass 96 deterministic
formatting-refusal checks, including an incomplete CRT write followed by -1.
The test hook only writes within its supplied output span.

The 108 runtime/fixture object checks include twelve Win32 objects. Public
symbol sets match across native C and both BASIC implementations for GCC/Clang
and Win64/Win32, including stdcall decorations. Win32 execution is not claimed.
The bound SDK dependency closure contains 89 headers. Pointer-overflow,
object-size and alignment traps are enabled in the checked native runs.

Strict source-only Windows lint reports zero errors and one existing whitespace
finding. Compiler-backed semantic lint remains open. Installed binaries and
runtime archives are unchanged; private objects precede the existing archive.

The selected queue refresh and exact warning delta are bound in
`build/c-emitter-bound-message-20261102/summary.json`. Two new Clang raw-pointer
observations, one per selected translation profile, describe the helper's
guarded terminator write. They are retained without suppression. This repair
closes the formatting contract previously left open by the
[array query packet](c-emitter-array-queries.md).

The overall warning and coverage campaign remains open. Twelve PNG profiles
remain excluded. The full audit reuses one C file and object at a time, with
bounded retained work, subprocess timeouts and below-normal build priority.
The preceding bulk-copy packet requalifies the GCC false-positive classification
already recorded by the November 1 Stunts reduction; that warning requires no
new emitter repair unless a different copy shape or behavior is demonstrated.

<!-- end of c-emitter-array-messages.md -->
