<!--
Project FreeBASIC compiler verification
File c-emitter-sequence-formats.md
Purpose Record the configured sequence format and floating output contract.
Responsibilities Bind complete emitted helpers, actual parsing providers and diagnostics.
Application argument processing and the integer-only fast path retain their owners.
-->

# Coreutils sequence formats

The complete emitted `seq` module and its `xstrtold`, `cl-strtold`, `c-strtold`
and `streq` providers retain identical C under both producers and development
main `4a477330`. The public regression appends entry wrappers to the complete
sequence module. Entry-symbol renaming preserves executable section bytes in
all five modules. The native executable links those actual providers, current
`longdouble_ops.c` and libc.

## Format and output checks

`LONG_DOUBLE_FORMAT` validates a single floating conversion and normalizes its
length modifier to `L`. It also computes decoded prefix and suffix lengths,
counting each escaped percent pair once. The observer checks all eight floating
conversions, seven flag choices, four widths, four precisions, two prefixes and
both original modifier choices. These produce 3,584 valid format configurations
per native context. Eighteen invalid formats must terminate through the actual
parser's error path.

`PRINT_NUMBERS` passes a native `long double` at its variadic output calls.
Twelve intervals cover ascending and descending sequences, empty ranges,
quarter steps, signed zero, the largest finite long double, subnormal steps and
endpoint rounding. Each runs with two separators and both locale-control paths.
An independent native reference computes the expected values, formatted output
and rounded endpoint decision. The observer checks every actual printed value,
its sign and the complete output bytes. The actual translated parsers handle
the production endpoint probes.

The observer controls translation, quoting, allocation and terminating errors.
It supplies bounded formatting and stream observers through GNU linker wrapping
while using real libc formatting. The native locale calls run through a checked
set/restore observer. Parser-result and rounding-probe allocations are tracked
through their actual frees. Separate injected controls cover failed printing,
failed separator output, failed rounding-probe allocation and failed parser
allocation. The stdout purge provider checks the real write-error helper's
termination path against the observer's separate output buffer.

## Native evidence

Four Linux LP64 executions cover GCC 14 and Clang 19 at O0 and O2. They pass
358,488 cases and 15,004,612 checks, including 1,011,860 printf calls, 305,028
rounding probes and 88 expected terminating errors. Eight native diagnostic
compiles accept the correct long-double literal at the actual printf call and
reject the wrong pointer conversion. Original diagnostic options remain enabled.

GCC retains its one production nonliteral-format observation. Clang retains
three nonliteral observations, including the two `asprintf` probes. The four
provider modules remain GCC-clean. These observations stay in the last complete
production census, whose disjoint GCC total remains 193.

The largest individual scratch tree uses 418,425 bytes. The outer worker's
retained inputs and results peak at 282,773 bytes. All scratch directories are
removed after their terminal results. The first test attempt needed the actual
`streq` provider added to its link. The next exposed a 512-byte observer buffer
limit when fixed-point formatting the largest finite long double. The completed
observer derives its line capacity from `LDBL_MAX_10_EXP` and retains explicit
output bounds. The production module remains unchanged.

Source-only strict lint across the five reviewed BASIC modules reports zero
errors and thirteen existing generated-line length advisories. The reviewed
nonliteral count reaches 123 of 129. Four dynamic contracts remain, covering two
forwarded formats and two numfmt floating formats. Two finite-format compiler
sites retain their follow-up.

## Reproduction

Emit the five complete Linux LP64 modules with their normal include paths.
Supply the aggressive unsuppressed native options as a JSON array. The runner
uses the adjacent stat and formatter support and the current runtime sources.

```text
python3 -B build_scripts/test-coreutils-sequence-formats.py --source-c seq.c --provider-c xstrtold.c --provider-c cl-strtold.c --provider-c c-strtold.c --provider-c streq.c --rtlib-dir src/rtlib --cc gcc --optimization 2 --native-flags-json flags.json --receipt seq-result.json
```

Repeat for GCC and Clang at O0 and O2. Each invocation owns one temporary
directory and removes it automatically. The qualification packet binds the exact
compiler, complete C identities, public test files and native tools used for the
four completed contexts.

<!-- end of c-emitter-sequence-formats.md -->
