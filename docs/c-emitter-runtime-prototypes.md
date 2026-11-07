<!--
Project: FreeBASIC runtime qualification
File: c-emitter-runtime-prototypes.md
Purpose: Record owned C declarations and checked runtime calling contracts.
Responsibilities: Bind native object identities, C/C++ clients and source delivery.
This file intentionally does NOT claim whole-campaign or foreign-host completion.
-->

# Runtime C declaration ownership

Ten owning headers now declare 108 existing runtime exports. Definitions retain
their exact argument/result types and calling conventions. Low-level C helpers
are not given FBCALL where the implementation uses plain C. The numeric header
uses native stdint/va_list types and leaves storage layouts private. Three local
UTF conversion declarations now come from the shared file/encoding interface.
No existing example or BASIC compiler source is changed.

Profiler getters return named opaque pointers. Their two private type tags use
the existing non-reserved naming convention, including the corresponding DOS
and WinCE definitions. Member layouts and export names stay unchanged. Collector
handles are borrowed; the declarations do not transfer ownership or expose the
records. Metrics remain the synchronized observation interface.

| Native profile | GCC before / after | Clang before / after |
| --- | ---: | ---: |
| ST, 522 members | 1,009 / 901 | 14,651 / 14,541 |
| MT, 522 members | 1,009 / 901 | 14,658 / 14,548 |
| Startup | 2 / 2 | 24 / 24 |

All 2,090 native checks pass. This removes 216 missing-prototype observations
from each C compiler and four Clang reserved-identifier observations. Every one
of the 1,045 GCC objects stays byte-identical. No other warning class changes
and no diagnostic is suppressed. The untouched ERROR/native interface retains
one missing-prototype observation per ST/MT profile under its existing boundary.

The affected-definition screen checks 26 actual C units and eight C/C++ Win32/64
client objects. Eight current-checkout client runs link all 108 typed references
against the matched normal ST/MT archives and execute selected integer, native
long-double, sign and fraction calls. Both C compilers reject a deliberately
conflicting declaration. These are interface tests, not execution of all APIs
or optional printer/device operations.

Fifty-two further objects compile the actual Win32 definitions with GCC/Clang.
Both C++ clients reference the same 108 decorated names as those definitions;
the two compilers agree. Win32 and foreign SDK execution are not claimed.
Eleven existing graphics/sound consumers retain 44 exact Win32/Win64 objects.
All 126 source-graph cases and three collision guards pass across 1,254 runtime
files and 21 groups. The relinked frozen compiler remains 3dd76a4d5fc0; its
224 self-host C identities and both complete backend suites are inherited from
that exact executable. No unchanged compiler tests are repeated.

The 115,337-byte prototype-source-update.zip combines these declarations
with the previous word and RND repairs. Extract portable-source.zip, then
string-length-source-update.zip, then prototype-source-update.zip into a fresh
short directory before building. The separate word/RND updates are not needed.
The archives remain in the existing qualification directory under
E:/fbc-c-audit-20261105-forward-views-r2-qualification/crt-candidate/canonical.

The unchanged source verifier checks all 611 runtime inputs. The shipped
verify-runtime-prototypes.py also runs unmodified in a separate temporary package
namespace: eight further C/C++ runs pass against the same qualified ST/MT library
bytes. Source junctions and hardlinks avoid another full source/object-tree copy.
Follow [the portable build](c-emitter-portable-build.md), then run the new checker
with --tool-dir. Compiler/BI source hashes and the C bootstrap remain frozen
before unrelated later main-compiler edits.

Two early client checks caught test plumbing: a missing math.h include and the
printer references' GDI32 link dependency. Both were corrected without changing
runtime algorithms; the rejected logs remain. Prototype harvesting excludes C
comments so quoted documentation signatures cannot become declarations.

All integration slots are retired. Retained allocation stays below 160 MiB;
integration uses the bounded 192 MiB allowance. Transparent lossless compression
saves 3,776,512 bytes on one immutable diagnostic database while preserving its
path and SHA-256. Accepted evidence, installed binaries and unrelated dirty work
remain intact. No PNG work is involved.

Another-host rebuilding, full external corpus replay from this source package,
foreign SDK execution and experimental promotion remain unverified. Main emitted-C
totals stay broad 2,139 GCC / 6,966 Clang and Exampleageddon 3,495 / 8,916 at their
measured cuts. The overall warning-cleanup campaign remains active.

Evidence: prototype plan/screen/native/client/Win32/consumer/structure/source-update
and portable-checker receipts in the qualification directory, plus the component
seal and verifier under build/c-emitter-forward-views-20261105.

<!-- end of c-emitter-runtime-prototypes.md -->
