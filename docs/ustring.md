<!--
Project: FreeBASIC
File: ustring.md
Purpose: Describe the built-in UTF-8 string type and its runtime contract.
Responsibilities: Explain text semantics, conversions, examples, and validation.
This file intentionally does NOT contain platform console setup instructions.
-->

# USTRING

`USTRING` is a built-in dynamic UTF-8 string. It needs no include file. Its
descriptor has the same layout as `STRING`: data pointer, byte length, and
allocated byte capacity. Copying, cleanup, procedure results, arrays, and record
fields use the existing descriptor ownership conventions.

```freebasic
dim as ustring text = "caf" + uchr(&hE9) + " " + uchr(&h1F600)
print len(text)                  '' 6 code points
print mid(text, 4, 1)            '' é
print hex(asc(text, 6))          '' 1F600
text[3] = &h1F642                '' replaces é, resizing the bytes as needed
```

Positions and lengths count Unicode scalar values, also called code points.
String functions use one-based positions; bracket indices use zero-based
positions and return `ULONG` scalars. Combining marks count separately, as do
the components of emoji sequences. Text is not automatically normalized into
NFC or NFD. Equality and ordering compare scalar sequences, without linguistic
collation.

Embedded NULs are preserved because the descriptor stores a byte length.
`STRPTR(text)` exposes the UTF-8 bytes. A caller that writes through this pointer
must preserve valid UTF-8, capacity, and the descriptor's byte length. An indexed
scalar has no stable address; taking its address is rejected.
An indexed read outside the string returns zero. An indexed write outside the
string leaves it unchanged and sets `ERR` to illegal function call.

## Construction and conversion

`USTRING(value)` accepts byte strings, wide strings, and numeric values.
`UCHR(codepoint, ...)` constructs up to 32 Unicode scalars. `USTRING(count,
codepoint)` repeats a scalar, and `USTRING(count, text)` repeats the first scalar
in `text`. `STRING(count, ustringValue)` also selects the UTF-8 implementation.
Surrogates, negative scalar values, and values above U+10FFFF become U+FFFD.

Assigning `STRING`, `ZSTRING`, or a narrow literal interprets its bytes as UTF-8.
Malformed input becomes U+FFFD using the Unicode maximal-subpart rule. Valid
bytes, embedded NULs, and combining sequences are retained. Assigning back to
`STRING` preserves the encoded bytes, so `LEN()` on that result counts bytes.
`CAST(STRING, text)` also makes an explicit copy of the encoded bytes.
Explicit `CONST AS USTRING` literals and `DIM AS CONST USTRING` variables retain
the type's Unicode semantics. A descriptor constant is constructed when used;
its string operations are evaluated at runtime.

`WSTR(ustringValue)` and assignment to `WSTRING` transcode to the target wide
representation. UTF-16 targets use surrogate pairs for supplementary scalars;
UTF-32 targets use one wide unit. As with other `WSTRING` operations, an embedded
NUL terminates subsequent wide-string reads. Targets with byte-sized `WSTRING`
storage substitute `?` for scalars above U+00FF.

Ordinary text files store UTF-8 bytes. Files opened with `ENCODING "utf-8"`,
`"utf-16"`, or `"utf-32"` transcode directly between that file encoding and
`USTRING`, retaining supplementary scalars and validating malformed input.

## Standard operations

| Operation | USTRING behavior |
| --- | --- |
| `LEN`, `ASC`, `text[index]` | Count or access Unicode scalars |
| `LEFT`, `RIGHT`, `MID` | Slice whole scalars and return `USTRING` |
| `MID` assignment, indexed assignment | Replace scalars; adjust the encoded byte length |
| `INSTR`, `INSTRREV`, including `ANY` | Return scalar positions; `ANY` matches whole scalars |
| `TRIM`, `LTRIM`, `RTRIM` | Remove spaces, repeated text patterns, or scalar sets with `ANY` |
| `UCASE`, `LCASE` | Apply Unicode 17.0 default full mappings, including expansions and final sigma |
| `UCASE(text, 1)`, `LCASE(text, 1)` | Apply the existing ASCII-only mode |
| `STRING(count, text)` | Repeat the first scalar when `text` is `USTRING` |
| `LSET`, `RSET` | Keep the destination's scalar width, truncating or padding with spaces |
| Concatenation and comparisons | Convert mixed UTF-8/wide operands; concatenation retains `USTRING` |
| `SWAP`, `IIF`, `SELECT CASE` | Preserve descriptor lifetime and Unicode text semantics |
| `STR`, `VAL`, `VALINT`, `VALLNG`, `VALUINT`, `VALULNG` | Accept `USTRING` through the existing string interfaces |
| `PRINT`, `WRITE`, `INPUT` statement, `LINE INPUT`, `READ`, `GET`, `PUT` | Use the descriptor's UTF-8 bytes; input validates UTF-8 |

The default case tables are generated from Unicode 17.0 `UnicodeData.txt`,
`SpecialCasing.txt`, and `DerivedCoreProperties.txt`. They are checked in, so
building FreeBASIC does not download Unicode data. Locale-specific casing is
not applied. The algorithms follow the Unicode standard's
[default casing and malformed-input rules](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/).

`BYVAL AS USTRING` makes an independent copy. `BYREF AS USTRING` preserves direct
reference behavior for another `USTRING`. Crossing a mutable `STRING`/`USTRING`
parameter boundary uses a temporary and copies back through assignment, which
validates UTF-8. Callers must synchronize concurrent writes to the same variable.
The multithreaded runtime lock protects temporary descriptors, rather than
providing a transaction around application-level string changes.

`USTRING * N` is rejected: the descriptor owns a dynamic byte buffer. Existing
fixed strings and binary string functions continue to use their existing byte
representations. Existing `STRING` programs retain their byte-oriented behavior.
The type is available in the FB, FBlite, and deprecated dialects; QB retains its
original keyword set.

## Examples and tests

- [Construction, conversion, slicing, and indexing](../examples/manual/strings/ustring.bas)
- [Search, trimming, casing, repetition, and alignment](../examples/manual/strings/ustring-functions.bas)
- [Procedures, records, arrays, and file I/O](../examples/manual/strings/ustring-io.bas)

Run `make ustring-test` after selecting a runnable host build compiler. The
runner builds its artifacts in a temporary directory, runs the language suite
and rejection tests across available native backends, and compiles and executes
all three examples. On Linux it also runs address and undefined-behavior
sanitizers over every Unicode scalar, every case-table entry, malformed input,
allocation limits, aliasing, and temporary ownership, with both UTF-16 and
UTF-32 wide storage. The language suite is also discovered by the ordinary
FreeBASIC unit-test harness.

To regenerate casing data from an unpacked Unicode 17.0 UCD directory:

```sh
python3 build_scripts/generate-ustring-case.py /path/to/ucd
```

<!-- end of ustring.md -->
