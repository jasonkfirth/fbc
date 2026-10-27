<!--
Project: FreeBASIC compiler verification
File: c-emitter-identifiers.md
Purpose: Document the qualified C identifier repair and its remaining limits.
Responsibilities: Explain token/object identity, source adoption and exact checks.
This file intentionally does NOT claim warning-free output or goal completion.
-->

# C identifier spelling repair

The canonical C backend now emits portable identifier tokens for BASIC names
containing dollar signs and for selected C keyword, scalar typedef and generated
namespace collisions. It retains the original symbol cache, object names, RTTI
and source-model identities. The overall warning-cleanup goal remains active.

## Naming and ABI contract

`backend/c/c-identifiers.bi` owns the pure C spelling helpers. Invalid bytes and
underscores become `_HH` byte escapes under `fb_c_sym_`. Escaping underscores
makes aliases containing literal `_24` distinct from aliases containing `$`.
User symbols beginning with `fb_c_`, `fb_type_` or `__FB_` are also encoded, so
they cannot capture backend temporaries, C type names or prolog macros.
Virtual registers, SELECT CASE jump-table storage and statement-expression
scratch variables use separate names under the protected `fb_c_` namespace.
The jump-table path still performs exactly one original unique-ID allocation.

The spelling is used consistently in declarations, expressions, function
pointers, labels, calls and C assembly constraints. Raw object names remain in
assembler strings and explicit `__asm__` declaration aliases. Local/static-local
variables do not receive global assembler aliases. Functions retain the previous
alias policy and target decoration, including Win32 fastcall hidden results.
The symbol cache is never renamed. The existing C type-name encoder is retained.

Windows CE uses CeGCC's emulated TLS. Its compiler mishandles explicit asm
labels on TLS variables by inserting an internal `*` marker into the generated
descriptor name. For otherwise valid GNU identifiers containing `$`, the C
backend keeps the original TLS name directly. This preserves the descriptor
name used by separately compiled modules without relying on an asm label.

## Qualification

The receipts are in `build/c-emitter-audit-20261022/c-identifiers-v45/`.
The same pinned Make-selected 189-module compiler graph was checked with GCC
16.2 and Clang 22.1.7 at O2, using the full audit warning lists without warning
suppression. The final comparison contains 756 accepted object stages, reusing
376 unchanged baseline controls, and 378 exact public-symbol comparisons.
The final helper-header/include corrections required two additional actual HLC
object checks; their C differs only in two independently decoded source-path
literals, and public symbols match. Other compiler objects retain their pins.

| Current-source warnings | Before | After |
| --- | ---: | ---: |
| GCC, all warning classes | 43,172 | 43,172 |
| Clang, all warning classes | 499,288 | 76,817 |
| Clang dollar-sign identifier warnings | 421,992 | 0 |

These are compiler-source counts, not a new corpus sweep or a defect count.
The later [v46 dispatch repair](c-emitter-dispatch.md) replaces these temporary
jump tables with C switches. The v45 results below remain a separate checkpoint.
The earlier initial experiment retained 816 jump-table dollar warnings and
introduced extra `asm` token warnings. Both causes were repaired; their initial
receipts remain separate from the final results.

Additional direct checks:

- Sixteen old/new main/provider combinations executed under matching GCC/Clang
  O0/O2 builds, with four exact public-symbol pairs. They cover a dollar alias
  on an exported global and a procedure, callbacks, static locals, arrays,
  fields, GOTO and DATA/RESTORE.
- Eight Win32 object-only builds preserve four symbol pairs, including
  `@fb_c_fast@8`, `_fb_c_std@8` and `@fb_c_record@16`. No Win32 runtime execution
  is claimed. A fixture's mistaken C spelling `__stdcall` was corrected to the
  BASIC keyword `stdcall`; the failed fixture receipts are retained.
- Four collision-fixture executions and four adopted regression executions pass
  at O0/O2 under both C compilers. The final compiler repeats all four adopted
  regression executions after the source-contract corrections.
- Eight source-model packets validate, and all four old/new pairs are byte
  identical.
- The prior assembly repair retains all eight real fbsound CPU/DSP object
  checks and sixteen native assembly-contract executions.
- Strict nonsemantic Windows lint reports eight files, zero errors, zero
  warnings and zero informational diagnostics.

The full private compiler contains all 189 freshly C-compiled modules and the
pinned complete v43 standard Win64 runtime. Its final source-format revision
reuses 188 of those compiler objects and recompiles HLC. It contains no private
error-handler outliner. Its SHA-256 is `e69e96c46cbf8c8cdaf8141bb8b2acb2bdc066db8b8be91aa3ed0a6e388bc60b`.
Nothing was installed, committed or pushed.

## Source ownership and limits

`source-adoption.json` and `source-format/results.json` record exact preimages
and canonical hashes for the emitter, two focused C-backend helper headers and
`tests/quirk/c-identifiers.bas`. The structure guard has no remaining C-backend
failure. Its eleven existing header/footer failures in seven semantic-tooling
files remain recorded separately; the repository-wide guard is not passing.
Installed runtime artifacts and the canonical AST procedure/error ABI sources
retain their protected hashes. The owned work directory remains below 256 MiB.
The archived corpus and unrelated dirty source trees were not copied again.

Alignment, qualifiers, initialization, array-origin contracts, general error
ownership/lifetime and broader platform/project qualification remain required.
The private error outliner must be rebased onto this canonical emitter rather
than copying an older whole file over the assembly and naming repairs.

<!-- end of c-emitter-identifiers.md -->
