<!--
Project: FreeBASIC alternate C entry arguments
File: c-emitter-entry-argv.md
Purpose: Explain the entry declaration fix and its complete qualification.
Responsibilities: Bind the source repair, native ABI evidence and retained limits.
This file intentionally does NOT claim the remaining warning campaign complete.
-->

# Alternate entry argument types

The selected compiler also contains [source diagnostics](compiler-source-warnings.md)
and [debug scratch cleanup](c-emitter-debug-scratch.md). The entry qualification
below records the historical BF344DD7 compiler; 5C78E4AD is the current rollback.

The implicit entry producer marked every public entry's second parameter as a
native C `main` argument. The C backend therefore emitted `char**` even for an
alternate name such as `custom_main`. Explicit BASIC declarations use the normal
pointer view, which makes no natural alignment promise. Clang diagnosed their
call boundary as passing a one-byte-aligned pointer to an eight-byte-aligned
parameter. The complete corpus had two such observations, in the checked/default
profiles of `examples/manual/module/option_entry.bas`.

`hMainBegin` in `src/compiler/core/fb-main.bas` now sets the special flag only
when the entry's actual name is `main`. Custom entries retain their ordinary
BASIC type in prototypes, definitions and expression metadata. DLL behavior and
the real C `main(int, char**)` form are preserved. There is no alignment cast or
new assumption. Both affected profiles retain byte-identical GCC/Clang objects
through both C generators. The permanent custom-entry control passes 64 native
value runs and 32 further object identities across the stated matrix. Win32
checks qualify definitions and objects only. The complete GCC backend suite and
the selected public SDK's checked custom-entry/header-using values pass.

All 703 fbctests outputs, all 1,324 production/additional C definitions and 3,488
Exampleageddon C outputs match B2827405 exactly. Only the two alternate-entry
outputs change; one compiler unit changes because it implements the repair.
The compiler's next self-host reaches the 224-unit C fixed point with GCC0.
All 10,923 source pins and protected installed/archive hashes remain verified.

The C23 variadic attempt was rejected before selection. One-argument cursor
initialization produced Clang pre-C23 compatibility observations, and dropping
the final named parameter's reference created GCC unused-parameter observations.
Its alternate header scan also failed the no-introduced-class gate. All owned
production changes from that attempt were reversed using exact byte journals;
the original variadic implementation is retained. No dummy parameter reads,
warning attributes or suppressions replace it. The existing two variadic
observations remain compatibility debt. Failed receipts are evidence of the
rejected approach, not part of accepted current totals.

Authoritative receipts in the existing qualification directory:

- `generated-c-entry-{source,build,fixture,profiles-v2,suite,structure}.json.zlib`
- `generated-c-entry-{fbctests,broad,examples}.json.zlib`
- `generated-c-entry-{compiler-clang,clang-broad,clang-examples,clang-closeout}.json.zlib`
- `generated-c-entry-{rejected-storage,final-storage,publish,documents,closeout}.json.zlib`

The Clang controller uses the complete GCC emission identity sweep and verified
prior C-independence checks to avoid emitting identical programs twice. Source,
native tools and original flags stay pinned. Diagnostic line pools reconstruct
every raw diagnostic byte and hash. The two changed Clang checks are fresh.

The historical `generated-c-c23-entry-*` packets retain the rejected recipe and
failed gates. The earlier entry-profile console label was copied from that
experiment; `profiles-v2` records and prints the actual eight identities and only
the two alignment removals. Shared archives preserve original JSON bytes,
compression levels, COFF bytes and compressed hashes. The existing `s.read` and
`journal-storage.original_bytes` readers reconstruct them. Compiler input pin
parents, immutable source deliveries and installed artifacts remain unchanged.

The twenty raw frontend rejections are now explicitly bound to current
diagnostics and source hashes: four intentional negative demonstrations, ten
legacy `VA_FIRST`/`VA_ARG`/`VA_NEXT` profiles unsupported by C backends, four
JavaScript/RISC OS target-specific profiles and two Visual Basic companion
profiles. Raw statuses stay unchanged. These differ from the two retained
`CVA_START` narrow-parameter Clang observations. See
`generated-c-entry-frontends.json.zlib`; valid examples are unchanged.

Current totals and dispositions: [complete census](c-emitter-current-clang-census.md).

<!-- end of c-emitter-entry-argv.md -->
