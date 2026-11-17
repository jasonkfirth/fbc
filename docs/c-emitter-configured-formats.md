<!--
Project FreeBASIC compiler verification
File c-emitter-configured-formats.md
Purpose Record configured line-number and split-filename format contracts.
Responsibilities Bind unchanged helper bodies, native checks and retained warnings.
Application startup, filesystem operations and allocation failure retain their owners.
-->

# Coreutils configured formats

The complete emitted `nl` and `csplit` modules retain identical C under both
producers and development main `4a477330`. The public regression appends entry
wrappers and calls the existing private helpers. Renaming the original main
symbol preserves every executable section's bytes.

## Line numbers

`PRINT_LINENO` selects one of three formats, `%-*jd%s`, `%*jd%s` and `%0*jd%s`.
The observer checks the actual format and the `int`, `intmax_t` and string
arguments, then compares libc output against separately typed literal calls.
Five widths, eleven starting values, eleven increments and five separators
cover 9,075 combinations per native context. Signed limits, wrapped next values
and the subsequent overflow error are checked. Negative and zero increments
extend the direct helper checks beyond the ordinary positive-increment caller.

## Split filenames

`MAX_OUT` validates one integer conversion, including its flags, width and
precision, and rewrites unsigned decimal to signed decimal. `MAKE_FILENAME`
then formats a nonnegative file index after the prefix. Its callers initialize
the file counter to zero, stop incrementing at `INT_MAX` and only format
nonnegative values during cleanup. Signed and unsigned integer argument views
therefore share the representable values used here.

An independent parser checks the production parser's decisions and rewritten
bytes. The matrix covers accepted and rejected flag/conversion combinations,
escaped percent signs, widths through 96, precisions through 12, four prefixes
and seven indices through `INT_MAX`. Default suffix widths from zero through
32 are also checked. Each context executes 92,642 cases. The observer measures
the original suffix with the real libc, verifies the exact production capacity
before the actual write and checks the surrounding guard bytes afterward.

The ASCII digit classifier is a controlled provider with its declared byte
return type. Translation and terminating error calls are controlled providers.
The allocation-failure cleanup graph retains two filesystem quoting references
at link time. Those providers terminate the test immediately if reached.
Allocation failure and filesystem behavior retain separate qualification.

## Native evidence

Eight Linux LP64 executions cover GCC 14 and Clang 19 at O0 and O2. They pass
406,868 cases and 162,853,076 checks, including 198,504 formatting calls and
244,664 expected terminating errors. Twenty diagnostic compiles include eight
correct-type passes and twelve expected wrong-type failures at the original
production call sites. All original native warning options remain enabled.

The production compilations retain all three nonliteral-format observations.
GCC also retains the existing default-suffix `%0*d` format-overflow observation
at both optimization levels. The bounded filename checks verify its actual
positive width, nonnegative index and allocated-capacity contract. The warning
remains in the ordinary GCC census.

The largest individual scratch tree uses 517,398 bytes. The outer worker's
retained inputs and results peak at 457,594 bytes. All scratch directories are
removed after their terminal results. The first attempt passed all four `nl`
contexts and stopped at the two unresolved filesystem quoting references in
`csplit`. The completed attempt supplies fail-fast providers for those inactive
paths and reruns all eight contexts.

Source-only strict lint reports zero errors and 21 existing generated-line
length advisories across the two unchanged BASIC modules. The reviewed count
reaches 122 of 129 nonliteral observations. Five dynamic contracts remain,
covering two forwarded formats, the configured sequence format and two numfmt
floating formats. Two finite-format compiler sites retain their follow-up.
The disjoint last-qualified GCC total remains 193.

## Reproduction

Emit each complete Linux LP64 module with its normal project include paths.
Supply the unsuppressed native options as a JSON array. The runner uses the
adjacent stat and variadic-formatter support and the current long-double runtime
for `nl`.

```text
python3 -B build_scripts/test-coreutils-configured-formats.py --source-c nl.c --program nl --rtlib-dir src/rtlib --cc gcc --optimization 2 --native-flags-json flags.json --receipt nl-result.json
python3 -B build_scripts/test-coreutils-configured-formats.py --source-c csplit.c --program csplit --rtlib-dir src/rtlib --cc clang --optimization 0 --native-flags-json flags.json --receipt csplit-result.json
```

Run both modules with each compiler and optimization. Each invocation owns one
temporary directory and removes it automatically. The private qualification
packet additionally binds the exact compiler, source, native tool and input
identities used for the eight completed runs.

<!-- end of c-emitter-configured-formats.md -->
