<!--
Project FreeBASIC C-emitter qualification
File c-emitter-change-formats.md
Purpose Record the final two Coreutils message-format contract reviews.
Responsibilities Bind original C, emitted modules, native arguments and callers.
Application traversal and converter regeneration have separate qualification.
-->

# Coreutils change-reporting formats, November 17

The `describe_change` functions in `chmod` and `chown-core` select translated
messages through local format pointers. Their final variadic arguments satisfy
all alternatives under their private caller contracts. Both original C
functions also produce nonliteral-format diagnostics. These two source reviews
close the remaining format contracts, bringing the reviewed ledger to all
129 observations. The production census records GCC 172, including all 129
unsuppressed nonliteral observations and Coreutils' total of 156.

## Source and argument contracts

The reference functions come from the upstream Coreutils v9.12 tag. Captured
source bytes, URLs and SHA256 values are retained in the packet. Their five
`chmod` and thirteen `chown-core` message occurrences match the current BASIC
bodies. Each current reporter has one private caller. Declarations, definitions
and direct calls account for all code uses in both BASIC and emitted C.
[chmod source](https://github.com/coreutils/coreutils/blob/v9.12/src/chmod.c),
[chown source](https://github.com/coreutils/coreutils/blob/v9.12/src/chown-core.c).

The final `chmod` call passes a quoted character pointer, two `unsigned long`
mode values and two local permission-string spans. Both mode values have the
07777 mask. The retained-message path prints its shorter argument list and
returns before the final call. Missing-file and symlink paths return earlier.
An invalid status reaches the nonreturning assertion.

The final `chown-core` call passes three character pointers. Messages consume
one, two or three of those arguments. C permits unused trailing arguments.
Failed changes with absent old names transfer the new specification into the
old-specification slot and clear the original slot before cleanup. Successful
and retained calls obtain their old names from the traversal stat record,
with numeric duplication as the name-lookup fallback. The source review binds
these caller assumptions to the current module hashes.

The compiler's saved-format metadata expires at branch joins and ordinary
calls. The quoting call after `chown-core`'s format selection is one such
boundary. The current diagnostics therefore describe a runtime-selected
translated format. Native type diagnostics remain enabled.

## Complete native functions and diagnostic controls

Both current FreeBASIC C producers emit identical complete owning modules.
The observer links those complete modules and their actual private helpers.
An appended wrapper exposes each reporter. Renaming `chmod`'s application
entry symbol preserves every executable object-section hash. Its inline
assembly keeps the source recipe's `-mno-red-zone` flag.

GCC and Clang execute the comparison at O0 and O2 on Linux LP64. The four
runs pass 49,008 cases and 3,738,000 checks. Inputs cover all valid status
values, invalid signed limits, mode masks, empty and special-character names,
absent old/new owner and group combinations, early returns and allocation
retirement. Successful and retained name combinations satisfy the caller
contract described above. Invalid enum cases use an intercepted nonreturning
assertion, after which the observer releases its own tracked storage.

Each run compares selected format text, output bytes, quoting and translation
calls, permission formatting, allocations, frees and assertion outcomes with
the independently compiled original C functions. Translation modes include
ordinary text, prefixed translations and positional argument conversions.
Quoting and translation use independent provider buffers.

Sixty-four diagnostic compiles replace only the format operand at the original
final native call. All 56 correct-format controls pass the type check. All
eight deliberately wrong-type controls produce the expected type diagnostic.
Shorter message variants retain their extra-argument diagnostics in these
isolated controls. Each production module reports one GCC nonliteral warning.
The original C references report two in `chmod` and one in `chown-core`.

The observer is GCC-clean. Each Clang observer build records four reserved
identifier observations for linker-wrap names, four declaration-placement
observations, its dynamic `vsnprintf` format observation and 39 bounded-buffer
observations. Native production and reference diagnostic maps are also retained.
Source-only strict lint of the two BASIC modules reports zero errors and 30
existing generated-line advisories.

The audit initially decoded BASIC's numeric string escapes using C's octal
rule. The format-set check exposed this before native execution, and the
reader now uses BASIC's decimal escape values. A redundant null comparison
against gettext's native nonnull message argument was removed from the
observer. The final four-run matrix uses the corrected reader and observer.

## Reproduction and remaining work

`build_scripts/test-coreutils-change-formats.py` accepts complete `--chmod-c`
and `--chown-c` emissions, the packet's `--chmod-reference` and
`--chown-reference` C files, `--cc`, `--native-flags-json`, `--optimization`
and `--receipt`. Its observer is
`tests/compiler-support/coreutils-change-formats.c`. An optional
`--scratch-root` chooses temporary storage. Each invocation removes its scratch
directory. The largest inner set uses 269,885 bytes and the outer worker peaks
at 300,670 bytes.

The following receipts reside under
`build/c-emitter-forward-views-20261105`.

- `change-format-packet-v2-20261117.json.zlib`
- `change-format-native-v2-20261117.json.zlib`
- `change-format-contracts-v1-20261117.json.zlib`
- `change-format-lint-v1-20261117.json.zlib`
- `current-format-ledger-v7-20261117.json.zlib`
- `change-format-qualification-v1-20261117.json.zlib`

Grouped-default production regeneration and full C2FB integration retain
their structural checks. Other GCC source owners, native platform checks,
Clang classes, port publication and whole-application executions retain their
existing scope.

<!-- end of c-emitter-change-formats.md -->
