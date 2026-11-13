<!--
Project: FreeBASIC C backend warning qualification
File: c-emitter-const-slots.md
Purpose: Explain qualified pointer temporaries, source diagnostics and library fixes.
Responsibilities: Record aliasing rules, exact coverage and unfinished checks.
This file does not qualify image loading, installed library replacement or foreign execution.
-->

# Qualified pointer slots

The selected development compiler is
`1b874138079b497b1cef2ca13aba6dbf07a6ff6b7117d0f42f556004b57850eb`.
It reaches an exact 224-unit emitted-C self-host fixed point with zero GCC
warnings. Fourteen selected-path GCC/Clang smoke runs pass under the original
aggressive native diagnostics. The existing pointer-word compiler,
`d37a9052009511c8132faeafcf471847c909b0925745228c98dfe46160d00f28`,
is its immediate rollback. Installed release executables and runtime archives
remain unchanged. Complete group counts remain in
[current status](c-emitter-current-status.md).

Qualification binds the staged compiler source cut. A concurrent DOS ASIN
precision edit to `ir-hlc.bas` remains in the working tree and outside this
commit. The qualified emitter is the exact original Git blob from `7a6d2b0e6`;
its retained compiler output matches the fixed point. Every other input pin
matches the canonical files. The source-cut reconciliation records both hashes
and preserves the independent DOS work for its own qualification.

## Values and aliases require different treatment

Passing a pointer result to `ByRef value As Const Payload Ptr` materializes
a fresh pointer slot. The emitter previously gave that slot the original
mutable pointer type, then cast its address to the parameter's pointer-to-pointer
type. The value conversion itself is safe. The temporary now has the requested
qualified type when all intervening pointer levels permit that conversion.
Calls, pointer expressions and materialized conditional values keep their
evaluation count and lifetime.

A real variable or a `ByRef` function result aliases caller storage. Its slot
cannot be replaced by a copy merely to avoid a diagnostic. Adding pointee
`Const` through a writable pointer slot can let a callee install a pointer to
read-only data into storage the caller still treats as writable. Warning 58,
"Unsafe nested CONST qualification", now reports this at selected argument,
assignment, initializer and reference-binding boundaries. Array parameters
also include their extra storage indirection. The existing const-removal and
different-pointer checks remain separate.

The pure symbol predicate checks only equal pointer shapes. It adds no
diagnostics during overload ranking. At each added deeper qualifier, every
intervening target pointer must also be const. A true alias includes the slot's
own qualifier in this check; a copied value does not. The warning is level one,
is enabled by default, respects `-w none`, and becomes fatal under `-w error`.
Safer signatures can use a value parameter or a read-only pointer slot, such
as `ByRef value As Const Payload Const Ptr`.

## Library corrections and compatibility

The complete recovered fb-ext-lib queue now has **194 contexts, 186 accepted,
GCC 0 / Clang 159**, compared with 7 / 165 on the preceding compiler. All eight
runner-context refusals and every other native warning class remain unchanged.
The original examples are unchanged.

Both `SharedPtr` pointer constructors left `m_refcount` uninitialized when the
supplied object pointer was null. They now initialize it to null before the
nonempty allocation branch. Allocation policy, deleter choice and the existing
destructor implementation are unchanged. The
[scoped constructor patch](c-emitter-sharedptr-null.patch) preserves unrelated
dirty library work. A scalar-only regression first reproduced both missing
initializations using poisoned placement storage and unsigned-byte inspection,
without following the uninitialized pointer. After repair, 18 lifecycle checks
pass under GCC, Clang and GAS64; four Win32 objects also compile.

The other six warnings came from five library transform APIs accepting writable
references to const-pointee pointers. Merely changing those public signatures
would change their C++ symbols. The
[scoped input-adapter patch](c-emitter-fbext-input-slots.patch) retains the old
declarations and exports, and adds local reference overloads for mutable source
pointers. Source argument passing stays ByRef. Each adapter creates a correctly
qualified source view before calling the legacy entry. Destination and numeric reference parameters retain their
aliases. The four transform source files and all installed/project archives
remain unchanged.

The adapter contract is checked by linking updated-header clients to separately
compiled original-header providers. Six calls per GCC, Clang and GAS64 run check
the private source slot and original destination/numeric aliases, using opaque
null slots only. Eight Win64 native module comparisons retain the original
exported symbols and instruction bytes; disassembler trailing comments with
renumbered local labels are excluded from the instruction comparison. No image
object, loader, display or rendering loop is run.
An inferred procedure-address control also preserves the original callable
signature before and after adding the overloads.

## Qualification and remaining work

