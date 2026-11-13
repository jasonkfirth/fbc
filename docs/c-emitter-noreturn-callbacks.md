<!--
Project: FreeBASIC C backend verification
File: c-emitter-noreturn-callbacks.md
Purpose: Record matching terminating-callback conversion and its qualification.
Responsibilities: Explain signature identity, native controls and selection limits.
This file intentionally does NOT change callback ABI or source warning policy.
-->

# Matching terminating callbacks

CGUI's unchanged `hello.bas` passes `stop_program` to an ordinary callback
parameter. The compiler knows that the body terminates with `END` and gives
its C declaration `__attribute__((noreturn))`. Casting that address to the
otherwise matching callback typedef produces Clang's
`-Wcast-function-type-strict` warning. Direct initialization or argument
passing accepts the original address without that attribute-only cast.

The C emitter now checks direct procedure addresses against the frontend's
interned procedure-pointer signature. It omits the cast only for a terminating
procedure whose complete signature is exactly the requested callback type.
Parameter types, result mode and calling convention remain part of that
identity. An erased signature, typed variance, different convention or an
effectful pointer expression retains its existing conversion path. Callback
addresses remain unchanged, including static initializers and returned values.

No example needs changing and no new BASIC warning is appropriate for this
well-typed callback. Real signature mismatches retain their native diagnostic.

## Qualification

The development candidate is `9e800d585c799c061f42beafa0fc35f2134762c15c5c15f39cf24cceeda610b7`.
All 224 compiler units reach an exact GCC-clean emitted-C fixed point. Thirty-two
native executions pass: Win32/Win64, both C emitters, GCC/Clang, GNU11/GNU23 and
O0/O2. The C observer checks original callback addresses and forwards the context
to the actual terminating BASIC body. All generated-C and observer diagnostics
are unsuppressed and empty in these cases.

Four original-compiler controls reproduce the strict cast failure. Four candidate
controls still reject a genuinely different terminating callback signature. The
unchanged checked CGUI profile has zero Clang warnings with the candidate.
The regular backend runner includes both the native value case and the required
Clang mismatch control.

Receipts reuse `build/c-emitter-forward-views-20261105`:
`noreturn-callback-selfhost-v5-20261113.json.zlib` and
`noreturn-callback-values-v4-20261113.json.zlib`,
`noreturn-callback-gates-v3-20261113.json.zlib` and
`noreturn-callback-class-v5-20261113.json.zlib`.
The value matrix peaks below 16 MiB and the self-host below 32 MiB. Installed
compilers and runtime archives are preserved.

The complete GCC/Clang backend suite passes in 564.469 seconds without a
whole-suite deadline. Every retained strict-cast profile is checked with both
C emitters and both native tools. All fourteen baseline C identities match the
completed census when the original output basename is preserved. CGUI's
two contexts omit the single matching callback cast in each. The combined
compiler also removes the conflicting C main attribute in six contexts, as
described in [runtime-backed arrays and epilogues](c-emitter-runtime-tls-arrays.md). Their two
Clang observations disappear per emitter; the five other strict-cast
observations and every other native class remain unchanged. BASIC warning
lines are identical. These compile checks do not execute corpus applications.

The first class controller exposed an output-name difference: module startup
identifiers depend on the C output basename. The corrected comparison preserves that
basename for both compilers and supersedes the v2 constructor-name differences.

The development path `build/main-gcc/bin/fbc.exe` now selects this compiler.
`noreturn-selected-publish-v3-20261113.json.zlib` records seven selected-path
native value runs and preserves the preceding compiler as rollback. The 703
fbctests replay retains zero GCC warnings. Installed release compilers and
archives remain unchanged.

Whole-queue counts remain the last completed preceding-compiler census in
[current status](c-emitter-current-status.md); affected-class results are not a
whole-corpus recount. JRPG's independent refresh remains explicitly pinned to
the immutable preceding compiler. Its durable checkpoint is resumed with the
same identity when the original execution handle disappears; completed
profiles are retained.

<!-- end of c-emitter-noreturn-callbacks.md -->
