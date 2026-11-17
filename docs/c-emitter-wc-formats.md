<!--
Project: FreeBASIC C-backend qualification
File: c-emitter-wc-formats.md
Purpose: Record word-count output contracts and the current runtime refresh.
Responsibilities: Bind native values, diagnostics, source versions and limits.
Complete utility execution retains its application-specific qualification.
-->

# Word-count formats and runtime qualification, November 16

Five pending dynamic-format contracts in Coreutils' `write_counts` helper now
have native execution coverage. They cover lines, words, characters, bytes and
the longest line. The remaining dynamic-format contract count falls from
seventeen to twelve. Production warning totals stay at Coreutils GCC 166 /
Clang 1,785 and SQLite GCC 21 / Clang 7,696. The disjoint GCC total is 203.

## Production output contracts

Development compiler `0be13d79` freshly emits the complete word-count module
through both C producers. Both reproduce the qualified 248,838-byte C input
with SHA256 `c014940c20f5015c3cbb605f6f885bae9978e5a3e230af2ada5e137f77cbff5f`.
An appended entry point calls the actual production helper with its own global
selection flags and field width. ELF executable-section hashes survive the
application entry-symbol rename used to link the observer.

GCC 14.2 and Clang 19.1.7 each compile and execute that input at O0 and O2 under
the full unsuppressed audit options. The four executions pass 37,888 cases and
1,979,532 checks. Coverage includes all 32 field selections, six widths, seven
integer boundary sets, seven filename forms and forced output failures.
The matrix covers unsigned maximum, signed minimum, counts above 32 bits,
negative width, UTF-8 bytes, percent characters and newline quoting.

The integer conversion observer returns an interior pointer in the caller's
21-byte buffer, matching the GNU inttostr interface. An independent libc
reference prints the actual integer values directly. Complete output bytes,
column ordering, conversion counts and quoting callbacks are compared.
Output-error callbacks check the requested exit status, saved errno, stream
identity, flushing, purging and error-state clearing. Cleanup callbacks change
errno independently, exercising preservation of the original write error.
The error observer returns
after recording the request so every failure case can run in one process.
Complete utility exit and file-reading behavior keep their application tests.

Forty-four diagnostic compiles retain each actual production argument
expression and replace only format operands. Four correct-type controls pass.
Forty deliberately incompatible width or value controls produce native
format-type errors. All five production `-Wformat-nonliteral` observations stay
counted under each native tool. Probe and baseline warning maps agree.
The static byte-array format and its one-byte suffix retain their native
warning disposition. The observer has zero GCC observations and 22 Clang
observations, recorded separately from production totals.

The native source profile is Linux x86-64 LP64 with the C locale. The source-only
linter reports zero errors and fourteen existing long-line advisories in the
generated BASIC module. Source and producer identities are checked again after
execution. The serial worker peaks at 470,442 scratch bytes and removes its
temporary tree.

## Current runtime source versions

The two subsequent runtime edits in `console/io_printusg.c` and
`drivers/unix/io_readstr.c` are qualified through packet v43. Complete fresh
MT and non-MT builds remain GCC-clean. Clang totals are 10,023 for MT and 10,021
for non-MT. All 332 current MT execution contexts and twenty non-MT public
executions pass against the rebuilt code.

Eight supplemental executions run the existing narrow and wide PRINT USING
suites and the final-field cleanup suite through the real fbcunit entry point.
All 17,320 assertions pass. Fifty-six fresh compiler emissions establish 28
paired producer identities across both optimization levels and thread modes.
The supplemental worker rebuilds the four normal runtime configurations and
matches every one of their 2,072 object hashes, all archive hashes and startup
hashes to the preceding complete runtime receipts.

The supplemental harness initially expected raw namespace names in the XML
report. The report normalizes trailing namespace underscores. Its corrected
check requires all three suites and validates every suite's result.

A subsequent Clang O2 run exposed the harness's omission of the C driver's
`-frounding-math` option. Eight independent native controls reproduce positive
NaNs under relaxed Clang O2 and the expected negative indeterminate values
under the normal driver options. The corrected harness uses both
`-frounding-math` and `-fno-math-errno`, bound to the qualified compiler-driver
source. Every native tool, optimization level and thread configuration then
passes the complete PRINT USING suites.

The fresh MT and non-MT workers peak at 23,861,267 and 16,895,518 bytes.
The PRINT USING worker peaks at 12,268,116 bytes. Native work is serial and
low-priority within each worker. Temporary build trees are removed.

## Reproduction and receipts

The repository entry point accepts the normal emitted word-count C module and
a GNU-compatible ELF64 toolchain.

```sh
python3 -B build_scripts/test-coreutils-wc-formats.py \
  --source-c /path/to/wc.c --cc gcc --optimization 2 \
  --native-flags-json /path/to/audit-flags.json \
  --receipt /path/to/new-wc-receipt.json
```

Repeat with Clang and native O0. Receipts under
`build/c-emitter-forward-views-20261105` include
`wc-format-packet-v3-20261116.json.zlib`,
`wc-format-native-matrix-v3-20261116.json.zlib`,
`runtime-current-packet-v43-20261116.json.zlib`,
`runtime-current-linux-v36-20261116.json.zlib`,
`native-nonmt-controls-v3-20261116.json.zlib` and
`print-using-native-v3-20261116.json.zlib`.

Three finite-format compiler sites, twelve dynamic-format contracts, other GCC
source faults and the remaining Clang/platform owners stay open. The overall
cleanup goal remains active.

<!-- end of c-emitter-wc-formats.md -->
