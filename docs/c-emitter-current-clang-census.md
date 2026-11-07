<!--
Project: FreeBASIC current emitted C diagnostic census
File: c-emitter-current-clang-census.md
Purpose: Record complete current counts, exclusions and original native flags.
Responsibilities: Keep measured reductions separate from retained contracts.
This file intentionally does NOT claim the entire campaign is complete.
-->

# Current selected compiler: compiler-owned debug bindings

`build/main-gcc/bin/fbc.exe` is `3b736d245e30fc79e34e386aca28eea7617ecb7cc3d1dc5e92771cb2133cd2b1`.
838B6658 is the immediate rollback. Installed release artifacts and normal ST/MT
archives are unchanged. The complete GCC queues bind 10,932 current source pins.

| Group | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Compiler | 224 | 0 | 1,338 |
| fbctests | 703 | 0 | 777 |
| Broad production | 1,129 | 65 | 945 |
| Broad additional | 239 | 124 | 1,634 |
| Exampleageddon checked | 1,775 | 3 | 2,727 |
| Exampleageddon default | 1,775 | 3 | 2,606 |
| Total | 5,845 | 195 | 10,027 |

All 24 remaining unused generated-variable observations are removed without
warning attributes, suppression or dummy reads. All 5,739 open GCC C definitions
pass. The separate 1,368-profile normal sweep retains three observations and its
exact preceding C/native binding. All fbctests and Exampleageddon C outputs match
the preceding compiler. Named debug storage, array types/bounds, live evaluations
and descriptors actually passed to procedures are preserved.

Qualification passes 288 native checks, 288 value runs, 52 debug binding checks,
eight DWARF type/bounds comparisons and the complete GCC backend suite in 166.272
seconds, including all 420 source-warning controls. Strict lint and fresh compiler
and runtime structural guards pass. Public SDK smoke values and source warnings
pass after selection. Two unrelated source updates were preserved in this build.

All five set-but-not-read observations match FBC warning 55 and refer to named
source values, including a macro-local TMP. Earlier summaries incorrectly called
four compiler temporaries. Remaining source advisories and default correctness
warnings remain available independently of backend cleanup.

The compiler was promoted after the full GCC gate, then both complete Clang
queues were refreshed. All 5,739 open C definitions pass both native compilers.
Clang falls by 28: 24 unused generated variables and four unreachable-code
observations inside intentional navigation feature stubs. No warning class is
added. Both 224-unit compiler producer graphs remain at 1,338 Clang observations.
Eleven broad C outputs change; all fbctests and Exampleageddon C outputs match
their predecessor. PNG, native-runtime and closed assembler boundaries remain
unchanged. The broader campaign stays active.

See [debug-binding qualification](c-emitter-debug-bindings.md).

## Previous complete GCC/Clang census: 838B6658

`build/main-gcc/bin/fbc.exe` is `838b6658a6ae3099839beba3e819b15b27886c316de44141d889d23859ea5a3a`.
5C78E4AD is retained for immediate rollback. Installed release artifacts and normal
ST/MT archives are unchanged. All 10,929 source pins agree across the current queues.

| Group | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Compiler | 224 | 0 | 1,338 |
| fbctests | 703 | 0 | 777 |
| Broad production | 1,129 | 73 | 953 |
| Broad additional | 239 | 140 | 1,654 |
| Exampleageddon checked | 1,775 | 3 | 2,727 |
| Exampleageddon default | 1,775 | 3 | 2,606 |
| Total | 5,845 | 219 | 10,055 |

Debug-only removal of unread compiler scratch stores eliminates ten GCC and ten
Clang observations while retaining named source bindings and opaque assembly.
Five corpus C outputs change. All fbctests and Exampleageddon outputs, and the
complete 1,368-profile normal C/native binding, remain unchanged. Normal GCC stays
at three observations. The compiler self-hosts 224 units at an emitted-C fixed
point with GCC0; 223 units match the immediately preceding compiler.

Qualification passes 192 native checks, 192 native value runs and twenty debug
binding checks, plus the full GCC backend suite in 172.111 seconds with all 420
source-warning controls. Strict lint, compiler structure and automatic SDK use pass.
Both complete diagnostic queues pass all 5,739 open C definitions. The original
closed profiles, exclusions and twenty classified frontend failures are preserved.

The Clang total is four below the preceding complete BF344DD7 census: this repair
removes ten unused-store observations, while the earlier source-warning repair
exposed six existing-layout padding observations. No new warning class is added.
The BASIC source warnings remain enabled under their established levels.

See [the repair and reproduction](c-emitter-debug-scratch.md). PNG, native runtime
and closed DOS assembler work remain excluded. The broader campaign stays active.

## Historical source-diagnostic qualification: 5C78E4AD

`build/main-gcc/bin/fbc.exe` is `5c78e4ade839f6e96c29864b2407ae780e64285b4debcb8b1ba3451930b4ae33`;
BF344DD7 is retained for immediate rollback. Installed release binaries and normal
ST/MT archives are unchanged. See [source warning policy and reproduction](compiler-source-warnings.md).

