<!--
Project: FreeBASIC C backend qualification
File: c-emitter-debug-parameters.md
Purpose: Explain ignored calling-contract parameters in debug output.
Responsibilities: Preserve formal storage, source diagnostics and native warning evidence.
This file does not claim complete corpus cleanup or installed promotion.
-->

# Ignored parameters in debug C

The retained cohort has 176 GCC unused-parameter observations across 43 debug
profiles. Removing formal names, as the non-debug C23 path does, would lose
debugger-visible arguments. FreeBASIC already reports unread source parameters
with warning 54 under `-w all`; C lowering must preserve that diagnostic.

The backend inspects the complete emitted body and retains each formal name.
For an unreferenced formal, it emits a local enum declaration whose value is
`sizeof(&parameter)`. This acknowledges the required ABI storage without reading
the argument, evaluating an address or creating runtime storage. The declaration
works in GNU11 and GNU23 and precedes the ordinary local declarations. It adds
no newline, so existing debug source-line mappings stay unchanged.

This proof is separate from dead-store removal. Debug locals and parameter
values retain their existing storage. Naked procedures and bodies containing
general opaque assembly retain their conservative behavior and C diagnostics.
The non-debug C23 unnamed-parameter path and GNU11 fallback are unchanged.

## Named-parameter qualification at the preceding compiler cut

The preceding selected development compiler is
`9d60dd5092770ecf583c586237b0ccf4c604f3dcfe58a327e6a6950fbc182b98`.
It reproduces 224 compiler C units at a fixed point with zero GCC warnings,
reusing 223 unchanged objects and then all 224 in the fixed-point round.
After the JRPG audit finished all 241 profiles, ten selected-prefix value runs
passed across GCC and Clang. The previous immutable compiler remains available
as rollback. Installed release binaries and runtime archives are unchanged.

The regression passes 16 native value runs across both C producers, GCC and
Clang, GNU11/GNU23 and O0/O2. Four GDB runs inspect all six ignored formal
arguments and complete the program. Removing only the enum declarations gives
exact prior debug C for both producers. All seven source unread-parameter
warnings remain, and four opaque-assembly controls retain their native warning.

All 43 affected retained profiles pass both native compilers under their
original aggressive unsuppressed flags. GCC warnings fall from 183 to 7 and
Clang warnings from 582 to 406: all 176 unused-parameter observations disappear
from each tool, and every other warning class is identical. All 165 existing
source warning 54 observations in those profiles are identical too. The
checkpoint working set peaks at 19,531,279 bytes; fixed scratch slots are reused.

Eight semantic models pass the independent reader across both C producers,
32/64-bit targets and text/binary transport. Export preserves eight exact C
identities. Four Win32 C objects pass the unused-parameter error gate; these
objects are not Win32 execution evidence.

Both complete C-backend suites pass in 218.531 seconds, including 462 source
warning checks. Compiler structure checks cover 409 documented files and 21
host graphs. Strict lint is clean for the three owned BASIC files.

The new debug fixture also exposed a pre-existing reader defect, reproduced
with both compiler generations: tabs inside literal source/debug text were
split as structured fields. The reader now splits that final free-form payload
once, retaining its exact text. Other union classes keep exact field arity and
discriminator checks. Four focused payload tests cover tab preservation, wrong
unions, extra structured fields, omitted records and both raw type encodings.

An intermediate statement acknowledgement introduced Clang's
`-Wdeclaration-after-statement` in the first original-flags corpus recheck and
was rejected. A static-assert experiment also introduced a compatibility
diagnostic and was not used. The final declaration-only form passes both
native C dialect controls without either diagnostic. Failed receipts remain
separate from accepted qualification.

Receipts reuse `build/c-emitter-forward-views-20261105/` with the
`debug-abi-selfhost-v3-`, `debug-abi-values-v3-`, `debug-abi-semantic-v3-`,
`debug-abi-gates-v3-` and `debug-abi-parameter-profiles-v3-` prefixes.
The complete retained refresh now covers 6,419 profiles and 6,313 accepted C
units, GCC 22 / Clang 15,831. Every other warning class is unchanged from the
callback compiler. Fourteen further fbrtLib test contexts retain exact prior C
and add GCC 1 / Clang 6 on this same compiler. The expanded completed cut is
6,433 profiles, 6,327 accepted C units, GCC 23 / Clang 15,837.

