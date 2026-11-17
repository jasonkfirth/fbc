<!--
Project: FreeBASIC compiler verification
File: c-emitter-time-formats.md
Purpose: Record the four production time-format call contracts.
Responsibilities: Bind current C identities, native executions and diagnostic limits.
Timezone-provider implementation retains its separate qualification.
-->

# Coreutils time-format contracts

The current `fprintftime`, `nstrftime`, `pinky` and `who` modules account for four
GCC nonliteral-format observations. Their complete C remains identical under
both C producers and development main `ba3b0a98`. The public regression appends
entry wrappers to those complete modules and calls their existing private
helpers. Entry-symbol renaming preserves every executable section's bytes.

The two `underlying_strftime` helpers construct a five-byte local format from a
leading space, percent sign, optional modifier and conversion. Their source
parser selects a zero, E or O modifier. The observer checks 56 conversion and
modifier choices, capacities from zero through 65, two locale configurations,
two timezone settings and six callback paths. Native libc supplies the separate
formatting reference. Callback controls check the opaque timezone token,
set/format/revert order and both callback failure results.

The `pinky` and `who` time slots each have two assignments, selecting
`%Y-%m-%d %H:%M` or `%b %e %H:%M`. Their private helpers use 33-byte buffers.
The regression covers both formats, twenty timestamps including signed 64-bit
extremes, timezone transitions and forced localtime failures. It links the
actual emitted `imaxtostr` module on the fallback path and compares the decimal
result with native `snprintf`. It also checks that the helper returns the
integer provider's actual result pointer.

## Native evidence

Sixteen Linux LP64 executions cover four production modules, GCC and Clang,
and O0/O2. They pass 5,679,616 cases and 61,532,496 checks. All four original GCC
nonliteral observations remain present in the production compilations.
The O0 compilations also expose one `IDLE_STRING` format-overflow observation in
each of `pinky` and `who`. Their hour/minute argument ranges retain separate
review. These two additional O0 observations are recorded alongside the
preceding O2 production census.
The largest individual scratch tree uses 1,178,034 bytes and the outer worker's
recorded retained input/result files peak at 1,116,140 bytes. Every scratch
directory is removed after its terminal result.

Sixty-four diagnostic compiles include independent native capability probes
and literal substitutions at the actual production calls. GCC rejects invalid
calendar conversions. Both tools reject a known printf argument-type mismatch.
Clang 19 accepts the invalid calendar conversion even with an explicit
strftime format attribute. Its conversion parser handles printf/scanf grammar
at this stage, and its nonliteral path explicitly permits strftime's fixed
time argument. The test records that measured capability and keeps the printf
failure control mandatory. [LLVM 19 format checking](https://github.com/llvm/llvm-project/blob/llvmorg-19.1.7/clang/lib/Sema/SemaChecking.cpp#L5675-L5678),
[LLVM 19 literal-format parsing](https://github.com/llvm/llvm-project/blob/llvmorg-19.1.7/clang/lib/Sema/SemaChecking.cpp#L7284-L7303)

The initial matrix stopped because its diagnostic gate assumed Clang would
reject the invalid calendar conversion. Its failed receipt remains retained.
The completed matrix uses an independent annotated calendar probe and an
active printf diagnostic control under the original unsuppressed options.

Source-only strict lint checks the four reviewed generated BASIC modules and
reports zero errors with 31 existing generated-line length advisories. Their
source text remains unchanged.

The four execution contracts now have reviewed dispositions. Eight dynamic
contracts remain, comprising two forwarded-format sites, four configured-format
sites and two numfmt floating-format sites. The two finite-format compiler
sites retain their follow-up. All 129 Coreutils nonliteral observations and the
disjoint last-qualified GCC total of 193 remain counted.

## Reproduction

Emit the complete Linux LP64 modules with the compiler and project include
paths being qualified. Supply the native diagnostic options as a JSON array.
The public runner uses the adjacent stat and formatter test support functions.

```text
python3 -B build_scripts/test-coreutils-time-formats.py --source-c nstrftime.c --kind underlying --cc gcc --optimization 2 --native-flags-json flags.json --receipt nstrftime-result.json
python3 -B build_scripts/test-coreutils-time-formats.py --source-c who.c --kind string --imaxtostr-c imaxtostr.c --cc clang --optimization 0 --native-flags-json flags.json --receipt who-result.json
```

Receipts are under `build/c-emitter-forward-views-20261105`.

- `time-format-packet-v2-20261116.json.zlib`
- `time-format-native-v2-20261116.json.zlib`
- `time-format-source-lint-v1-20261116.log`
- `current-format-ledger-v2-20261116.json.zlib`

<!-- end of c-emitter-time-formats.md -->
