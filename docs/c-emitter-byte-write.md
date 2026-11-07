<!--
Project: FreeBASIC emitted-C verification
File: c-emitter-byte-write.md
Purpose: Record the corpus unsigned-byte WRITE format repair.
Responsibilities: Explain the native type, exhaustive byte checks and queue receipts.
This file intentionally does NOT claim file-device or foreign runtime qualification.
-->

# Unsigned-byte WRITE format, November 2, 2026

`C:/fb_corpus/fbrtLib/io_writebyte.bas` converts the unsigned byte to FreeBASIC
`ULONG` before passing it to the `%u` format. A byte would otherwise undergo
the C default integer promotion to signed `int`. `ULONG` supplies the unsigned
32-bit argument expected by this format on the qualified C targets. The public
WRITE parameter types, signed-byte path, output bytes and mask policy remain
the same.

The corpus file's original LF endings are preserved. Its header and footer now
identify its role. No compiler source, installed runtime or installed compiler
is changed by this repair. The existing compiler remains
`build/c-emitter-long-literals-20261102/current/fbc.exe`, SHA-256
`65b7bb4bc58d9c2256f991a18ed086e131336046643382be77422ede385c13d9`.

## Direct qualification

The new corpus `byte-write-contract.bas` includes the owning production module.
It calls both byte WRITE routines for every representable signed and unsigned
byte and each mask from zero through seven. Each native run performs 4,096
format calls. A bounded capture function replaces only the final file-buffer
handoff. Expected decimal digits are calculated independently, and every byte,
output length and handoff count is checked. Windows text and binary CR/LF and
comma endings are covered. No file device, display or actual concurrent writer
stress test is claimed.

The driver passes 32 native runs and 40 native object checks across both C
backends, GCC and Clang, GNU11/GNU23, O0/O2 and ST/MT builds. Eight of the object
checks target Win32; they are not native execution claims. Every run links the
corresponding pinned `libfb` or `libfbmt` archive. The original module preimage
reproduces its three Clang format-signedness warnings without suppression.

Strict source-only lint is clean for the production module and BASIC fixture.
The compiler source and binary hashes match the prior completed qualification.
Both selected production profiles are freshly emitted and compiled with their
original aggressive, unsuppressed GCC and Clang flags. The other 2,281 selected
profiles retain their prior receipts after rebinding their unchanged source,
compiler, tool and protected input hashes. This avoids rebuilding an unchanged
compiler or re-emitting the entire unchanged corpus.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 4,801 |
| Unit tests / fbcunit | 703 | 16 | 14,347 |
| Production corpus/projects | 1,117 | 490 | 11,104 |
| Additional projects | 239 | 1,805 | 41,133 |
| Total | 2,283 | 2,311 | 71,385 |

Six Clang `-Wformat-signedness` occurrences disappear. No other group changes.
The selected queues now contain no occurrences of that warning. Receipts and
the source preimage are under `build/c-emitter-byte-write-20261102`, with a
16 MiB retained-stage limit. Twelve PNG profiles remain excluded. Unselected
source contexts and the broader compiler warning campaign remain open.

## Variadic-start experiment

The preceding experiment in `build/c-emitter-varargs-start-20261102` is resolved
as unpromoted. Both local toolchain headers choose `__builtin_c23_va_start` for
C23. Its one-argument form executes correctly in the native C probe but adds
Clang's pre-C23 compatibility diagnostic and exposes unused named parameters.
Supplying the narrow final parameter retains the varargs warning. Supplying an
ignored zero also warns. GNU11 rejects the C23 builtin.

No compiler change, dummy parameter read, ABI signature change or diagnostic
suppression was promoted. The two selected `-Wvarargs` occurrences remain
counted. A complete implementation must address C11's parameter constraint,
C23 compatibility diagnostics, conditional parameter naming and ABI behavior
together. The probe receipts and `decision.json` retain this boundary.

<!-- end of c-emitter-byte-write.md -->