The full refresh reused 6,309 exact native results per tool and peaked at
29,052,687 owned bytes under a 64 MiB cap. Its twenty frontend refusals preserve
their exact diagnostic signatures. The guarded-label runtime proof remains
bound by exact C and artifact identities. No applications or unrelated native
library edits receive new qualification credit. The `debug-abi-cohort-close-v3-`
and `debug-abi-selected-publish-v3-` receipts record these checks and promotion.
See the [current census](c-emitter-current-status.md) for separate new-project
counts and remaining warning classes.

## Executable private debug bodies

The additional fb-ext-lib sweep exposed 589 unused-function observations in
debug output. These functions have source bodies, often from library template
macros. The C reachability pass already retains them as debugger entries after
their callers disappear, but the native optimizer could discard the actual code.

The selected follow-up compiler separates actual native-call roots from roots kept only
for debugging. It applies `__attribute__((used))` only to debugger-only source
entries, as the backend already does for retained debug globals. Ordinary called
bodies, generated default members and read-only implementations retain their
native optimization. A separate original-ABI wrapper is retained only when that
source entry is needed. Prototypes and function-pointer types stay unchanged.
Optimized debug objects can grow because the source bodies now have executable
entries. No native warning flag or diagnostic attribute is used to hide them.

The regression gives one source body a caller that folding eliminates. The old
compiler fails the native unused-function error gate. The corrected compiler passes 16
native value runs, eight optimized assembly-entry checks, four debugger runs,
two exact normalized debug-C comparisons, four opaque-assembly controls, eight
independently read semantic models and four Win32 objects. The 224-unit fixed
point is GCC-clean. Both complete C-backend suites pass in 237.692 seconds,
including 462 source-warning controls. Strict lint is clean for the five
changed BASIC files. The original source
parameter warnings remain. Six real profiles also preserve all nine pre-existing
GCC data-flow observations. A broader intermediate attribute policy removed those
observations along with the unused-function warnings and was rejected. The
complete real-project recheck now covers all 194 profiles: GCC 666 to 77 and
Clang 816 to 227. All 589 unused-function observations disappear per tool, with
every other warning class identical in each profile. All 186 accepted C files
match after removing only the new attributes; all 658 source parameter warnings,
250 source variable warnings and eight harness-refusal signatures remain. The
original native-check peak is 3,032,346 bytes. The complete retained recount now
accepts 6,313 C units across 6,419 profiles, GCC 18 / Clang 15,827. Including the
fourteen rebound fbrtLib contexts gives GCC 19 / Clang 15,833. Ten selected-view
value runs pass; the existing named-parameter compiler is the rollback.

The final compiler also corrects semantic definition headers for no-argument
procedures. A late graph decision can remove a provisional retention attribute
from an ordinary called body. Its exported definition header now waits for that
decision through the owned ABI cache, even when there are no formals. The
regression reproduces eight failures on the preceding candidate and passes all
eight producer/target cases after the fix, with exact C identity when export is
enabled. No wire schema or native calling convention changes.

The shared native helper probes C23 spelling and pointer width, retaining
GNU2x on older tools and the same diagnostic gates. A later CI-only edit added
native capability unit controls; those six tests pass independently, and the
backend gate's source-pin reconciliation records that sole input delta.

Current receipts use the `debug-body-selfhost-v5-`, `debug-body-values-v5-`,
`debug-body-semantic-v5-`, `debug-body-gates-v5-`, `debug-noarg-header-v5-`,
`debug-body-class-close-v5-`, `debug-body-cohort-close-v5-`, and
`debug-body-selected-publish-v5-` prefixes. The `current-fbext-debug-body-v5r3-`
database freshly re-emits all 194 contexts with exact prior C. Its comparison
normalizes CRLF/LF line endings only. Earlier rejected attribute policies and
the accepted V4 native-class census retain separate receipts.

<!-- end of c-emitter-debug-parameters.md -->
