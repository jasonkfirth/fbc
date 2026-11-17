<!--
Project: FreeBASIC C-backend verification
File: c-emitter-coreutils-formatter.md
Purpose: Record shared formatter ownership and the largest GCC format cluster.
Responsibilities: Bind source corrections, native exports, diagnostics and values.
Full application execution and other format sites have separate qualification.
-->

# Current formatter replay, November 16

Development main `626a5305` freshly emits all six formatter providers through
both C producers with identical C. The permanent
`build_scripts/test-coreutils-vasnprintf.py` links the actual `vasnprintf`,
`printf-args`, `printf-parse`, `realloc`, `xsize` and `mbszero` modules, plus
the captured production long-double runtime implementation. The final matrix
uses the translated state-reset provider. Initial link probes used a native
state-reset shim.

Four unsuppressed GCC/Clang O0/O2 executions pass 65,012 cases and 37,119,372
checks. They cover all 33 argument categories, C23 fixed/fast integer-width
directives, integer limits, floating limits, signed zero, infinities, NaNs,
embedded NUL characters, UTF-8/wide strings and five caller-buffer modes.
Returned lengths, bytes, buffer identity, red zones and tracked allocation
ownership are checked against native libc. Each run injects seven allocation
failures in child processes. Five return the formatter's ENOMEM result, one
recovers and one takes the translated automatic-storage ERROR 7 exit.
Process termination supplies the allocation disposition for that last path.

GCC line coverage reaches 93 of the 99 native calls. The six generic string
fallback calls retain their earlier bypass disposition through the formatter's
dedicated string handlers. Three reached character/pointer precision variants
exercise this GNU host's behavior. Their ISO format restriction remains
recorded in the preceding qualification. All 99 argument lists receive native
canonical type checks, and eight deliberately wrong-type controls retain
format diagnostics. The uchar/ushort dispatch locals use unsigned int, so
their canonical type probes use `%u`. Runtime comparisons exercise the actual
`%hhu` and `%hu` conversions with representable values.

This strengthens an already reviewed dynamic-format owner. All 99 production
nonliteral observations remain counted, and the twelve other pending dynamic
contracts remain open. Fresh independent GCC compilation of all 1,053 port
modules confirms Coreutils at 165 and SQLite at 21. The original 129 Coreutils
nonliteral observations remain visible. The source-only linter reports zero
errors and 32 existing generated-source line-length advisories across the six
providers. No BASIC source changes are needed for this replay.

The permanent formatter runner accepts the six emitted C files in a module
directory and a runtime source directory. Use `--native-flags-json` with the
full recorded options, select `--cc` and `--optimization`, and use `--coverage`
with GCC for the 93-call execution check. Its caller must supply the Linux
x86-64 LP64 production profile and the `C.UTF-8` locale used by the observer.
Each run owns and removes one scratch directory, capped at 32 MiB. The largest
observed scratch footprint is 1,528,273 bytes.
The runner marks a receipt incomplete whenever its final source-hash check
fails. A deliberate input mutation after the last diagnostic compile verifies
that rejection after otherwise successful native execution.

Current receipts under `build/c-emitter-forward-views-20261105` are
`gcc-port-current-packet-v1-20261116.json.zlib`,
`gcc-port-current-native-v1-20261116.json.zlib`,
`vasnprintf-producer-parity-v1-20261116.json.zlib` and
`vasnprintf-matrix-v7-20261116.json.zlib`. The final replay binds runtime
packet v47. Earlier failed probes retain their
failure receipts. The complete cleanup goal remains active.

# Preceding formatter ownership and dynamic formats

The November 14 formatter qualification fixes a real linked-library defect.
The `printf-args`, `printf-parse` and `vasnprintf` translated units each exported
the header's `mbszero` body. Linking their dependency closure with the dedicated
`mbszero` unit failed with three duplicate definitions.

The subsequent [literal-offset and ABS repairs](c-emitter-literal-offsets.md)
bring coreutils totals to GCC 201 / Clang 1,845 on `cd0cc1b6`. The subsequent
[plural-format repair](c-emitter-plural-formats.md) reaches GCC 197 / Clang 1,841
on selected compiler `9c1ebb28`. The ownership qualification below retains its
`cd32db5b` cut.

The subsequent [enum-width review](c-emitter-enum-widths.md) traces all 43
source contracts beyond the 99-site builder and repairs the copyright-year
argument. Fifteen finite-format sites remain compiler propagation candidates,
with 27 additional runtime contracts pending native qualification. The complete
coreutils totals retain GCC 197 / Clang 1,841 after that source correction.

The subsequent [private-format repair](c-emitter-local-formats.md) selects
`f64ca95a`, reaching coreutils GCC 195 / Clang 1,839. The nonliteral class
now contains 140 observations. Thirteen finite-format propagation candidates,
27 additional runtime contracts and nine builder gaps were recorded at that cut.
The subsequent [cleanup and runtime qualification](c-emitter-cleanup-dependencies.md)
closes three floating contracts and records explicit dispositions for those nine
unexecuted sites. Twenty-four runtime contracts and thirteen compiler candidates remain.

## Shared helper ownership

The three callers now retain their external declarations and use the dedicated
`lib/mbszero.c` translation. Their duplicate bodies are removed. Matching
`--exclude-proc=mbszero` entries in `fb_port/port.py` preserve this ownership
when regenerating the units, following the port's existing shared-helper
configuration. The application formatter and parser bodies retain their exact
source text.

