<!--
Project: FreeBASIC C backend warning qualification
File: c-emitter-pointer-field-words.md
Purpose: Describe pointer-field byte-copy recovery and its verification.
Responsibilities: Record storage rules, original test results and census limits.
This file does not qualify foreign runtimes or unrelated library changes.
-->

# Pointer fields carried as integer words

Optimized IR can carry a pointer field's bytes in a native integer word. The
C emitter already copies those bytes with `__builtin_memcpy`, preserving the
access width and permitting unaligned storage. Previously its member matcher
rejected that copy because the declared field was a pointer while the temporary
was an integer. It then formed the address through integer arithmetic, losing
the storage relationship that GCC needed when optimizing array operations.

`backend/c/c-member-access.bi` now accepts that representation for address-only
matching when the requested integer has exactly the target pointer size. The
copy names the declared member's address through the existing alignment-one
record view. It still copies into the original integer temporary. It neither
loads pointer storage through an integer lvalue nor converts the stored pointer
value. Record layouts, copied widths and source expression evaluation are
unchanged. Other widths, arbitrary integer origins and ordinary address-value
formation retain their existing paths. No diagnostic option is removed.

The change avoids a new bounded-array cast or a generated-C postprocessor.
The existing member matcher proves the field path, and the existing view emitter
supplies a complete declaration with the original record layout.

The selected development compiler is
`d37a9052009511c8132faeafcf471847c909b0925745228c98dfe46160d00f28`.
Its 224-unit self-host reaches an exact emitted-C fixed point with zero GCC
warnings. The first round rebuilds the C emitter; the second changes 32 C units;
the third reuses all 224 qualified objects. Observed build checkpoints reach
10,664,526 bytes. Including the largest live C slot, successor image and receipt
temporary gives a conservative owned-storage bound below 32 MiB.
The existing unread-local compiler, hash
`fc092d79d744a7491bf980fade21705900fbd39dd430dcde0898327f089f85f9`,
is the immediate rollback. Ten selected-path GCC/Clang smoke runs pass under
the original aggressive native diagnostics. Installed release executables and
runtime archives remain unchanged; no additional backup tree is created.

| Complete recovered fb-ext-lib queue | GCC | Clang |
| --- | ---: | ---: |
| Previous compiler, 194 profiles | 15 | 165 |
| Selected compiler, same 194 profiles | 7 | 165 |

All 186 accepted profiles compile under the original aggressive unsuppressed
native flags. The eight refused contexts retain their exact frontend diagnostics.
Eight GCC array observations disappear across `array-ctor-rep.bas`,
`array-ctor.bas`, `array-sort.bas`, `array-swap.bas` and `array.bas`. Every source
warning line and every other native warning class remains unchanged. The
legitimate uninitialized-field warning remains visible, alongside six qualifier
cast observations. Thirty C profiles change; the queue's scratch peak is
3,048,977 bytes under its 32 MiB cap.

Those five original test files also run unchanged with both C backends. Their
six-module test framework is rebuilt in temporary storage for each backend.
Each backend runs ten tests and 48 assertions with zero failures or errors.
The qualification checks actual report totals because the old framework main's
exit status alone does not distinguish a failed Boolean test result. No project
or installed archive is replaced; completed test artifacts occupy at most
228,733 bytes at build checkpoints.
Temporary compiler files and framework archives are removed after verification.

The [new regression](../tests/compiler-support/c-pointer-field-words.bas) checks
native-word copies, const views, packed fields and sentinels, a one-slot prefix
view, callable storage and a side-effecting address expression. Its 59
checks include sixteen byte origins for an ordinary record view. They pass
through GCC, Clang and the independent GAS64 control. The C runs also pass
alignment, object-size and pointer-overflow checks under the original aggressive
diagnostics, with no native warnings. The complete GCC/Clang backend gate passes
against the strengthened fixture and the latest CI portability controls,
including source-warning and raw-address controls. Semantic checks validate
eight models with eight exact emitted-C identities and four Win32 objects.
Those objects use the normal FBC aliasing and signed-overflow compilation
contract, retaining every diagnostic flag. An earlier standalone check omitted
that contract and reported a GCC strict-aliasing observation; its receipt is
retained separately. Twelve compiler objects also compile for Win32, Linux x86_64,
ARMv7 and AArch64 under unsuppressed Clang diagnostics. Their 302 warning
observations are retained; these objects are not foreign runtime execution.

Compiler structure checks cover 410 documented files and 21 host graphs.
Strict lint of the changed compiler include and new BASIC fixture reports no
errors or warnings.

The complete retained census stays at 6,419 profiles, 6,313 accepted C units,
GCC 14 / Clang 15,823. All 224 compiler C identities match the self-host fixed
point. The 126-context Tiko cut stays at GCC 1 / Clang 4,628; its headless widget
regression also passes twelve assertions per backend. Fourteen recovered
fbrtLib contexts retain byte-identical C and GCC 1 / Clang 6. The fb-ext-lib and
Tiko expansions overlap other coverage and are not added to the retained total.

Independent closure compares every source warning code and line, not just
parameter and variable warnings: 3,345 retained, 3,063 Tiko, one fbrtLib and
910 fb-ext-lib diagnostic lines remain unchanged. The seven inserted compiler
source lines have an explicit old/new location map. The six intentional
null-input observations retain FBC warning 57; separate `-w all` controls retain
warning 55 for both unread static locals. Every per-profile native warning
class is unchanged except the eight GCC observations removed from the five
array tests. Unknown contexts and frontend refusals retain their dispositions.

The retained and Tiko queue scratch peaks are 32,108,408 and 68,115,731 bytes
under their 64/96 MiB caps. Reused native work requires exact C identity and
matching pinned tools and original flags. The selected-path array smoke uses
the fixture's existing checked branch; its intentionally unchecked high-bit
index branch correctly fails when bounds checks are enabled. That first pilot
and its verified rollback remain recorded. No example source was changed.

Receipts reuse `build/c-emitter-forward-views-20261105/`:
`pointer-word-selfhost-v1-20261113.json.zlib`,
`pointer-word-gates-v3-20261113.json.zlib`,
`pointer-word-semantic-v3-20261113.json.zlib`,
`pointer-word-alignment-matrix-v2-20261113.json.zlib`,
`pointer-word-array-runtime-v1-20261113.json.zlib`,
`pointer-word-cross-keys-v1-20261113.json.zlib`, and
`current-fbext-pointer-word-v1-20261113.sqlite3`,
`pointer-word-retained-v1-20261113.sqlite3`,
`current-tiko-pointer-word-v1-20261113.sqlite3`,
`pointer-word-fbrt-v1-20261113.sqlite3`,
`pointer-word-tiko-values-v1-20261113.json.zlib`,
`pointer-word-cohort-close-v1-20261113.json.zlib`, and
`pointer-word-selected-publish-v1-20261113.json.zlib`.

This closes the pointer-field repair, not the overall warning audit. The six
remaining qualifier observations, the legitimate shared-pointer initialization
fault, remaining retained GCC classes, JRPG refresh, foreign providers and
Clang cleanup remain open. PNG and unrelated native-library changes remain
outside this qualification.

<!-- end of c-emitter-pointer-field-words.md -->
