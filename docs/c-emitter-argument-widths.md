<!--
Project: FreeBASIC C argument-width contracts
File: c-emitter-argument-widths.md
Purpose: Record real C API and storage-width defects repaired in example sources.
Responsibilities: Separate native value tests, affected profiles and corpus totals.
This file intentionally does NOT claim experimental compiler promotion.
-->

# C argument and storage widths, November 5, 2026

Seven source files had real contracts to repair. FreeBASIC INTEGER is native
width; the C int accepted by %d and %i, and unsigned int accepted by %u,
remain 32 bits on Win64. Compiler or library changes cannot make a mismatched
variadic argument or scan destination correct without changing those APIs.

- NeHe lesson10 uses LONG for its scanned triangle count.
- The shared 3Dobject.bi loader scans counts and indices into ULONG values,
  checks conversion results and array limits, then assigns the existing native
  record fields. All successful OPEN paths reach one CLOSE. A one-face model
  returns from connectivity setup before unsigned subtraction can wrap.
- The variadic manual example supplies LONG literals to its %i fields.
- The ANY example reads the four-byte SINGLE representation through LONG,
  preserving its purpose on both native widths.
- FILEATTR uses LONG for fprintf and fgetc, checking EOF before Chr.
- Both GTK list examples use LONG for their bounded sprintf loop values.

The two compiler cuts freshly accept all 28 affected original Win64 source
profiles under the unchanged aggressive GCC and Clang flags. All 56 Win32
object checks pass. Exact C identities bind resumed native checks to the final
source files. Across this cohort, each cut removes 26 format observations from
each C tool and one GCC array-bounds observation. Main counts go from 55 to 28
GCC and 84 to 58 Clang; experimental counts go from 27 to zero GCC and 58 to 32
Clang. These are cohort totals, not a new whole-body census.

One experimental checked geometry profile gains a natural padding observation:
five 32-bit scan destinations occupy 20 bytes before a native-sized result slot
in its internal capture record. Another profile loses one padding observation,
so total cohort padding stays unchanged. Neither diagnostic is suppressed and
no record is packed or given dummy fields. Public mesh record layouts remain
unchanged. No other warning class increases in the affected profiles.

Native qualification passes 192 C executions and twelve GAS64 controls. It
covers both C producers, both C tools, GNU11/GNU23, O0/O2 and ordinary/checked
modes. It verifies ANY bits, formatted text, the FILEATTR round trip, both GTK
label patterns, every supplied mesh/world value by independent checksums, and
the valid single-face connectivity case. Geometry checks do not open graphics
windows or load images. GTK label expressions run in a console harness; the
GTK GUI itself was not executed. The ANY harness removes only its interactive
pause, and the world harness redirects only the fixture path.

Strict source-only fblint reports seven files, zero errors and zero warnings.
All 10,899 frozen corpus source pins were checked; only these seven intended
files changed. Installed compiler/runtime files remain unchanged. Compiler and
SDK sources are unchanged by this batch. PNG and user-closed investigations
remain excluded. The broader compiler cleanup and experimental portable build
remain incomplete.

Evidence is retained in build/c-emitter-forward-views-20261105:
width-census.json, width-native.json, width-source-changes.json and
width-verification.json. One existing scratch area holds the compressed native
logs and C outputs. No new directory ladder or full source-tree copy was made.

<!-- end of c-emitter-argument-widths.md -->
