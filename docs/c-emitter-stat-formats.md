<!--
Project: FreeBASIC C backend verification
File: c-emitter-stat-formats.md
Purpose: Record native qualification of the coreutils stat format builders.
Responsibilities: Describe tested inputs, native diagnostics and reproducible checks.
The ordinary coreutils application has its own execution qualification.
-->

# Coreutils stat format contracts, November 15

Seven pending dynamic-format contracts now have Linux x86-64 native checks.
They cover `OUT_STRING`, `OUT_INT`, `OUT_UINT`, `OUT_UINT_O`, `OUT_UINT_X`,
`OUT_MINUS_ZERO` and `OUT_FILE_CONTEXT` in the generated FreeBASIC stat module.
The production source and compiler retain their preceding implementations.

The selected compiler `dffcd9a1` emits identical 287,326-byte C modules through
its GCC and Clang producers. Their SHA256 is
`73f95346f7d21d0825dd508db41f7df3c045669fc1f15a8dff57f4e8391baa60`.
GCC 14.2 and Clang 19.1.7 each compile and execute the production helper bodies
at O0 and O2 with the full unsuppressed audit diagnostics. These four executions
pass 330,780 comparisons of formatted bytes, conversion buffers, return values
and callback ownership.

## Native coverage

The observer exercises all 128 combinations of the production flag alphabet,
five widths, four precisions, signed and unsigned limits, empty strings, percent
characters and UTF-8 bytes. It compares complete bounded output and format
storage against a separate libc reference. Forced output failures cover every
helper. File-context callbacks cover follow-link selection, returned context
ownership and lookup failure with deterministic callback results.

The epoch caller removes precision before invoking `OUT_MINUS_ZERO`. Its matrix
follows that contract. File-context string prefixes cover space padding, left
alignment and precision. Caller-owned storage includes conversion and NUL space.
The native profile requires 64-bit `long` and pointers and uses the C locale.
Other data models, locales and complete application execution retain their
separate qualification.

An appended C wrapper exposes the original static helpers to the observer.
Object-symbol renaming gives the observer its own entry point. Executable ELF
section hashes remain identical across that rename. The production function
bodies and variadic arguments retain their emitted bytes.

Thirty-two additional diagnostic compiles replace only the seven format
operands with literals. Four correct-type controls compile successfully.
Twenty-eight deliberately incompatible controls fail with native format-type
errors. These diagnostic variants establish actual argument types for each
conversion. Runtime checks execute the original dynamic formats.

## Warning accounting

The original stat module retains seven `-Wformat-nonliteral` observations under
each native tool. Probe and baseline warning-class maps match in every context.
The observer's four reference-format observations remain recorded separately.
Coreutils stays at GCC 168 / Clang 1,803 and the disjoint GCC total stays 280.
Seven pending runtime contracts are closed, leaving seventeen. Four finite-format
compiler propagation sites remain open.

## Reproduction and receipts

Emit the generated stat module for Linux x86-64 with the selected compiler and
its normal project includes. Run the repository entry point with the resulting
C file and an ELF64 GNU-compatible native toolchain.

```sh
python3 -B build_scripts/test-coreutils-stat-formats.py \
  --source-c /path/to/stat.c --cc gcc --optimization 2 \
  --receipt /path/to/new-stat-receipt.json
```

Repeat with Clang and optimization levels 0 and 2. `--native-flags-json` accepts
the complete original audit flag array. The runner rejects suppression flags,
checks input hashes after execution and removes its temporary products.

Receipts are under `build/c-emitter-forward-views-20261105`.

- `stat-format-packet-v3-20261115.json.zlib` pins the exact C, tools, flags and
  normal repository test sources.
- `stat-format-native-matrix-v1-20261115.json.zlib` records four native runs,
  32 diagnostic compiles and matching production warning maps. Its serial guest
  scratch peaks at 500,331 bytes and is removed.
- `stat-format-qualification-v1-20261115.json.zlib` binds the completed contracts
  to their source and protected-product checks and retains earlier queue cuts.

The read-only source lint reports zero errors and nineteen existing observations
in the generated BASIC module. Its source and emission diagnostics are preserved.

The final preservation scan checks 12,974 earlier source pins and ten protected
products. Ten concurrently edited Templari inputs differ, including its build
recipe and media/device GUI sources. Templari's 674-profile result remains its
last qualified source cut. These newer inputs need a subsequent project replay.
The stat inputs, other source pins and protected products match their receipts.

<!-- end of c-emitter-stat-formats.md -->
