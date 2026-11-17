<!--
Project: FreeBASIC C backend qualification
File: c-emitter-vararg-promotions.md
Purpose: Record unsigned variadic operand ranges and the Coreutils source refresh.
Responsibilities: Bind native argument values, format diagnostics and source ownership.
Application execution and other warning classes retain separate qualification.
-->

# Unsigned variadic argument ranges, November 16

Development main `0be13d79` retains unsigned byte and short expressions at
variadic calls. Native C performs their default promotion to its 32-bit `int`.
The semantic ABI output records that promoted type. Explicit source `Long`
casts keep their own conversions, and other backends retain their existing
promotion handling.

The preceding explicit promotion loses the operand's unsigned range in native
format checking. The registered `c-va-promotions` regression covers direct and
indirect native calls, a BASIC variadic reader, all 256 byte values and nine
short boundaries. Sixty-four Windows/Linux executions cover both revisions,
both C producers, both native tools, 32/64-bit targets and O0/O2. All 147,456
formatted-output comparisons pass. Each execution also checks 4,608 native
argument observations and the BASIC reader. The repaired modules have zero
format diagnostics under the complete unsuppressed audit options.

Wrong string arguments, signed operands used as strings and dynamic formats
retain native diagnostics. The focused regression checks the emitted explicit
source casts. The complete normal backend suite passes in 600.237 seconds.
Its first run stopped in the variant library helper, whose old failure report
omitted the exit and timeout status. Both compiler revisions compile that input
in direct controls. The helper now permits compilation to finish and reports
both statuses, retaining its runtime guard and bounded scratch tree.

## Translated library arguments

C2FB also inserted explicit widening casts at these calls. Its revised normal
translator CLI preserves narrow unsigned operands and retains explicit C source
casts. GCC and Clang converter builds pass the new unsigned-argument regression,
the existing enum-width regression and the pointer-qualifier body regression.

The reviewed library refresh removes seventeen widening casts across eight
cksum variants, pr, shred, tr, od and ls. Each operand is an unsigned byte.
Numfmt's debugging format additionally receives its existing typed
`MAX_UNSCALED_DIGITS` constant. Its preceding bare BASIC constant had 64-bit
type on the target while the format consumed C `int`.

Both producers emit identical C for each refreshed module. Twenty-seven of
twenty-eight native object pairs match byte for byte. Clang's numfmt object
reorders three independent register moves before `fprintf`. Its complete
symbols, relocations and remaining disassembly match. The same value 18 reaches
the same argument register.

Coreutils reaches GCC 166 / Clang 1,785. The GCC format-type class falls from
two observations to one, retaining the POSIX grouping-flag observation. All
eighteen Clang format-type observations close. The 130 nonliteral-format
observations retain their recorded review scope. SQLite stays at GCC 21 /
Clang 7,696. Its two compiler-changed modules retain identical objects under
both native tools.

## Reproduction and pending checks

The normal backend test entry point includes the new fixture and its diagnostic
checks. The Coreutils converter's normal integration regression entry point
includes the unsigned-argument test.

```sh
python3 -B build_scripts/test-compiler-backends.py \
  --root /path/to/fbc --fbc /path/to/compiler \
  --toolchain-prefix /path/to/toolchain --backend gcc --backend clang
```

Receipts under `build/c-emitter-forward-views-20261105` use the
`vararg-promotions` prefix. The full compiler self-host reaches an exact
224-unit GCC-clean fixed point. Completed 1,053-profile port emissions and the
fourteen-source overlay bind current production inputs and original native
options. The `port-overlay-native-result-v3-20261116` receipt records the
object review and current counts.

Development-main selection passes assertion-enabled executions through both
normal C backend entry points. The independent installed compiler and nine
other protected products retain their hashes. The diagnostic helper was
strengthened after selection to reject injected casts directly. The complete
backend suite and focused diagnostic matrix pass with that helper version.

Templari's current client/server recipes qualify 676 owning contexts with both
C producers at GCC 2 / Clang 54,173. The refreshed GUI inputs add 390 padding
observations to its preceding source cut. Midisoft, Rotator3d, FS4 and HART
qualify 146, 16, 125 and 153 standalone profiles respectively, all GCC-clean.
HART also retains its separately recorded included-provider disposition.
These checks compile their owning sources. Application execution retains its
project-specific qualification.

Both fresh Make-selected Linux runtime builds are GCC-clean. MT reaches
Clang 10,023 and non-MT reaches Clang 10,021 across 2,072 native object profiles.
The 332 current MT execution contexts and twenty non-MT public executions pass,
including 2,528 assertions in the existing non-MT FORMAT suite. The refreshed
runtime cut preserves thirteen concurrent runtime/Make edits. A later
graphics-only Make edit has identical runtime compile/archive commands in all
four native-tool/configuration contexts. Its two publication fingerprints per
context change with the included Makefile, as required by the build cache.

The source-only linter reports zero errors and retains 284 existing FBL002
long-line advisories across the seventeen changed existing BASIC sources. The
new BASIC fixture has zero findings. Eight validated semantic models preserve
identical C and record eighty narrow-source arguments with promoted Long ABI
types. A portable sixteen-file Coreutils source patch applies to an isolated
baseline checkout and reproduces the qualified source bytes.

Templari's serial worker permits a 32 MiB emitted module within a 64 MiB
scratch limit and peaks at 20,893,918 bytes. The fresh MT and non-MT workers
peak at 23,860,689 and 16,894,940 bytes. Each completed worker removes its owned
temporary build tree. Completed transport payloads and a duplicate converter
image are removed, releasing 26,701,528 bytes.

The complete retained queue qualifies all 6,419 entries at GCC 14 / Clang
16,433. A Scorched Earth input refresh and two TurboTrek recipe refreshes close
all source drift and newly exposed include failures. The current TurboTrek
release script supplies the GUI include root used for those two contexts.
All 813 previously pending synchronized GUI inputs are replayed through their
owning contexts. The queue retains 46 PNG exclusions, six included-provider
entries, two assembly dispositions and twenty existing frontend failures.

The final source gate catches one subsequent Templari client input-module edit.
Receipt `vararg-promotions-templari-v3-20261116.sqlite3` rechecks its owning main
context and reuses the identical source/tool inputs of the other 675 contexts.
Both native warning totals remain unchanged. Its separately named v3 input
archive preserves every current opened source version.

The refreshed navigation-profile smoke source adds two unused private-helper
observations under both native tools. Their emitted definitions have no
surviving C callers. Their FBC diagnostic and procedure-reachability follow-up
remains open. The last-qualified disjoint GCC total is 203, with one
Coreutils observation removed and two new GUI observations relative to the
preceding total of 202.

A verification reader initially held the replay's SQLite database open during
source checks, stopping its writer after profile 6,360. The reader was released
and the writer resumed from saved profiles. Subsequent readers materialize
their rows and close the database before source checks. The complete source
reconciliation passes. The wider cleanup goal remains active, including three
finite-format compiler sites, seventeen dynamic-format contracts and the
remaining GCC and Clang warning owners. The full C2FB integration suite retains
separate qualification beyond this batch's three focused regression groups.

Two later concurrent edits in `src/rtlib/console/io_printusg.c` and
`src/rtlib/drivers/unix/io_readstr.c` await native replay. The runtime counts
above bind the completed packet-v42 qualification. Both newer source versions
are preserved in `vararg-promotions-runtime-pending-v1-20261116.json.zlib`.
Compiler and project input checks are current.

<!-- end of c-emitter-vararg-promotions.md -->