The complete GCC/Clang backend gate passes with the new runtime fixtures and
warning driver. The warning driver checks 408 combinations across C and native
assembly producers, 32/64-bit targets and default/all/none/error policies. It
verifies warning locations and 204 emitted-output identities. Its unsafe source
examples are compiled only. Sixteen semantic models validate both transports
and preserve sixteen emitted-C identities; eight Win32 objects retain the
intentional real-alias diagnostic.

The new runtime fixture separately checks 23 value/const-slot conditions and
25 conditions with its real-alias proof enabled. That proof uses two mutable
objects and observes the caller's slot during the call. It keeps both FBC
warning 58 and the corresponding native qualifier diagnostic. GAS64 also
passes both the 23-check value case and 25-check alias case. The ordinary
temporary cases have no native qualifier warnings.

The complete retained census stays at 6,419 profiles, 6,313 accepted C units,
GCC 14 / Clang 15,823. All 224 compiler C units match the GCC-clean fixed point;
fbctests GCC warnings remain zero. Only 19 retained C profiles change. The
126-context Tiko cut retains exact C identities and GCC 1 / Clang 4,628.
Fourteen recovered fbrtLib contexts also retain exact C identities and GCC 1 /
Clang 6. Both headless widget runs pass their twelve assertions.

Independent closure preserves every original source-warning line: 3,345
retained, 3,063 Tiko, one fbrtLib and 910 fb-ext-lib lines. Compiler and library
source edits have explicit old/new location maps. The new warning adds no
observations to these corrected corpus cuts. All twenty retained and eight
fb-ext-lib frontend-refusal signatures retain their dispositions. Historical
per-profile dependency annotations are reconciled against the current identity
pins for the two changed library headers.

The final retained recount stays below its 64 MiB scratch cap; its exact
peak was not persisted after the concurrent-source check stopped finalization.
The initial B8 retained measurement was 29,147,064 bytes. Tiko peaks at
63,839,013 bytes under its 96 MiB cap. Each native tool reuses 6,312 final
retained results and all 126 Tiko results only after exact C identity,
header/macro safety and pinned tool/flag checks.
The headless widget run remains below its 96 MiB cap. Selected-prefix smoke
artifacts peak at 95,687 bytes and are removed after execution. The existing
immutable rollback is reused without creating another backup tree.

A separate Win32 module check found that the existing `RotoZoomASM` C emission
exceeds GCC's 30-operand inline-assembly limit. It uses 24 distinct `+m`
memory operands, counted as 48 read/write operands by GCC. Symbol deduplication
is already active. A structural repair must preserve variable slots, register
state and control flow. Its failed baseline is retained;
the Win64 ABI comparison and opaque-slot provider tests do not qualify that
Win32 implementation. This structural defect remains open. Clang's remaining
layout/startup classes, JRPG and foreign-provider qualification remain separate.
PNG work remains excluded.

Strict lint reports no new findings in the compiler and regression sources.
The adapter header retains an existing include-guard macro-parenthesis advisory;
its original macro text is unchanged.

Receipts reuse `build/c-emitter-forward-views-20261105/`:
`const-slot-selfhost-v2-20261113.json.zlib`,
`const-slot-gates-v1-20261113.json.zlib`,
`const-slot-probes-v3-20261113.json.zlib`,
`const-slot-semantic-v1-20261113.json.zlib`,
`const-slot-gas64-values-20261113.json.zlib`,
`const-slot-alignment-matrix-v1-20261113.json.zlib`,
`const-slot-class-close-v2-20261113.json.zlib`,
`const-slot-cohort-close-v2-20261113.json.zlib`,
`const-slot-source-cut-20261113.json.zlib`,
`const-slot-selected-recheck-v2-20261113.json.zlib`,
`const-slot-retained-v2-20261113.sqlite3`,
`current-tiko-const-slot-v1-20261113.sqlite3`,
`const-slot-fbrt-v1-20261113.sqlite3`,
`current-fbext-const-slot-v1-20261113.sqlite3`,
`current-fbext-const-slot-v3-20261113.sqlite3`,
`sharedptr-null-baseline-v2-20261113.json.zlib`,
`sharedptr-null-values-20261113.json.zlib`,
`fbext-input-slots-values-v3-20261113.json.zlib`,
`fbext-input-slots-procptr-v2-20261113.json.zlib`, and
`fbext-input-slots-abi-after64-v3-20261113.json.zlib`, and
`asm-operand-inspect-20261113.json.zlib`.
The first class cut retains all six native qualifier diagnostics alongside
the new FBC warnings; the final cut qualifies the corrected library header with reference passing retained.

