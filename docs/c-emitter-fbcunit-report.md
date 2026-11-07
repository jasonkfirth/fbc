<!--
Project: FreeBASIC compiler verification
File: c-emitter-fbcunit-report.md
Purpose: Record fixed report formatting and stream lifecycle qualification.
Responsibilities: Explain the warning delta, observable behavior and receipts.
This file intentionally does NOT change unit execution or result collection.
-->

# fbcunit report writer, November 2, 2026

`tests/fbcunit/src/fbcunit_report.bas` now uses a fixed `"%s%s"` format and owned
padding storage. Empty padding/text use real empty string literals. The write
still uses one `fprintf` call and preserves normal report bytes. Successful
writes return true; the original Boolean function always returned false.

Closing a report clears its handle, including on a close error. Opening another
report resets indentation, and public report finalization closes the stream.
This makes repeated close and write-after-close checks operate on the reporter's
state instead of a stale handle. The existing serialized callback design remains.
The original license text, unrelated code and LF line endings were preserved.

Evidence lives in `build/c-emitter-fbcunit-report-20261102`:

- Sixteen old/new native cases pass across GCC/Clang, GNU11/GNU23 and O0/O2.
  Normal report output is compared exactly after newline normalization. New
  cases check empty writes, write results, replacement streams, indentation,
  repeated close, closed-state writes and finalized XML.
- Eight old/new Win32/Win64 production object checks pass. The canonical owning
  Win64 profile was then emitted and compiled with both C compilers directly.
- One GCC and one Clang `-Wformat-nonliteral` warning are removed. All other
  groups in that profile remain unchanged.
- Fresh C witnesses cover all 703 selected unit profiles: 702 remain unchanged;
  the report support unit matches its newly accepted canonical object checks.
- The expanded GCC/Clang backend suite passes, including the reusable report
  fixture and stdio, x86-64 arithmetic/layout and TLS checks. Linux i386 GAS
  arithmetic is assembled into a checked ELF32 object without claiming execution.
- Source-only lint retains seven existing whitespace findings and introduces
  none. The new BASIC fixture is clean.

## Report-repair checkpoint totals

The later [runtime alias repair](c-emitter-runtime-aliases.md) supersedes this
table with a fresh queue recount and two fewer GCC warnings.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 4,830 |
| Unit tests / fbcunit | 703 | 18 | 14,354 |
| Production corpus | 1,117 | 490 | 11,222 |
| Additional projects | 239 | 1,810 | 41,140 |
| Total | 2,283 | 2,318 | 71,546 |

These are unsuppressed warning instances across selected profiles, including
repetitions. The compiler is the complete current-source snapshot documented in
the [stdio qualification](c-emitter-stdio-contract.md). Twelve PNG profiles are
excluded. Installed binaries were preserved, and no fbcunit library was rebuilt
or installed. The broader campaign remains incomplete.

<!-- end of c-emitter-fbcunit-report.md -->
