<!--
Project: FreeBASIC C-backend verification
File: c-emitter-enum-widths.md
Purpose: Record the known-enum argument-width repair and format-source review.
Responsibilities: Bind source changes, native type controls and remaining work.
Broader application and corpus coverage retain their recorded evidence.
-->

# C2FB enum argument widths and remaining formats

The C2FB expression reader now emits known C enum values with an explicit
`Cast(c2fb_c_int, value)`. Its semantic model already assigned those values
the C int type. The previous emitted bare BASIC number became a 64-bit integer
on 64-bit targets. A variadic call consequently received a different type from
the original C expression.

The coreutils copyright call had the same defect. Its format owner contains
`Copyright %s %d Free Software Foundation, Inc.`, while the year argument was
emitted as `2026ll`. The corrected library source uses
`Cast(c2fb_c_int, c2fb_COPYRIGHT_YEAR)`, retaining the existing typed header
constant. Its current emitted C differs at exactly that argument, which is
now `2026`.

This enum-width qualification used compiler `9c1ebb28`. The subsequent
[private-format repair](c-emitter-local-formats.md) selects `f64ca95a` and
reduces coreutils to GCC 195 / Clang 1,839. The new qualified converter is
`build/c-emitter-forward-views-20261105/c2fb-enum-width-v1-20261114.exe`,
SHA256 `8176ad63bd52da95a582dda7dfae636b5d1c23946ca0f14ed9ca8310c315d3cf`.
Both producers emit identical complete converter C. It compiles with GCC 0
and Clang 158 observations, comprising 156 padding and two startup registrations.
Both resulting native converter builds pass the registered enum-width test
and the existing fifteen-function pointer-qualifier body regression.

## Native argument and value controls

The permanent Coreutils regression covers zero, 2026, -7, 2147483647 and
-2147483647 through the normal raw-source translator CLI. It checks explicit
types in the generated BASIC and compiles the actual C under native format
diagnostics. The original converter fails its type-preservation assertion.

Thirty-two repaired executions cover Windows/Linux, 32/64-bit targets, both
C producers, GCC/Clang and O0/O2. Native checks inspect the five actual emitted
argument expressions in each profile and retain all 160 C-int assertions.
Sixteen native original-C reference executions produce the same formatted
values. Sixteen old 32-bit executions also pass. Sixteen old 64-bit controls
fail their argument-type checks. Every repaired enum module compiles with
zero GCC and Clang warnings under the original aggressive options.

The actual Linux production version unit and its separately emitted format
owner pass eight copyright-output executions across both native compilers,
O0/O2 and before/after source. The external format warning remains counted.
An additional diagnostic control substitutes the proven provider text at that
format argument. The original 64-bit year fails native format checking and
the corrected year passes in all four profiles.

Three of four production object pairs are byte-identical. Clang O2 reorders
three independent register moves immediately before the copyright `fprintf`.
Its disassembly and symbols retain the complete comparison. The call arguments
and all other instructions match.

Strict source-only lint of the two changed BASIC modules retains four existing
advisories and adds zero findings. The converter's FBC source-warning classes
retain their existing counts.

The subsequent [cleanup and runtime qualification](c-emitter-cleanup-dependencies.md)
selects `409dc1ef`, completes Templari's 658-profile paired census and adds
261,120 native floating-format comparisons. Thirteen compiler propagation
candidates and 24 runtime contracts remain after these subsequent repairs.
The source-contract table below retains the original review cut.

## The other 43 format contracts

Source ownership is now traced for all 43 nonliteral-format sites beyond the
previously reviewed 99-site runtime formatter. Each record retains the actual
C statement, native warning line, translated function, source hash and review.

| Source contract | Sites | Remaining qualification |
| --- | ---: | --- |
| Finite literals through conditionals or temporaries | 7 | Compiler value propagation and native controls |
| Branch-selected gettext-family results | 8 | Stored-result propagation with branch-sensitive contracts |
| Runtime floating-format builders | 5 | Typed values and complete builder branches |
| Runtime time formats | 4 | Constructed and selected strftime directives |
| Stat directive builders | 7 | All typed output-helper branches |
| Configured command formats | 4 | Suffix, line-number and sequence validation paths |
| Stored byte-array format | 5 | Word-count output paths and selected suffixes |
| Caller-supplied formats | 2 | Complete caller ownership and output values |
| Separately owned copyright format | 1 | Argument-width repair qualified above |

The fifteen finite-format sites are the next compiler propagation candidates.
Their contracts need reaching-definition and branch checks. Several branches
intentionally consume different numbers of arguments. Twenty-seven additional
runtime contracts require native execution coverage. Original C byte-array
controls reproduce the word-count nonliteral warning with both const and
mutable arrays, through string and numeric initializers.

The last complete Linux production graph retains coreutils GCC 197 / Clang
1,841 and SQLite GCC 23 / Clang 7,696. One corrected version unit is freshly
emitted and compiled, with unchanged diagnostic counts. All other opened
production inputs retain verified hashes. Compiler and fbctests remain at
their last qualified GCC 0 totals. Templari retains its separately qualified
two GCC observations and pending full Clang census. The wider cleanup remains
active.

## Receipts and reproduction

Evidence lives under `build/c-emitter-forward-views-20261105`.

| Evidence | File |
| --- | --- |
| Original enum type fault | `enum-width-pilot-v1-20261114.json.zlib` |
| Aggressive converter builds and permanent regressions | `enum-width-build-v1-20261114.json.zlib` |
| Normal-CLI emissions and actual production difference | `enum-width-packet-v1-20261114.json.zlib` |
| Windows native controls | `enum-width-windows-v1-20261114.json.zlib` |
| Linux native and production controls | `enum-width-linux-v1-20261114.json.zlib` |
| Clang O2 instruction comparison | `enum-width-object-review-v1-20261114.json.zlib` |
| Strict source-only lint | `enum-width-lint-v1-20261114.json.zlib` |
| All 43 source-contract owners | `gcc-format-source-contracts-v1-20261114.json.zlib` |
| Reviewed format groups and pending checks | `gcc-format-classification-v1-20261114.json.zlib` |
| Original byte-array format controls | `format-array-controls-v1-20261114.json.zlib` |
| Portable source overlay | `enum-width-source-cut-v1-20261114.json.zlib` |
| Final qualification | `enum-width-final-v1-20261114.json.zlib` |

The overlay records both Git bases and its preceding source delivery. Apply
the parent deliveries, then its FBC and CORE namespaces. Build and execution
receipts preserve commands and diagnostic options. Jobs use one automatically
cleaned scratch directory and serial native builds. The Linux work reuses the
existing compile-audit directory. Full regeneration from the original coreutils
C tree retains its existing source-tree dependency.

<!-- end of c-emitter-enum-widths.md -->