FBC now reports impossible range comparisons and provable null accesses by default,
with unread named parameters and scalar values under `-w all`. All 61 previously
warning-bearing profiles pass fresh unsuppressed GCC checks, retaining exactly
229 observations and byte-identical C. The compiler self-hosts 224 units with GCC0;
420 focused checks, 408 emitted-output identities, strict lint and the complete
GCC backend suite pass. Both automatic SDK smoke programs run successfully.

The current compiler Clang check passes both 224-unit generator graphs: 1,338
observations each, six more existing-layout padding observations and no new class.
The other Clang queues and the previously clean GCC queues were not fully rerun
for this diagnostic-only change. The last complete corpus census is retained below.
PNG, native runtime and closed assembler boundaries remain unchanged.

## Last complete queue census: BF344DD7

The compiler used for this full census was `build/main-gcc/bin/fbc.exe`, SHA256
`bf344dd71f14b0a99cfa4d72f3d14eb803931445fd6f99c5f2eaeaf48d45c1ab`. Canonical sources contain the qualified entry
repair. Installed release artifacts and normal ST/MT archives are unchanged.
That complete census bound 10,923 source pins.

| Group | Configured profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Compiler | 224 | 0 | 1,332 |
| fbctests | 703 | 0 | 777 |
| Broad production | 1,129 | 77 | 957 |
| Broad additional | 239 | 146 | 1,660 |
| Exampleageddon checked | 1,775 | 3 | 2,727 |
| Exampleageddon default | 1,775 | 3 | 2,606 |
| Total | 5,845 | 229 | 10,059 |

`src/compiler/core/fb-main.bas` now reserves the native C `char**` argument form
for an entry actually named `main`. A function selected with `-entry custom_main`
retains the ordinary BASIC pointer view used by its explicit declaration. This
removes both alternate-entry alignment observations without adding alignment
assumptions, changing the argument ABI or modifying the example.

The compiler self-hosts 224 units at an emitted-C fixed point with zero GCC
warnings; 223 predecessor C units are unchanged. All 703 fbctests C outputs match
the predecessor exactly and remain GCC-clean. The complete GCC backend suite
passes in 141.576 seconds without a whole-suite timeout. Qualification passes
64 native value runs, 32 whole-object identities and eight further real-profile
object identities across both C generators, GCC/Clang, C11/C23, O0/O2, ST/MT and
Win32/Win64 definitions. Win32 objects are not executed. Strict production lint
and compiler/runtime structural checks pass. Both checked SDK smoke fixtures
emit, compile with GCC0, link and run without `-prefix` or `-i`.

The complete Clang census falls from 10,061 to 10,059, with no new warning class.
The GCC sweep proves 5,515 non-compiler C outputs byte-identical to B2827405;
only the two alternate-entry profiles change. The Clang census reuses native
diagnostics only with matching C bytes, flags and tools. Skipping a repeated
emission additionally requires prior proof that those bytes contain no external
C includes or time macros. Changed C is checked freshly. All 5,739 open native
C definitions pass. The original 32 Optical exclusions, 46 PNG exclusions, six
included-by-owner profiles, twenty frontend failures and two closed DOS assembler
cases remain explicit.

GCC's 229 observations remain counted: 220 debug/source contracts, six deliberate
fault-demonstration observations, two checked formatter diagnostics and the
documented Stunts optimizer copy false positive. The separate 1,368-profile normal
production/additional replay retains three observations and exactly matches its
predecessor's complete C/native-diagnostic binding. Native runtime C remains a
separate deferred queue: 660 ST, 660 MT and two startup observations.

The variadic C23 experiment is rejected: it introduced GCC unused-parameter or
Clang compatibility diagnostics under the original flags. Its production edits,
temporary regression source and runner changes are reversed byte-for-byte.
The two existing narrow-parameter variadic diagnostics remain visible and are not
claimed fixed. The rejected compiler images are retired with source/object
recipes and failed-gate evidence retained. B2827405 is the immediate rollback;
D7B30837 is retired. Installed binaries and immutable delivery archives are
unchanged. Shared evidence compression preserves every original byte/hash and
saves 1,516,275 bytes in this batch. Retained/transient limits remain 160/192 MiB.

The twenty raw frontend rejections are now explicitly bound to current
diagnostics and source hashes: four intentional negative demonstrations, ten
legacy `VA_FIRST`/`VA_ARG`/`VA_NEXT` profiles unsupported by C backends, four
JavaScript/RISC OS target-specific profiles and two Visual Basic companion
profiles. Raw statuses stay unchanged. These differ from the two retained
`CVA_START` narrow-parameter Clang observations. See
`generated-c-entry-frontends.json.zlib`; valid examples are unchanged.

The overall campaign remains ACTIVE. Remaining actionable work includes typed
callback conversions; ABI padding and required startup registrations stay counted.
See [entry repair and receipts](c-emitter-entry-argv.md).

<!-- end of c-emitter-current-clang-census.md -->
