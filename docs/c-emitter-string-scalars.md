<!--
Project: FreeBASIC C backend verification
File: c-emitter-string-scalars.md
Purpose: Record unsigned parser and wide-character conversion contracts.
Responsibilities: Explain source changes, native diagnostics and reproduction.
Full application qualification retains its project-specific evidence.
-->

# String scalar conversions, November 15

The complete Linux MT runtime and startup build reaches GCC 72 / Clang 10,078,
removing five observations from each native tool. All 1,036 native object
identities match the preceding runtime cut. The disjoint GCC total is 275.

Four runtime sources make five intended unsigned conversions explicit.
`VALUINT` returns an unsigned 32-bit result after the native byte/wide parser.
When native `unsigned long` is wider, the final conversion retains its low
32 bits. Native parser overflow retains the target C library's behavior.
Wide `ASC` exposes the raw code unit, including the bit pattern of signed
32-bit `wchar_t`. The Unicode decoder passes those unsigned values to the
existing scalar encoder, which replaces out-of-range values and isolated
surrogates with U+FFFD.

These conversions implement the existing public result contracts. Explicit
casts preserve the preceding C conversion semantics. They provide no new
source-level warning condition for the BASIC caller. The existing compiler
diagnostics remain applicable to the caller's own narrowing operations.

## Regression coverage

`tests/rtlib/string-scalar-values.c` includes the actual production helpers.
It checks decimal, binary, octal and hexadecimal input, signs, trailing text,
32-bit boundaries, native parser overflow and matching byte/wide results.
The observer compares the native parser error result as well as the value.

The wide-character cases visit every nonzero 16-bit code unit, every UTF-16
surrogate pair and every code point through U+10FFFF on 32-bit wide targets.
Additional signed high-bit patterns and out-of-range values cover raw ASC,
decoder offsets and replacement encoding. Unexpected descriptor allocation
or deletion aborts the component observer. Ordinary BASIC execution uses
the complete runtime archives and exercises descriptor ownership separately.

The Windows matrix qualifies eight tool/word-size/optimization contexts with
71,386,224 assertions. Six contexts execute afresh and two exact GCC64 contexts
are reused from the immediately preceding run with matching source/tool pins.
Clang32 compiles against the real MinGW32 headers and links its object through
GCC32. Both Win32 GCC contexts retain the two public-header ABI padding warnings.
All native warnings are captured without suppression.

`tests/rtlib/string-scalar-runtime.bas` checks ordinary string and WSTRING
`VALUINT` overloads and `ASC`, including high-bit 32-bit code units on Linux.
Source-only strict lint reports zero errors and zero warnings.

Eight Linux component contexts pass 71,337,392 assertions across GCC/Clang,
32/64-bit targets and O0/O2. Two original-source controls pass the same value
tests while retaining their original five conversion warnings under each tool.
Eight BASIC executions pass against the complete rebuilt runtime archives.
The preceding 76 runtime executions also pass afresh. Combined Windows and
Linux current component contexts cover 142,723,616 assertions. The Linux worker
owns at most 17,794,177 scratch bytes and removes its temporary tree.

## Reproduction

The component entry point supports an actual GNU-compatible native toolchain
and the target's installed system headers and libraries.

```sh
python3 -B build_scripts/test-string-scalar-warning-fixes.py \
  --cc /usr/bin/gcc --bits 64 --optimization 2 \
  --receipt /path/to/new-string-scalar-receipt.json
```

Repeat for both native tools, word sizes and optimization levels 0 and 2.
`--native-flags-json` supplies the complete audit flag array. The runner rejects
warning suppression, verifies tested source hashes and removes its temporary
products. The production changes, original source bytes and full native worker
inputs are pinned under `build/c-emitter-forward-views-20261105`.

- `runtime-current-packet-v30-20261115.json.zlib` contains the exact 1,328
  runtime/Make inputs and all public/component regression inputs.
- `runtime-current-linux-v26-20261115.json.zlib` contains both complete native
  runtime builds and current/original runtime controls.
- `string-scalar-windows-v4-20261115.json.zlib` contains all eight Windows
  component contexts and their unchanged source/tool identities.
- `string-scalar-qualification-v1-20261115.json.zlib` binds the result to exact
  tested sources, native receipts and preserved installed products.

Other project results retain their preceding qualified source cuts. Templari's
newer concurrent source/recipe edits await project replay. Its last qualified
674-profile result remains GCC 2 / Clang 53,469. Seventeen dynamic-format runtime
contracts, four finite-format compiler sites and other GCC/Clang classes remain
in the complete cleanup scope.

<!-- end of c-emitter-string-scalars.md -->
