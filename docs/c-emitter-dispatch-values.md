<!--
Project: FreeBASIC C backend qualification
File: c-emitter-dispatch-values.md
Purpose: Record dispatch placement, warning counts and native continuation evidence.
Responsibilities: Separate qualified source snapshots from later edits and existing lifetime defects.
This file intentionally does NOT claim general ERROR unwinding or suppress diagnostics.
-->

# C continuation dispatch, November 3, 2026

The emitter used to append a bypass around its indirect dispatch block at the
end of a procedure. After RETURN or END that bypass could never execute. The
block now lives in the header, with an initial branch into the source body.
Returning and falling-through bodies need no artificial tail branch. Actual
GNU label addresses, operand evaluation and the activation-local target remain
unchanged. The reserved fb_c_ namespace also covers the new body label.

The complete selected snapshot loses 273 Clang warnings, reducing its total
from 8,763 to 8,490. GCC remains at 2,141. Unreachable-code observations fall
from 304 to 31; no profile gains one, and every other warning class is unchanged.
The remaining observations are in Stunts and WSR profiles and have separate
source/control-flow causes. No warning option is disabled.

The canonical compiler, SHA-256
`df374b68feb42c4c611f529e055736b8fab948bc7e06c57d986099c893ab8979`,
passes 224 C fixed-point checks with zero GCC and 1,652 Clang warnings. The full
backend suite, eight semantic tests in bounded partitions, and the structure
guard pass. Strict source-only lint has zero errors; success-marker advice is
retained for the fixtures.

The permanent dispatch fixture checks local ERROR/RESUME, retry, recursive
activations, scalar/string values, void fallthrough, real GOSUB transport,
inline assembly and termination. Both C producers and both C tools run under
GNU11/GNU23 and O0/O2. Of 96 baseline/candidate executions, 80 pass and sixteen
retain the same valid-source automatic-GOSUB lifetime failure, eight per
provider. Eight Win32 objects pass. Two GAS64 controls pass the automatic-local
case, preserving evidence that it remains a C-backend defect. The dispatch
change introduces no native regression and does not claim to repair that defect.

Fresh mixed GAS64/C checks also preserve their earlier results: C-owner/GAS64-
worker passes 8/16 and GAS64-owner/C-worker passes 12/16 for each provider.
There are no new failures; the existing twelve failures per provider remain
open. Both GAS64 objects retain their previous bytes. The private owner and
token experiments are not included in this compiler.

The main audit qualifies 2,251 stable profiles. The 32 Optical profiles receive
separate baseline/candidate checks on bound input snapshots so concurrent
project changes are not attributed to the compiler repair. Their warning totals
are unchanged between providers. Optical changes again after qualification;
all 32 live profiles therefore remain explicitly pending for stable inputs.
The table below is the last qualified snapshot, not a live count for those edits.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Current compiler | 224 | 0 | 1,652 |
| Unit tests / fbcunit | 703 | 15 | 829 |
| Production corpus/projects | 1,117 | 443 | 2,569 |
| Additional projects | 239 | 1,683 | 3,440 |
| Total | 2,283 | 2,141 | 8,490 |

Receipts are under `build/c-emitter-unreachable-20261103/`. The combined
`audit/results.sqlite3` and `audit/receipt.json` retain the snapshot comparison.
`audit/stable-results.sqlite3.gz` preserves the earlier stable-only database
exactly, and the first Optical database is also archived with a verified round
trip. The incremental replay reuses diagnostics only for matching C bytes,
tools and flags. These closures avoid another full database copy.

Installed binaries and unrelated dirty work remain unchanged. PNG and the
user-closed intermittent-crash investigation stay excluded. The stage has a
64 MiB retained-artifact cap, reuses scratch C/objects and uses at most two
heavy workers at below-normal priority. The overall warning cleanup is open.

<!-- end of c-emitter-dispatch-values.md -->
