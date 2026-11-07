<!--
Project: FreeBASIC runtime qualification
File: c-emitter-utf-words.md
Purpose: Record fixed-width little-endian helper contracts and native checks.
Responsibilities: Bind unsuppressed diagnostics, real file I/O and source delivery.
This file intentionally does NOT claim external-host or whole-campaign completion.
-->

# Fixed-width little-endian word helpers

The four shared UTF-16/UTF-32 word helpers now copy exactly two or four bytes
through local byte arrays. Explicit shifts retain little-endian storage on any
host. Callers must still provide the required accessible bytes. Unaligned access,
signatures and layouts are preserved; there is no allocation or warning suppression.

| Native profile | GCC before / after | Clang before / after |
| --- | ---: | ---: |
| ST, 522 members | 1,009 / 1,009 | 19,830 / 15,686 |
| MT, 522 members | 1,009 / 1,009 | 19,837 / 15,693 |
| Startup | 2 / 2 | 34 / 26 |

All 2,090 native C checks pass. The 8,296 removed Clang observations belong
only to unsafe-buffer-usage; no other C warning class changes. Of 1,045 GCC
objects, 1,039 are byte-identical. The ST/MT encoded narrow/wide writers and
USTRING writer account for the six changed objects and receive real archive
qualification. This is a native runtime reduction, not a new emitted-C census.

In C++, eight byte-index warnings become four unsafe-buffer-usage-in-libc-call
warnings per translation unit. Those four remain visible and counted. The
pointer extent is a caller contract; this repair does not prove arbitrary caller
bounds or make the library warning-free.

Thirty-two before/after C/C++ executions cover all 65,536 16-bit values and 68
32-bit patterns at aligned and unaligned offsets, with independent arithmetic
references and adjacent-byte guards. GNU11/GNU23 and GNU++11/GNU++20 each run at
O0/O2 with GCC/Clang. Eight Win32 objects pass. Eight little-/big-endian cross
constant probes fold correctly; they are compilation checks, not foreign-host
runtime executions.

Both updated normal archives preserve 522-member order and every export. Eight
GCC/Clang C/C++ ST/MT runs perform 48 actual encoded write/read cycles, checking
UTF-16/UTF-32 BOM and payload bytes for narrow, wide and USTRING writers.
The relinked frozen compiler remains 3dd76a4d5fc0 and reproduces all 224
bootstrap C files byte for byte; both complete backend suites pass. Eight
current-checkout provider object pairs and 44 graphics/sound Win32/Win64 object
pairs are exact. All 126 runtime graph cases and three collision guards pass
across 1,252 sources and 21 groups. No PNG source or loader is involved.

For source rebuilding, extract these archives in order into one fresh short
directory before the first build:

1. portable-source.zip
2. string-length-source-update.zip
3. utf-word-source-update.zip (69,013 bytes)

They reside in the existing E:/fbc-c-audit-20261105-forward-views-r2-qualification/
crt-candidate/canonical directory. The last archive is additive to the cumulative
string-length update and contains no binary, native object or cache. All 609
composed runtime inputs match the qualified sources. The unchanged delivered
source verifier passes through a temporary namespace of junctions and hardlinks;
only the cross-volume replacement header is copied. That namespace is retired.
Follow [the portable build instructions](c-emitter-portable-build.md) to run the
verifier, build.py and verify-smoke.py. This does not update the frozen compiler
sources/bootstrap to the later main-checkout compiler cut.

Installed binaries and unrelated dirty work are preserved. Integration allocation
peaked at 170,658,938 bytes under the 192 MiB transient cap. Retained data stays
under 160 MiB. Stronger transparent filesystem compression saves 614,400 bytes
on one immutable diagnostic database, preserving its exact path and SHA-256.
No accepted evidence or old test tree is deleted for this repair.

Another-host rebuilding, full external corpus replay from the source package and
experimental promotion remain unverified. Main emitted-C totals remain broad
2,139 GCC / 6,966 Clang and Exampleageddon 3,495 / 8,916 at their measured cuts.
The overall warning-cleanup campaign remains active.

Evidence: utf-word native, values, archive, consumer, source-update and structure
receipts in the existing qualification directory; the component seal and verifier
are under build/c-emitter-forward-views-20261105.

<!-- end of c-emitter-utf-words.md -->
