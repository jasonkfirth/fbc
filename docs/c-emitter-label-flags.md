<!--
Project: FreeBASIC compiler label storage qualification
File: c-emitter-label-flags.md
Purpose: Record the native compiler label flag and its complete warning census.
Responsibilities: State storage contracts, qualification and remaining limits.
This file intentionally does NOT claim zero warnings or experimental promotion.
-->

# Native compiler label flags

FBS_LABEL.gosub now uses INTEGER, matching the compiler's other native label
state. Its consumers initialize it to FALSE, assign TRUE, test its truth value,
or export a normalized zero/one semantic fact. They do not require one-byte
Boolean storage. The native flag occupies the old tail-padding slot in the
tested Windows layouts. The source keeps the same number of lines.

| Target | Label size before / after | Symbol size before / after |
| --- | ---: | ---: |
| Win32 | 16 / 16 | 216 / 216 |
| Win64 | 32 / 32 | 368 / 368 |

The real compiler headers pass field-offset assertions and GCC/Clang object
checks at both widths. Four native executions compare copied flags, normalized
truth and layout values before and after. No record packing or warning
suppression is added, and no symbol allocation grows in this matrix.

All 224 Make-selected compiler units are emitted from the actual current tree.
The 178 changed C units receive fresh aggressive, unsuppressed GCC and Clang
checks. Each loses exactly one Clang padding observation; no other warning class
changes. GCC remains zero. Clang falls from 1,506 to 1,328: 1,315 padding and 13
startup observations. GCC produces 220 byte-identical objects. The four actual
flag readers/writers receive updated objects; their diagnosed C is retained in
one compact archive. Two preceding current-main objects are reconstructed once
to preserve the concurrent parser/semantic changes outside the frozen package.

Compiler 30cab9d7 regenerates all 224 C inputs byte for byte and passes both
complete backend suites. A direct/computed GOSUB fixture also passes through
the rebuilt GAS64 backend. The new regression fixtures are part of the ordinary
backend runner. Strict lint is clean for all three BASIC/header inputs when
using this exact compiler. That explicit compiler selection resolves missing
metadata from the stale automatically selected compiler; no linter is replaced.
The compiler structure guard passes 399 documented files and 21 host graphs.

| Current main queue | GCC | Clang | C-passed profiles |
| --- | ---: | ---: | ---: |
| Broad corpus/projects/compiler | 2,139 | 6,966 | 2,251 |
| Exampleageddon | 3,495 | 8,916 | 3,488 |

Both full queues are refreshed. The broad reduction is exactly 178; example
totals and all classifications are unchanged. Eligible application C is freshly
emitted. Native checks are reused only for exact C, tools and complete flags
without C preprocessing dependencies. The existing DOS/frontend limits,
included-owner profiles, PNG exclusions and concurrent Optical exclusions stay
classified. No corpus application is executed by this object census. Queues
overlap and their totals count observations, not distinct defects.

The first backend gate correctly ran the layout fixture but rejected its
informative numeric output against a marker-only expectation. The runner now
validates that fixture's marker and four numeric fields; all other fixtures keep
their exact-output checks. The complete native/self-host checkpoint is retained.
Six objects reconstruct the exact same compiler image, so accepted native checks
are preserved while both full runtime suites are completed after the harness fix.

Two superseded test compiler images are preserved in lossless archives, with
original hashes and verified byte restoration. archive-legacy-bootstraps.py
supports --restore canonical or --restore private. The active main bootstrap,
portable package and installed binaries are preserved. All transient C/compiler
slots and temporary header overlays are retired. The same work area is reused,
with a 160 MiB retained ceiling and separate 192 MiB integration ceiling.

This is a main-source compiler repair. Native runtime totals are unchanged.
The portable experimental source package remains its earlier frozen cut;
another-host reproduction and experimental promotion remain unverified. Other
warning families remain open, and the complete campaign is active.

Evidence: label-flag-build.json.zlib, label-flag-screen.json.zlib and
label-flag-queues.json.zlib in the existing E: qualification area, with
build/c-emitter-forward-views-20261105/label-flag-verification.json.

<!-- end of c-emitter-label-flags.md -->