The permanent `test_formatter_ownership.py` regression is registered in the
C2FB Make test target. It checks the three regeneration options, external caller
declarations and dedicated owner. The original sources fail the ownership
controls and the repaired sources pass all three tests. Full regeneration from
the original Coreutils C tree remains pending because that tree is absent from
the sparse checkout.

Both C producers re-emit all six dependency units through selected compiler
`cd32db5bb8e210a84c10c62ba38bb3bf61e5bf023cbbcda4663e1bef71f02b3f`.
GCC 14.2 and Clang 19.1.7 compile the before/after object pairs. Native symbol
inspection verifies that each affected object loses exactly the duplicate
`mbszero` export. All other global definitions and native warning counts are
preserved. Strict source-only lint retains 32 existing advisories across the
three changed BASIC files and reports zero new findings.
Two FBC unused-temporary warnings disappear with the removed helper bodies.
Every other FBC warning remains present in the repaired emissions.

## Native formatting values

Sixteen executions cover both C producers, both native C compilers, O0/O2 and
pristine/instrumented C. Each compares 1,385 formatting results with native
libc, totaling 22,160 value comparisons. The checks include signed and unsigned
integer widths, floating point and long double, characters, strings, pointers,
positional arguments, literal percent characters, dynamic width/precision,
buffer reuse and allocation growth.

The fixture includes C23 `wN` and `wfN` integer-width formats at 8, 16, 32 and
64 bits. The native implementation and corresponding compiler checks are
documented by the glibc implementation change.
[glibc C23 integer-width format support](https://sourceware.org/pipermail/libc-alpha/2023-June/148940.html)

The exact current `longdouble_ops.c` and `extended_math.h` supply the runtime
long-double operations. Other needed runtime exports come from a pinned,
read-only Linux archive. The harness uses a C startup and calls the actual six
emitted library units. The source packet pins 26 inputs. Native builds run
serially at reduced priority and reuse one temporary directory. The final
worker's peak owned files total 861,861 bytes.

## The 99-warning cluster

All 99 `vasnprintf` observations use the runtime-built directive buffer as the
native `snprintf` format. The formatter parses the incoming format, builds the
individual directive and dispatches its typed argument. These operations
appear in the translated library source and its emitted C.

Independent native C controls produce one nonliteral-format warning for a
runtime-built directive, zero warnings for the correct literal equivalent and
one argument-type warning for an incorrect literal. Both native tools retain
those results under the original unsuppressed flags. GCC documents this warning
as a format whose arguments it cannot check from a literal.
[GCC format diagnostics](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#index-Wformat-nonliteral)

Observational counters in a separate emitted-C copy reach 90 of the 99 call
sites, with identical successful value results across every profile. The nine
unexecuted sites remain recorded as coverage gaps. They comprise the two-field
character, wide-character and pointer variants, plus six string/wide-string
dispatch sites. The counters preserve the original call statements and add one
counter increment before each call. Pristine C also passes every value check.

The GCC warning total stays **208** across all 498 coreutils profiles. The
complete Clang total stays **1,852**. All 495 other profiles retain their
qualified C results with verified current opened-input hashes. The three
changed modules have fresh emissions and native before/after comparisons.
All 153 nonliteral-format warnings remain counted. The other 54 sites still
need their own source-contract review. The library ownership correction and
runtime evidence add no warning suppression.

## Reproduction and receipts

The local coreutils checkout is under
`build/c-emitter-forward-views-20261105/github-projects/fbxl_gnu_coreutils`,
pinned to `c1efae128e8c20c3b5b85d8faf41fdc42b3e4add` plus the recorded local
binding and ownership corrections. Receipts under the enclosing audit directory
retain exact bytes, hashes, commands, diagnostics and observed executions.

| Evidence | File |
| --- | --- |
| Original ownership source | `dynamic-format-ownership-before-v1-20261114.json.zlib` |
| Original and current six-unit emissions | `dynamic-format-packet-v{1,2}-20261114.json.zlib` |
| Original duplicate-definition failure | `dynamic-format-native-failure-v1-20261114.json.zlib` |
| Ownership tests and source/lint comparison | `dynamic-format-source-check-v1-20261114.json.zlib` |
| Final sixteen native executions | `dynamic-format-native-result-v3-20261114.json.zlib` |
| Native symbols and independent diagnostic controls | `dynamic-format-symbols-result-v1-20261114.json.zlib` |
| Source correction and permanent regression | `dynamic-format-ownership-v1-20261114.patch` |
| Portable dependency source snapshot | `dynamic-format-source-cut-v1-20261114.json.zlib` |
| Complete graph reconciliation and final checks | `dynamic-format-final-v1-20261114.json.zlib` |

The source snapshot preserves the exact six-unit dependency inputs, runtime
provider, build options and regression. The native worker and value fixture
are retained beside the packet. Another host needs matching native toolchains
and the pinned runtime archive, with path substitutions for its workspace.
The snapshot contains 29 files in 92,575 compressed bytes. The six-file patch
passes an isolated dry run and reproduces the current changed files exactly.
The broader compiler/corpus cleanup remains active.

<!-- end of c-emitter-coreutils-formatter.md -->
