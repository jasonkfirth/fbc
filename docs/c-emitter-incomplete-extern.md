<!--
Project: FreeBASIC compiler verification
File: c-emitter-incomplete-extern.md
Purpose: Record the incomplete external object and profiler marker repairs.
Responsibilities: Explain C object extent, diagnostic changes and saved checks.
This file intentionally does NOT qualify a complete cycle profiling session.
-->

# Incomplete external objects, November 2, 2026

The C backend previously represented an unresolved external object as `char`.
The parser correctly permitted its address and rejected operations requiring a
layout, but the C declaration invented a one-byte object extent. Address-only
objects now use an incomplete C record declaration with a private tag. Linker
names, pointer representation and by-reference calls retain their existing ABI.
Objects whose forward types become complete still use their completed types.

The cycle profiler in `C:/fb_corpus/fbrtLib/profile_cycles.bas` declares its two
linker section boundaries with an unresolved record type. These are addresses
around a variable-length record region, not byte-sized objects. Merely changing
the BASIC declaration did not help with the old C-emitter surrogate; both repairs
are needed. The first private control retains that failed diagnostic comparison.

## Result

Evidence is in `build/c-emitter-profile-markers-20261102`. Its noninstalled
compiler is `compiler/fbc.exe`, SHA-256
`c210d207554b900d6399471c16c621b431f0091bb8cc77a4247b7c034c30ac88`.
Only the HLC object was replaced on the pinned typed-callback compiler. The old
link was reproduced first, and the live pre-repair HLC emitted exactly its prior
C before the edit. Header, tool, installed binary and inherited object pins were
checked around the build. The mixed source snapshot is recorded in its receipt;
later unrelated frontend edits remain outside this compiler's qualification.

The compiler retains zero GCC warnings and 4,790 Clang warnings in its selected
224-unit build profile. The changed HLC unit has the same warning groups and
counts as before. GCC bounds warnings fall from four to zero in each of the
profiler's single-threaded and multithreaded production profiles, removing eight
counted warnings. Their Clang counts are unchanged.

| Selected non-PNG cohort | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Pinned compiler build | 224 | 0 | 4,790 |
| Units and fbcunit support | 703 | 19 | 14,356 |
| Production configurations | 1,117 | 490 | 11,250 |
| Additional projects | 239 | 1,810 | 41,141 |

Counts describe repeated compilation profiles, not distinct source defects.
All diagnostic flags remain enabled. These totals compose the prior declaration,
JSON and callback receipts with the newly compiled profiler profiles.

## Verification

- Sixteen production object checks pass: old/new profiler sources, both thread
  modes, Win32/Win64 and GCC/Clang under expanded unsuppressed GNU23 O2 diagnostics.
  Four additional object checks use the promoted canonical source directly.
- Sixteen old/new native fixture executions pass across GCC/Clang, GNU11/GNU23
  and O0/O2. The reusable canonical runner passes eight further executions.
  The actual production helpers count records, honor collection limits, preserve
  output slots and sort through their real callback.
- The GCC/Clang backend suite passes. It now checks that the incomplete object
  fixture's emitted declaration preserves its unknown extent, in addition to
  executing address and by-reference checks and rejecting incomplete value uses.
  Separate old/new controls reject the old declaration at both pointer widths.
- Source-only strict lint introduces no warnings: the two production files retain
  six existing maintenance/whitespace warnings and the new BASIC fixture is clean.
- Fresh C identities are recorded in `c-identity/receipt.json` for all 2,059
  selected noncompiler profiles. The two profiler profiles use their newly
  compiled canonical C checks. Other profiles retain their accepted object checks.
  Twelve PNG profiles are excluded.

The first native harness failed because its global assembler block changed to
`.data` without restoring `.text` before C function bodies. The fixture now
restores the section explicitly, and the complete native matrix passes. Failed
harness logs remain available; their obsolete objects and executables were retired.

## Limits and resources

The complete live compiler tree is still unqualified. Its structure check reports
13 missing header declarations in concurrently added semantic-tooling files,
which were preserved. Compiler-backed semantic lint, foreign execution and a full
cycle-profiling session are separate unfinished checks. The nonlocal ERROR frame
issue and other unsuppressed warning groups remain open.

The reusable profiler test is under
`C:/fb_corpus/fbrtLib/tests/c-backend-regression`. No installed compiler, runtime
library or project binary was replaced. One scratch C/object/executable set is
reused; receipts compress diagnostics. PNG work remains excluded.

<!-- end of c-emitter-incomplete-extern.md -->
