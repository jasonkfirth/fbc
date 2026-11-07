<!--
Project: FreeBASIC USTRING runtime qualification
File: c-emitter-ustring-conversions.md
Purpose: Record checked byte counts and scalar narrowing across the USTRING family.
Responsibilities: Bind diagnostics, values, ABI and reproducible source inputs.
This file intentionally does NOT claim experimental promotion or another-host replay.
-->

# USTRING conversion qualification

Seven shared USTRING units keep byte arithmetic signed until checked libc
boundaries, preserve UTF-8 byte representations independently of plain char
signedness, and narrow scalar/UTF-16 values after their existing range checks.
The I/O unit keeps EOF separate from byte/scalar values and passes native byte
counts to fread and putback. Public declarations and descriptor layouts stay exact.
Existing examples and the existing Linux sanitizer fixture are unchanged.

| Native runtime profile | GCC before / after | Clang before / after |
| --- | ---: | ---: |
| ST | 820 / 754 | 14,470 / 14,404 |
| MT | 820 / 754 | 14,477 / 14,411 |
| Startup | 2 / 2 | 24 / 24 |

The repair removes 132 observations from each C tool across ST/MT, with no
increase in any profile or class. Twenty-eight accepted actual-checkout native
compiles replace the affected checks; 2,062 unrelated checks inherit exact prior
inputs/tools/flags. Four superseded I/O compiles remain in the preflight record.
Twelve of the fourteen target GCC objects stay byte-identical. The seven units
retain thirteen intentional ABI parameters and one existing borrowed-descriptor
const observation per mode; their conversion warnings are gone.

Sixteen before/after GCC/Clang GNU11/GNU23 O0/O2 executions pass every valid
Unicode scalar, case mappings, ownership, profiler names, bounded allocation
limits and actual native UTF-8/16LE/32LE disk I/O. The separate Win64 test view
admits native 16-bit wchar_t and removes its Unicode profiler filename through
_wremove. The original fixture remains unchanged. Its earlier Windows filename
cleanup failure is retained as a harness preflight.

Both normal 522-member archives preserve order and the full export multiset.
Fourteen source-provider GCC objects match actual-checkout objects. Eight fresh
C/C++ executions pass through those normal ST/MT archives, including the same
complete scalar and native file-I/O controls. C++ literal inputs use bounded
mutable copies for the generic C void-pointer source API. Initial C-only fixture
and duplicate-header wrapper failures retain their diagnostics; no permissive
flag or warning suppression is added. MT links are single-thread controls.

The linked compiler image remains byte-identical to the prior string-capacity
checkpoint. Its 224 self-host C identities and both full backend suites are
therefore inherited. Twenty-eight Win32 definition compiles preserve all 100 C
definition names across both source versions and tools. Win32 execution is not
claimed. Runtime structure passes 1,256 files, 21 groups, 126 graph cases and
three guards.

The 174,836-byte ustring-conversion-source-update.zip combines all preceding
runtime repairs, the exact executed checker and its source fixtures. Extract
portable-source.zip, string-length-source-update.zip, then this update. All 613
runtime inputs pass the unchanged delivered verifier; no native binaries or
old object cache are shipped. The separate cold replay already in flight uses
the preceding string-capacity source cut, not this later USTRING update.

One immutable owned census database uses lossless LZX compression to recover
3,813,376 bytes, with its content hash unchanged. Component scratch is removed;
the separate live cold replay may still occupy the authorized 192 MiB integration
allowance. Its retirement/final retained-budget check remains separately tracked.
Installed compiler/archive pins and unrelated dirty sources stay unchanged.

Main emitted-C queue counts stay broad 2,139 GCC and Exampleageddon 3,495 GCC.
The last experimental comparison remains broad 247 / Exampleageddon 83 GCC.
Next work targets those experimental GCC classes, then promotion into current
main while preserving concurrent work. Clang-only cleanup is deferred. Another
host, foreign SDK execution, full delivered-package external corpus replay and
experimental promotion remain unverified; the full campaign stays active.

Evidence: ustring-conversion-*.json/.json.zlib and source-update.zip in the
existing qualification area, with controllers under
build/c-emitter-forward-views-20261105.

<!-- end of c-emitter-ustring-conversions.md -->
