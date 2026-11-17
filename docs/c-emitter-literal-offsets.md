<!--
Project: FreeBASIC C-backend verification
File: c-emitter-literal-offsets.md
Purpose: Record bounded literal addresses and corrected C ABS result widths.
Responsibilities: Bind the current compiler, production diagnostics and values.
Broader corpus coverage and application deployment have separate evidence.
-->

# Literal suffixes and native format checks

The subsequent [plural-format qualification](c-emitter-plural-formats.md)
selects `9c1ebb28` and brings coreutils to GCC 197 / Clang 1,841. This report
retains the preceding literal-offset cut and its exact receipts.

The selected development compiler is
`cd0cc1b6c8c1e4d3f3a1505a4512f3ea0ed1c5ac4f0bf91a19761798683dc8e4`.
`build/main-gcc/bin/fbc.exe` selects that image. Immediate rollback is the
qualified `cd32db5b` image. Thirty executions pass through the public development
path after selection.

| Current qualified graph | Profiles | GCC warnings | Clang warnings |
| --- | ---: | ---: | ---: |
| Compiler | 224 | 0 | 1,339 |
| fbctests | 703 | 0 | 771 |
| Coreutils Linux x86-64 | 498 | 201 | 1,845 |
| SQLite Linux x86-64 | 555 | 23 | 7,696 |

Coreutils loses seven warnings from each native tool. All 1,053 port contexts
and all 703 retained fbctests contexts are emitted again through the selected
image and match their fully qualified C exactly. Every FBC warning line is
preserved. The current compiler reaches an exact 224-unit C fixed point. Its
Clang observations comprise 1,326 padding and thirteen startup-registration
observations. The full GCC/Clang backend suite passes in 382.197 seconds on the
final image.

## Compiler lowering

The C backend normally forms BASIC pointer offsets through native unsigned
addresses. This preserves null, virtual and wrapping address arithmetic.
That representation also obscured seven literal formats in the coreutils date
parser and temporary-directory code.

The new `c-literal-offsets.bi` proof recognizes a zero-or-one byte offset into
a nonempty, read-only narrow literal. It follows integer comparisons, signed
negations and casts that preserve the value set. An unsigned cast of a negative
value fails that proof. The selected C expression chooses between addresses
of array elements zero and one in the original literal storage. The condition
appears once. Both addresses stay inside that array, including its terminator.
Other address expressions retain their established lowering.

A native pilot shows that typed pointer addition still loses format checking.
An explicit bounded address choice restores it. Constant array indices also
retain Clang's buffer diagnostics without introducing an unchecked pointer
step. The existing literal-symbol lookup now belongs to literal storage and
serves both the suffix proof and evaluated translation-result handling.

The permanent `c-literal-offsets.bas`, `.c` and `.py` regression is registered
in `build_scripts/test-compiler-backends.py`. The native diagnostic controls
accept the proven forms and retain errors for wrong argument types, arbitrary
offsets, unsigned conversions, empty literals and negative offsets.

## A genuine library width fault

Restored format checking exposed two argument-width faults in `tm_year_str`.
Each appeared in both literal branches, producing four diagnostics per native
tool. C2FB mapped C `abs` to BASIC `Abs`, whose narrow integer argument is
promoted to native `Integer`. On 64-bit targets that result reached `%d` with
the wrong C argument type.

C2FB now casts the intrinsic result to its declared C return type. The expression
metadata also identifies the wrapped value correctly when a caller discards
it. The checked-in date-parser translation receives the matching two casts.
BASIC's `Abs` behavior retains its language contract. The formatter passes the
native C `int` values required by its format.

The existing runtime-ABS regression now checks the return value, a variadic
call and a discarded call containing a post-increment. It uses explicit C
declarations through the raw-source entry point. The original converter fails
the result-width control and the repaired converter passes the native-default
and explicit GCC builds. The Windows qualification supplies native DLL export
flags for the ctypes observers. Full regeneration from original Coreutils C
remains pending because that source tree is absent from the sparse checkout.

## Executed values and source qualification

Sixty-four native C-startup executions cover Win32/Win64, both C producers,
both native tools and O0/O2. Thirty-two exercise literal selection, condition
call counts and original-storage address identity. Another thirty-two compare
the extracted original/repaired date formatter at seventeen year values,
including both C-int limits and the 1900 boundary. Both pointer widths retain
their native ABI. All repaired generated probe modules have zero GCC and
Clang warnings under the saved aggressive flags.

The production replay has three changed C units. GCC 14.2 and Clang 19.1.7
compile every original/repaired pair under the recorded unsuppressed options.
A second date-parser comparison qualifies the result-width correction and
removes its four newly exposed type observations. The final image reproduces
all 1,756 qualified test/port C files exactly. Compiler-native checks use GCC
16.2 and Clang 22.1.7, with GCC 16.1 supplying the Win32 link environment.

A concurrent GAS64 scalar-copy edit appeared during qualification. The final
candidate incorporates that current source and repeats the compiler fixed
point and full backend suite. Its imported store/reload test runs through the
real fbcunit framework at O0/O2. Both cases and all eight assertions pass per
run. The preceding compiler's corresponding runs are also retained.

Strict source-only lint reports zero findings in the C-backend implementation
and literal fixture. The generated date-parser statement retains one new
line-length advisory at 407 characters, preserving the translator's statement
formatting. Its 25 other advisories predate this correction.

## Reproduction and receipts

Receipts live under `build/c-emitter-forward-views-20261105`.

| Evidence | File |
| --- | --- |
| Current exact compiler fixed point | `literal-offset-selfhost-v3-20261114.json.zlib` |
| Complete compiler Clang graph | `literal-offset-compiler-clang-v2-20261114.json.zlib` |
| Full normal backend suite | `literal-offset-suite-v2-20261114.json.zlib` |
| Complete port diagnostics and C | `literal-offset-port-replay-v1-20261114.sqlite3` |
| Three native production comparisons | `literal-offset-port-native-result-v1-20261114.json.zlib` |
| Date-parser width comparison | `literal-offset-abs-port-result-v1-20261114.json.zlib` |
| Complete fbctests diagnostics | `literal-offset-fbctests-v1-20261114.sqlite3` |
| Final-image test/port C identities | `literal-offset-current-identity-v1-20261114.json.zlib` |
| Thirty-two literal executions | `literal-offset-native-values-v2-20261114.json.zlib` |
| Thirty-two date-format executions | `literal-offset-year-values-v2-20261114.json.zlib` |
| Existing translator regression | `literal-offset-abs-regression-v2-20261114.json.zlib` |
| Owning GAS64 fbcunit runs | `literal-offset-gas64-owner-v1-20261114.json.zlib` |
| Current incremental source overlay | `literal-offset-source-cut-v1-20261114.json.zlib` |
| Public development-path selection | `literal-offset-selected-v1-20261114.json.zlib` |
| Final verification | `literal-offset-final-v1-20261114.json.zlib` |

The 15-file overlay preserves 3,776,245 raw bytes in 1,007,577 compressed bytes.
It records the compiler and coreutils Git bases and the exact parent source
deliveries. Apply those recorded parent overlays before this increment. The
qualification controllers retain commands, tool hashes and runtime-archive
identities. Installed release binaries and runtime archives pass their
preservation hashes.

Coreutils retains 146 nonliteral-format warnings, including the separately
reviewed 99-site runtime builder. Forty-seven other nonliteral sites still need
their own contracts reviewed. The nine unexecuted builder sites, wider source
coverage, parser stack control and semantic-linter consumer remain recorded
in the linked status reports. The broader cleanup remains active.

<!-- end of c-emitter-literal-offsets.md -->
