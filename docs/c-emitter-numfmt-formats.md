<!--
Project FreeBASIC compiler verification
File c-emitter-numfmt-formats.md
Purpose Record the two runtime-built numeric output contracts.
Responsibilities Bind native arguments, buffer capacities and actual runtime cleanup.
Application option parsing and localized grouping retain their own qualification.
-->

# Coreutils numeric formats

The complete emitted `numfmt` module retains identical C under both producers
and development main `4a477330`. The public regression appends an entry wrapper
and invokes the original `DOUBLE_TO_HUMAN` helper. Renaming the original main
symbol preserves every executable section's bytes.

## Native call contracts

The unscaled format carries an integer precision, native `long double` and one
suffix string. The scaled format carries the same numeric arguments and four
strings for the unit separator, scale suffix, IEC marker and caller suffix.
The observer checks both actual formats and every argument before real libc
formatting. An independent reference uses native rounding functions and scale
arithmetic to check the transformed number, precision, suffixes and output.

The matrix covers twenty signed values, signed zero, all four output scales,
five rounding modes, four input precisions and five requested precisions.
Widths through 96, optional grouping, four suffix choices and four separator
choices rotate through these combinations. Additional cases cover both signs
at every SI and IEC suffix power through Q. Six output capacities check empty
buffers, truncation, exact lengths and complete strings, with guard bytes around
the destination.

Each helper call allocates its original one-dimensional, 28-byte format array
through the actual current runtime. The observer validates its bounds and
format termination, checks zero-width-prefix writes before they occur and
checks retirement through the actual array erase routine. Both ordinary output
and injected negative `snprintf` results must release the format array.

Maximum `INT_MAX` width cases check the complete built format, native arguments,
return transport and array retirement with a controlled formatting result.
These cases avoid requesting billions of padding characters from libc.
All ordinary bounded-capacity cases use real libc formatting.

## Native evidence

Eight Linux LP64 executions cover GCC 14 and Clang 19, O0/O2 and both MT and
non-MT runtime archives. They pass 400,224 cases and 225,698,496 checks. The
runtime retires all 400,224 format arrays. The unscaled path receives 96,096
calls and the scaled path receives 304,128 calls.

Twenty-four diagnostic compiles check correct literals at both actual calls
and inject a wrong integer conversion for each string payload. Eight correct
controls have no type diagnostics at those calls. Sixteen wrong-type controls
retain their expected native warnings at the changed call locations. The
existing GNU apostrophe-extension warning retains its separate location and
stays enabled throughout these checks.

The production module retains two GCC nonliteral observations and one GCC
apostrophe-extension observation. Clang retains its two nonliteral observations.
The reviewed nonliteral count reaches 125 of 129, leaving two forwarded-format
contracts and two finite compiler branch-join sites. The disjoint last-qualified
GCC total remains 193.

The older borrowed test archive predates the current error-frame ABI. The
[runtime source replay](c-emitter-runtime-current-refresh.md) supplies four
freshly built and fully checked archives for this matrix. Their source, tool,
archive and startup identities are recorded before the format executions.
All eight temporary archive/startup files are removed after qualification,
releasing 6,141,552 bytes.
The largest individual format scratch tree uses 775,481 bytes and the outer
worker's retained files peak at 993,398 bytes. Source-only strict lint reports
zero errors and nineteen existing generated-line length advisories.

## Reproduction

Emit the complete Linux LP64 module with its normal include paths. Build the
runtime archive and startup object from the source tree being qualified.
Supply the aggressive unsuppressed native options as a JSON array.

```text
python3 -B build_scripts/test-coreutils-numfmt-formats.py --source-c numfmt.c --rtlib-dir src/rtlib --runtime-archive build/runtime/lib/freebasic/linux-x86_64/libfbmt.a --startup-object build/runtime/lib/freebasic/linux-x86_64/fbrt0.o --enable-mt --cc gcc --optimization 2 --native-flags-json flags.json --receipt numfmt-result.json
```

Repeat with both native tools, O0/O2 and each runtime mode. Omit `--enable-mt`
for the non-MT archive. Each invocation owns one automatically cleaned scratch
directory and records its complete commands, diagnostics and input hashes.

<!-- end of c-emitter-numfmt-formats.md -->
