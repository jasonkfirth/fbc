<!--
Project: FreeBASIC compiler verification
File: c-emitter-const-slots.md
Purpose: Record const-pointer slot repairs in the selected corpus.
Responsibilities: Explain interface ownership, runtime checks and diagnostic deltas.
This file intentionally does NOT claim completion of the warning campaign.
-->

# Corpus const-pointer slots

The current compiler's remaining three selected `-Wcast-qual` occurrences came
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

## Current selected counts

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

## Native and interface checks

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
