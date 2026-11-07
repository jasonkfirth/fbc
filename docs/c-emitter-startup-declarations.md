<!--
Project: FreeBASIC C startup declaration ownership
File: c-emitter-startup-declarations.md
Purpose: Explain the qualified registration attribute repair and GCC disposition.
Responsibilities: Link source ownership, native evidence and unchanged boundaries.
This file intentionally does NOT claim all warning queues are zero.
-->

# Startup declaration ownership

`src/compiler/backend/c/ir-hlc.bas` already writes a shared-header declaration
for each emitted global body. The declaration's constructor/destructor attribute
registers an ordinary callback. Repeating the same attribute on its definition
produced a second Clang diagnostic without adding a registration. `_emitProcBegin`
now marks all global startup bodies as declared in that header; the existing
aliased path uses the same rule. Declaration order and argument/return ABI stay
unchanged. Combined startup bodies retain their existing `used` declaration, and
NetBSD destructor registration still follows its existing `.fini_array` path.

The permanent `tests/compiler-support/c-startup-declarations.bas` regression
covers priorities, grouped callbacks, public and assembler-labelled callbacks,
default priorities, implicit module startup, a module-to-main CDECL call, and
cleanup values. A terminating callback control is object-only. The backend runner
compiles a second module from the same fixture and validates both output markers.

The complete corpus proof reconstructs all 5,517 non-compiler C outputs exactly
to their predecessor hashes by restoring only redundant definition attributes.
Of those outputs, 414 change and 425 attributes disappear; 38 were retention
attributes and 387 caused duplicate registration observations. Native Clang
independently confirms exactly 387 removals and no introduced class. The compiler
rebuild changes one backend unit and reuses 223 identical C units; its next
self-host reaches the 224-unit emitted-C fixed point with zero GCC warnings.

Native evidence includes 64 value runs, 88 byte-identical objects and six further
real-profile object identities. Win32, Linux and NetBSD checks qualify definitions
and objects only. Source checks, ABI identities and native values are separate
evidence. The original diagnostic flags contain no added warning suppressions.

The complete GCC queues retain compiler 0, fbctests 0 and Exampleageddon 6.
Fresh checks with both generators show that all six are deliberate null writes
in the signal-handling demonstration, three in each mode. The previously repaired
DOS source is GCC-clean through assembly output in both modes and generators.
The native DOS assembler stays closed. No valid example is rewritten to change
these counts. Other GCC observations remain classified and counted separately.

Receipts in the existing qualification directory:

- `generated-c-startup-{source,build-v2,test-amendment,fixture,suite,structure}.json.zlib`
- `generated-c-startup-{fbctests,broad-v2,examples,gcc-remaining}.json.zlib`
- `generated-c-startup-{compiler-clang,clang-broad,clang-examples,clang-closeout}.json.zlib`
- `generated-c-startup-{final-storage,publish,documents}.json.zlib`

The unsuccessful broad completion recorder is retained as
`generated-c-startup-broad.json.zlib`. Its final aggregate lookup shadowed a
per-profile variable after every native check completed. The `broad-v2` amendment
validates the complete predecessor normal binding and unchanged compiled results.
The executed Clang controllers are archived as `generated-c-startup-clang-queues-v1.py`
and `-v2.py`, with exact executed hashes. Shared evidence wrappers preserve the
original JSON bytes, original compression levels and compressed hashes; use the
existing `s.read` / `journal-storage.original_bytes` readers for reconstruction.
Compiler input pin parents and immutable delivery archives are unchanged.

Counts and limits: [current census](c-emitter-current-clang-census.md).

<!-- end of c-emitter-startup-declarations.md -->
