<!--
Project: FreeBASIC compiler verification
File: c-emitter-destructor-reachability.md
Purpose: Record complete-destructor reachability and static cleanup roots.
Responsibilities: Explain ownership contracts, native tests and warning receipts.
This file intentionally does NOT claim complete project or foreign execution.
-->

# Private complete-destructor reachability

Private complete destructors were excluded from the C procedure graph's
discardable bodies. Three selected SHA2 profiles emitted twelve uncalled generic
shared-array destructors each. Complete destructors can now be graph candidates;
public, virtual and deleting destructors keep their existing emission rules.

Static cleanup wrappers are generated after ordinary body reachability settles.
They need an explicit root, even when an unrelated dead body directly calls the
same destructor. The earlier static-queue flag describes pending traversal, not
ownership by such a wrapper. `CPROC_STATIC_ROOT` records that separate fact,
survives graph initialization and roots the complete destructor. Its calls and
dependencies are then traversed by the existing graph. Global instance wrappers
are already generated before analysis. No destructor signature, type layout or
language-level ownership rule changes.

The permanent fixture covers implicit field cleanup, local and allocated arrays,
global fields, a static whose destructor has a dead direct caller, and virtual
derived/base cleanup. An unobserved private owner and its destructor disappear.
Both C producers, GCC/Clang, release/debug, GNU11/GNU23 and O0/O2 pass 32 native
executions. Forty main objects include eight Win32 checks. Eight foreign objects
cover Linux x86_64, ARMv7, AArch64 and Darwin AArch64. These are object checks,
not execution on those targets. Four controls cover the old unpruned destructor
and a compiler variant missing the static-root mark under both C producers.

The existing reachability suite, including nested static cleanup, copy-back,
assembly fallback and GAS64 execution, passes again. The ordinary GCC/Clang
backend suite checks the complete cleanup output and rejects the historical
compiler's unobserved destructor body. Its initial expected output omitted
post-main cleanup messages; that harness error is resolved and its failed log
is retained. The structural guard uses debug emission, matching the profiles
that produced the warnings; the older compiler already omits that unobserved
body in release output through its later C pass. The initial release-guard
observation is retained separately. Source-only strict fblint reports three files with zero findings.
The structure check passes 393 documented files and 21 host source graphs.
The broader semantic lint limitation remains open.

The noninstalled compiler is
`build/c-emitter-destructor-reachability-20261102/accepted/fbc.exe`, SHA-256
`1ea4f02fd67807cee9cc1dee6631c6c6e724424e6a762f9ddabcc63ee32787ac`. Bootstrap and self-host binaries match. All
224 final compiler C outputs close the fixed point; its diagnostics remain
zero GCC and 4,802 Clang warnings.

| Queue | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Compiler | 224 | 0 | 4,802 |
| Unit tests | 703 | 16 | 14,347 |
| Production | 1117 | 449 | 11,051 |
| Additional projects | 239 | 1,683 | 39,758 |
| Total | 2,283 | 2,148 | 69,958 |

This repair removes 36 GCC and 36 Clang warning
occurrences. 40 project C outputs change and are recompiled under
their original unsuppressed profiles; 2019 unchanged outputs retain their
bound checks. Twelve PNG profiles remain excluded. Counts include repeated
source configurations, and object qualification does not establish complete
game execution. Installed tools and unrelated source/changelog bytes are
preserved. This stage is capped at 64 MiB.

Receipts, controls, source preimages and logs are in
`build/c-emitter-destructor-reachability-20261102/summary.json`. The
[current status](c-emitter-current-status.md) records the unfinished campaign.

<!-- end of c-emitter-destructor-reachability.md -->
