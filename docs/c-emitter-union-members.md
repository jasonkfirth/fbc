<!--
Project: FreeBASIC compiler verification
File: c-emitter-union-members.md
Purpose: Record typed overlapping-member selection and its native checks.
Responsibilities: Explain the repair, preserved diagnostics and qualification.
Application execution follows each project's own build and test workflow.
-->

# Typed union member selection

The C member-path search previously accepted the first overlapping member with
a compatible representation. A union whose first pointer member referred to
const data could therefore supply the address for a writable pointer member.
The emitted cast then discarded that first member's qualifiers. This affected
valid BASIC stores in Coreutils' `printf-args` module.

The search now tries the requested datatype, subtype and pointer qualifiers
through the union before using its representation-compatible search. The
existing raw-word copy path remains available when the union has no member of
the requested integer type. Failed searches retain ownership of the original
expression. Packed records continue through their established byte-copy path.

Explicit source const removal retains FBC warning 40 under `-w constness` and
the native `-Wcast-qual` diagnostic. The regression checks both through the
normal compiler entry points. The complete backend suite registers the BASIC
fixture and its diagnostic checker. The public checker also compiles and runs
the independent C observer at O0/O2, so these value checks can be reproduced
through the repository test entry point.

Run it with the compiler and SDK prefix being qualified.

```text
python -B tests/compiler-support/c-union-member-qualifiers.py --fbc /path/to/fbc --prefix /path/to/sdk --backend gcc
python -B tests/compiler-support/c-union-member-qualifiers.py --fbc /path/to/fbc --prefix /path/to/sdk --backend clang
```

## Focused native qualification

The immutable candidate is `ba3b0a985dd36684f41563cb3f91f94239dffa3ab5cbe28ccbc005fcae40d621`.
Its two-round 224-module GCC self-build reaches identical emitted C on the
second round, with zero GCC warnings.

Windows and Linux checks cover both pointer widths, both C producers, both
native compilers and O0/O2. Sixty-four before/after executions pass 229,376
cases and 737,728 checks. An independent C observer verifies layouts, pointer
slot addresses, packed stores, raw-word copies and surrounding bytes. All
sixty-four explicit const-removal controls retain their source warning and
fail native compilation under `-Werror=cast-qual`. The original valid fixture
reproduces three spurious cast-qualifier diagnostics in each native context.

The current valid fixture has zero GCC warnings in this matrix. Linux Clang
retains eight byte-access observations across its valid current contexts.
The serial workers remove their scratch directories. Their peak owned scratch
sizes are 289,124 bytes on Windows and 39,327 bytes on Linux.

Source-only strict lint checks the member-access helper and BASIC fixture,
reporting zero errors and one advisory about the fixture's success-marker
`Print`. The marker supplies the backend runner's expected result.

The initial Windows worker stopped at an observer hash check because text-mode
output changed line endings. The subsequent worker writes the captured bytes
and checks the same hash on both hosts. The failed receipt remains recorded.

Receipts are under `build/c-emitter-forward-views-20261105`.

- `union-member-selfhost-v1-20261116.json.zlib`
- `union-member-packet-v1-20261116.json.zlib`
- `union-member-native-windows-v2-20261116.json.zlib`
- `union-member-native-linux-v2-20261116.json.zlib`
- `union-member-lint-v3-20261116.log`

The complete normal GCC/Clang backend suite passes in 621.537 seconds. Re-emitting
all 1,053 current port modules changes five Coreutils modules and one SQLite
module. All six changed modules have identical C from both producers. Fresh
GCC/Clang compilation of their complete before/after modules retains all twelve
native object hashes. Coreutils reaches GCC 159 / Clang 1,782 after the two
spurious `printf-args` warnings disappear from each native compiler. SQLite
retains GCC 18 / Clang 7,696. Source diagnostics remain identical.

The production receipts are `union-member-ports-v1-20261116.json.zlib` and
`union-member-port-native-result-v1-20261116.json.zlib`.

## Complete owning queues and selection

The 6,419-profile retained replay completes at GCC 14 / Clang 16,433. Its three
changed C profiles compile successfully. All 224 compiler modules remain
GCC-clean with Clang 1,340. All 703 fbctests remain GCC-clean with Clang 771.
The original PNG, provider, assembly and frontend dispositions remain recorded.

Templari's current graph contains 395 client and 282 server contexts. Both C
producers pass all 677 contexts at GCC 2 / Clang 54,317. The added
`external_xmpp_pep_notify.bas` server module contributes 143 padding observations
and one startup-registration observation. Its complete C is identical under the
preceding compiler and candidate, with both C producers. The other contexts
retain their preceding aggregate counts.

Midisoft's 149, Rotator3d's sixteen, FS4's 125 and HART's 153 standalone profiles
all remain GCC-clean. The additional HART included-provider context retains its
owning-reader checks. This replay closes the five previously pending Midisoft
UI input versions.

The public checker gained its C-observer executions after the complete suite
run. Its two normal invocations pass again, with all other suite input hashes
unchanged. The standalone smoke check now reads the pointer stored through its
const view. A second refresh confirms all 64 library-body C identities against
the native matrix, and the final ordinary GCC/Clang entries pass warning-free
with assertions enabled.

The initial standalone check compared pointer-to-pointer types with different
nested qualifiers. Native C reported `-Wcompare-distinct-pointer-types`, and
selection restored the preceding compiler. The captured smoke preimage retains
that separate pointer-comparison lowering follow-up. The current smoke check
verifies the stored value, while the independent C observer checks slot addresses.

Development main now selects `ba3b0a98`, with `626a5305` retained for rollback.
All ten protected products retain their hashes. The selected-view receipt is
`union-member-selected-v2-20261116.json.zlib`. The current public-checker refresh
is `union-member-suite-v3-20261116.json.zlib`.

The disjoint last-qualified GCC total is 193. Twelve dynamic format contracts,
two finite-format compiler sites and the pointer-comparison follow-up remain
open. Concurrent runtime work adds nineteen common/runtime input versions and
brings the captured DOS, Haiku and BSD platform inventory to 46 versions awaiting
owning native replay. Linux runtime totals retain their tested packet-v48 cut.
One later `gfxlib2/gfx_unicode.h` version also retains its owning native replay.
These counts bind the captured qualification cut. Subsequent concurrent source
versions retain follow-up against their own bytes.
The overall warning-cleanup goal remains active.

<!-- end of c-emitter-union-members.md -->
