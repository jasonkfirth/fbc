<!--
Project: FreeBASIC C-backend verification
File: c-emitter-pointer-qualifiers.md
Purpose: Record the C2FB pointer-qualifier repair and native production replay.
Responsibilities: Bind corrected declarations, counts, object identities and limits.
-->

# Coreutils pointer qualifiers, November 14

This qualifier cut uses `cd32db5b`. Its complete 498-unit Linux coreutils
graph has **208 GCC warnings and 1,852 Clang warnings**. Eight GCC builtin
declaration mismatches are removed. All seven affected GCC objects and all
seven affected Clang objects match their original bytes. The other 491 emitted
C files match their preceding qualified files. Original FBC warning lines are
preserved throughout the complete replay.

The subsequent [literal-offset and ABS repairs](c-emitter-literal-offsets.md)
bring coreutils totals to GCC 201 / Clang 1,845 on `cd0cc1b6`. The subsequent
[plural-format repair](c-emitter-plural-formats.md) reaches GCC 197 / Clang 1,841
on selected compiler `9c1ebb28`. The counts and object comparisons below retain
this qualifier cut.

## Library and generator correction

C2FB retained const on a C base type but discarded qualifiers following `*`.
For `execv` and `execvp`, this changed `char *const *` into `char **`. FreeBASIC
already emits `ZString Const Ptr Ptr` correctly. The repair therefore belongs
to the translator and imported library declarations.

The shared C2FB type model now records pointer-level const. Header and body
resolution share the renderer, including the offset introduced by flattened
pointer aliases. Existing outermost storage-const handling remains a separate
translator responsibility. The focused regression checks five header shapes,
five translated bodies, external-call casts and a conditional function pointer.
It is registered in the translator's normal Make test target.

Eight imported signatures across seven generated library units are corrected
directly against the native argument-vector contract. Runcon's corresponding
callback alias receives the same qualifier. This avoids introducing incompatible
function-pointer casts. The upstream Coreutils C tree was absent from this
sparse checkout, so a full regeneration was outside this qualification.

Two negative controls retain FBC error 119 for assignment through a const
pointer slot. Sixteen actual Linux executions cover both C producers, GCC and
Clang, 32/64-bit targets, and O0/O2. Native type assertions check the imported
pointer signature. The application comparisons use the complete saved
unsuppressed GCC 14.2 and Clang 19.1.7 options at O2.

## Reproduction and receipts

The coreutils checkout is pinned to
`c1efae128e8c20c3b5b85d8faf41fdc42b3e4add`. The portable patch and source
delivery are under `build/c-emitter-forward-views-20261105`.

- `pointer-const-coreutils-v1-20261114.patch` contains the generator,
  regression, Make/README and corrected binding changes.
- `pointer-const-source-delivery-v1-20261114.json.zlib` retains portable
  relative paths, exact source bytes and source hashes.
- `pointer-const-port-replay-v2-20261114.sqlite3` contains all 498 actual
  emissions. Its final callback refresh reuses 497 emissions with verified
  current opened-input pins and produces one fresh emission.
- `pointer-const-port-native-result-v1-20261114.json.zlib` and its v2 callback
  correction retain complete before/after native diagnostics and object hashes.
- `pointer-const-native-result-v3-20261114.json.zlib` retains the translator
  regressions and sixteen current-compiler C value executions.
- `pointer-const-qualified-typedef-open-v1-20261114.json.zlib` retains a
  separate existing qualifier fault described below.

The Linux translator regressions use native GAS64 assembly emitted by the
selected compiler and a pinned read-only runtime archive. Both Windows
translator revisions also build with the selected C backend. Attempting to
link the current Linux C translator against the borrowed older runtime fails
on missing cleanup-frame functions. The older native bootstrap also rejects
current private-declaration syntax. Those two attempts provide no executable
qualification. Their logs and source packets remain recorded. A current Linux
C translator build requires the corresponding current runtime.

## Subsequent formatter ownership qualification

The [formatter correction](c-emitter-coreutils-formatter.md) removes three
duplicate mbszero bodies from linked library callers and records matching
regeneration options. Native symbol comparisons preserve all other exports
and every warning. Sixteen executions compare 22,160 outputs with libc. The
complete coreutils totals remain GCC 208 / Clang 1,852.

## Remaining work

The GCC classes are 153 nonliteral formats, 21 unused functions, fifteen
qualifier casts, eight format-overflow observations, five null dereferences,
four format-truncation observations, one other format observation and one
return-local-address observation. These remain counted under their original
diagnostics.

The subsequent [qualified-typedef repair](c-emitter-qualified-typedefs.md)
closes the separately reproduced flattening fault. `PointerAlias const *`,
with the alias defined as `char *`, now becomes `ZString Const Ptr Ptr`.
It also preserves signed-byte reads through these slots. Thirty-two native
executions and six retained FBC error-119 controls qualify the repair.
Outermost body-storage const and qualified function-pointer alias limits
retain separate open checks.

Strict source-only lint of the ten changed BASIC files retains the same 29
pre-existing readability/documentation findings. Installed compiler/runtime
artifacts pass their seven preservation hashes. The wider corpus coverage and
semantic-linter consumer checks remain open.

<!-- end of c-emitter-pointer-qualifiers.md -->
