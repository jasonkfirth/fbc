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
variables do not receive global assembler aliases. Target decoration, including
Win32 fastcall hidden results, remains unchanged. The symbol cache is never
renamed. The existing C type-name encoder is retained.

File-scope variables with leading underscores and record fields beginning with
two underscores now receive encoded C tokens. The four ordinary CRT functions
`_strtoi64`, `_strtoui64`, `_wcstoi64` and `_wcstoui64` are encoded with their
original object names. Other underscore-prefixed functions keep their existing
spelling because a C compiler may recognize them as intrinsics. In particular,
renaming `_mm_pause` made Clang look for a nonexistent external function.

The Windows GCC TLS relocation requires the original C token for applicable
external thread-local variables. Those names retain their original spelling on
the existing TLS compatibility path, so Clang may still report a reserved-name
warning for them. Linux TLS can use the encoded token and assembler alias.

The identifier encoder also recognizes the new C23 keywords. The generated C
token is encoded in both GNU11 and C23 output, while the original linker name
remains intact. On Windows, a thread-local object with an identifier-shaped
linker name that needs encoding uses assembler `.set` directives instead of a C
declaration's asm label. GCC otherwise produces an invalid TLS relocation.
Definitions export the original name at the generated object's address;
external references resolve the generated name to the original object. Win32
object names retain their leading underscore. Other TLS compatibility cases
continue to use the existing path above.

The candidate passed 27 ordinary mixed-language executions and 36 Windows TLS
executions under GNU11 and GNU23, including separate C storage and forward
declarations. Four GCC and four Clang Win32 TLS objects plus one WebAssembly
object were checked without claiming execution on those targets. Linux GCC and
Clang passed 40 ordinary and TLS executions. The selected compiler, unit and
project audits still have warnings; this repair does not resolve the separate
cross-procedure error-handler defect.

The installed FreeBASIC linter reports no style findings for the changed
compiler files, but could not build their standalone semantic models. For the
three keyword-heavy fixtures, its accepted model lacks two
analysis inputs, so those checks finish with operational errors. The fixture
that exports mutable storage also receives the expected shared-storage advice.
The installed compiler cannot provide a semantic model for the forward TLS
fixture. These linter limits are separate from the passing build and runtime
checks and remain open.

MinGW, Windows CE, OpenBSD and RISC OS can use emulated TLS. Their GCC compilers mishandle
explicit asm labels on TLS variables by inserting an internal `*` marker into
the generated descriptor name. For otherwise valid GNU identifiers containing
`$`, the C backend keeps the original TLS name directly. This preserves the
descriptor name used by separately compiled modules without an asm label.

## Qualification

The November 1 source-built compiler was checked against the current compiler
graph with unsuppressed GNU23 GCC and Clang profiles. All 224 compiler
modules, 703 Make-selected unit inputs and 1,129 selected project configurations
emitted and compiled without errors. The project comparison reduced Clang
warnings from 16,090 to 11,924; GCC remained at 616. The unit comparison
reduced Clang warnings from 14,455 to 14,387; GCC remained at 29. These counts
cover the selected inputs, not every source or target in the corpus. The
source-built compiler re-emitted every input. It reused C compiler results for
221 compiler modules, 703 unit inputs and 1,126 project configurations only
after their generated C matched the qualified candidate; the other C files
were compiled again.

The regression fixtures exercise the four CRT functions, user globals and
fields, compiler intrinsics, mixed C/native object links, and Win32 object names.
Windows TLS is checked with real threads and both C producers. Linux GCC and
Clang passed sixteen mixed producer/compiler TLS cases. A separate existing
boundary remains: the Windows native backend uses FreeBASIC-managed TLS
storage, and a C-backend consumer cannot treat that exported storage as C TLS.
This candidate does not change that native-backend contract. Its receipts are
under `build/c-emitter-active-20261101/` in the
`reserved-abi-promoted-compiler-audit`, `reserved-abi-promoted-unit-audit` and
`remaining-production-audit-reserved-abi-promoted` directories.

The earlier v45 receipts are in `build/c-emitter-audit-20261022/c-identifiers-v45/`.
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

## Builtin function addresses and target types

For GCC-generated C, direct libc builtin calls retain their builtin spelling.
When a libc builtin address is taken, the emitter declares a separate C
identifier with the same signature and the corresponding libc object name.
This lets both GCC and Clang compile the address without changing direct calls.
Intrinsic-only builtins are not mapped to libc.

The CRT includes now use the target C long spelling for WebAssembly `size_t`,
`ssize_t`, `ptrdiff_t`, `intptr_t` and `uintptr_t`, and for LP64 pointer-sized
integer types. Their FreeBASIC widths remain unchanged. The legacy bootstrap
compatibility branch still uses its older type spellings because older
bootstrap compilers do not support type aliases.

With the source-built, noninstalled compiler, the builtin fixture passed 16
Win64 and 16 Linux runtime cases plus Win32 and WebAssembly object checks.
The pointer-sized type fixture passed 32 cross-target object checks and 16
native runtime cases on each of Win64 and Linux. GCC and Clang used GNU11 and
GNU23 where supported; the WebAssembly objects used Clang. The checked C
produced no diagnostics. The Linux receipts are in `build/c-emitter-active-20261101/builtin-address-repair/`.

The original 115 source contexts were replayed with both C compilers and both
standards. All four checks passed for 113 contexts. The remaining two saved
Win64 contexts contain 32-bit assembly; separate correctly targeted checks
passed at O0 and O2. The DOS check validates 32-bit assembly in objects, not a
DOS link or execution. Other warnings and the broader corpus remain open.

<!-- end of c-emitter-identifiers.md -->