The earlier VRegex and sprite interface repairs below remain part of the audit.
Their counts and receipts describe that historical cut, not the current totals above.

## Earlier corpus interface repairs

At that earlier checkpoint, the compiler's remaining three selected `-Wcast-qual` occurrences came
from pointer slots in fb-ext-lib. A pointer to mutable storage cannot safely be
passed as a pointer to a pointer to const storage. The callee could replace the
slot with the address of a const object, leaving a mutable view in the caller.
These repairs express the intended ownership without changing compiler lowering
or suppressing diagnostics.

VRegex's `error_string` is now a pointer to const ZSTRING. PCRE supplies borrowed
diagnostic text through `const char **` outputs in `pcre_compile` and `pcre_study`.
VRegex does not own or modify those strings. Its reporting function takes a
mutable STRING reference, so the two reporting paths now copy diagnostic text
into owned STRING locals. The execution-error path also checks for a null error
pointer before copying: a failed match need not supply compilation-error text.

Sprite.RotateFromImage now gives Rotate a separate pointer-to-const-image local.
Rotate's reference parameter aliases that local slot rather than the method's
mutable image-pointer parameter. Existing public rotation signatures and frame
ownership remain intact.

### Counts at the earlier interface checkpoint

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 4,802 |
| Unit tests / fbcunit | 703 | 16 | 14,347 |
| Production corpus/projects | 1,117 | 487 | 11,089 |
| Additional projects | 239 | 1,805 | 41,133 |
| Total | 2,283 | 2,308 | 71,371 |

The two affected production C outputs are freshly compiled with the original
aggressive, unsuppressed GCC and Clang profiles. Exactly three `-Wcast-qual`
occurrences disappear from each tool; all other diagnostic groups match.
The other 2,057 project outputs are re-emitted and byte-identical. The compiler
source and executable are unchanged from the preceding 224-unit fixed-point
checkpoint. Twelve PNG profiles remain excluded.

### Earlier native and interface checks

The real production VRegex and error-reporting sources run against the pinned
PCRE static library. A separate C helper includes the actual PCRE SDK header and
writes a borrowed compilation diagnostic through the public field. Nineteen
SDK headers are bound from the GCC and Clang dependency closures for both
widths and language standards. Tests cover
matching, a nonmatch, an invalid expression, diagnostic offsets and destruction
without freeing PCRE-owned text. Both C emitters, both C compilers, GNU11/GNU23
and O0/O2 pass all 16 native cases. Twenty main objects, twenty SDK objects and
eight foreign main objects also pass; four main and four SDK objects are Win32
checks, not Win32 executions.

Both C backends reject an attempted write through the new diagnostic pointer
with error 119, `Cannot modify a constant`. The same source compiles using the
retained old header. Eight native before/after layout checks agree on the Win64
record size and public diagnostic-field offsets: `160,0,8`. Sixteen layout
objects include eight matching Win32 checks.

The sprite test builds the actual sprite, image, rotation, matrix and rectangle
collision modules separately. It initializes a 16 by 16 null-driver graphics
context, allocates offscreen images, rotates three times, checks all destination
pixels and verifies that the original image survives frame destruction. Sixteen
before/after pairs pass, for 32 native runs and 112 main/component objects.
The old sprite object retains one qualifier warning; every repaired sprite
object has none. No window is opened and no image file is loaded.

The initial graphics fixture omitted IMAGECREATE's graphics-context precondition
and failed. The corrected null-driver fixture resolves that harness error.
The earlier single-translation-unit attempt also encountered a duplicate template
macro, and the first separate-module link omitted the rectangle-collision owner.
Those receipts are retained separately from the completed matrix. Production
modules and graphics runtime files were not changed to accommodate the tests.

## Preservation and remaining scope

Only these corpus files change:

- `fb-ext-lib/include/freebasic/ext/vregex.bi`
- `fb-ext-lib/src/modules/misc/libext-misc-vregex.bas`
- `fb-ext-lib/src/modules/graphics/libext-gfx-sprite.bas`

Exact source preimages, inverse edits, input hashes and receipts are under
`build/c-emitter-const-slots-20261102`, with a 32 MiB retained-stage cap. Original
LF bytes and unrelated whitespace remain intact. Source-only lint retains the
two existing header findings; the added footer resolves one old final-blank-line
finding. Compiler source, installed binaries and unrelated dirty work are
preserved.

The [current status](c-emitter-current-status.md) tracks remaining diagnostics,
nonlocal ERROR ownership, descriptor and pointer contracts, public CRT header
compatibility, unselected source contexts and semantic lint qualification.
This packet closes the selected qualifier warnings, not the overall campaign.

<!-- end of c-emitter-const-slots.md -->
