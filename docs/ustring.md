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
| `PRINT USING`, `LPRINT USING` | Accept all three text types; Unicode fields count scalars and retain whole UTF-8 sequences |

The default case tables are generated from Unicode 17.0 `UnicodeData.txt`,
`SpecialCasing.txt`, `DerivedCoreProperties.txt`, and `CaseFolding.txt`. They are checked in, so
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

## Shared text APIs

Every built-in text consumer accepts `STRING`, `WSTRING`, and `USTRING`.
Procedures which operate on a byte descriptor or terminated byte pointer have
a UTF-8 argument route: wide text is encoded explicitly, and UTF-8 descriptors
retain their bytes. Mutable text outputs use a temporary and copy back into
the caller's text type. This includes device names, filenames, environment
variables, process commands, dynamic library names, graphics controls, and
sound commands. The operating system or device provider owns the final host
encoding and any platform restrictions.

The optional FreeBASIC headers provide the following paths:

| Header | Operations | Unicode contract |
| --- | --- | --- |
| `string.bi` | `StrReverse`, `Replace`, `StrComp`, `Format` | Explicit wide/UTF-8 overloads; `Replace` retains the source type; all 27 source/pattern/replacement combinations are supported |
| `string.bi` | `StrComp`, `Replace` with `fbTextCompare` | Unicode default full case folding for wide/UTF-8 text, including expansions such as sharp s to `ss` |
| `datetime.bi` | `DateValue`, `TimeValue`, `IsDate`, `DateAdd`, `DatePart`, `DateDiff` | Accept all three types through the existing date/time grammar |
| `file.bi`, `dir.bi` | `FileExists`, `FileLen`, `FileDateTime`, `GetAttr`, `SetAttr`, `FileCopy`, `DIR` | Wide paths encode to UTF-8 independently of the process locale; mixed copy arguments are supported |
| `fbgfx.bi` | Graphics-mode `PRINT`, `DRAW STRING`, `DrawStringSize` | Decode a whole scalar per glyph; existing bitmap fonts address U+0000..U+00FF, with `?` for larger scalars |
| `fbgfx.bi` | `PaintPattern` | Preserve the packed-byte format; wide/UTF-8 arguments contribute UTF-8 bytes, one byte per pattern row |
| `fbgfx3.bi` | `Gfx3SurfaceLoad` | Accept byte, wide, and UTF-8 asset filenames; gfxlib3 also exports the common Unicode bitmap adapters |
| `fbnetwire.bi` | `FbNetPutStringLE`, `FbNetGetStringLE` | Keep the Int32 byte-length prefix; Unicode writes truncate only at scalar boundaries; UTF-8 reads validate malformed input |
| `sfxlib_raw.bi` | `OutputCaptureSave` | Accept UTF-8 bytes and transcode wide filenames explicitly |
| `fbc-int/string.bi` | `FBC.LeftSelf` | Truncate byte strings by bytes and Unicode strings by scalars; retain the existing allocation |
| `fbc-int/profile.bi` | Profiler names, ignore lists, and report filenames | Accept UTF-8 bytes and wide names; bound wide output without splitting UTF-16 pairs |

The optional wide string helpers operate on scalars, including UTF-16 pairs.
Core `WSTRING` operations retain their established target-wide-unit semantics.
The new type does not change `STRING`'s byte operations or external C library
declarations. C byte buffers, binary conversion functions, and paint patterns
continue to consume bytes. Use `STRPTR()` and the encoded byte length when an
external interface explicitly requires bytes.

Wide output buffers require a capacity because a `BYREF AS WSTRING` parameter
does not carry its caller's allocation size. The network reader takes this as
its fourth argument, in wide units including the terminator:

```freebasic
dim as wstring * 32 text
if FbNetGetStringLE(handle, text, 128, 32) = 0 then
    print "Read failed or the wide buffer was too small"
end if
```

The compiler defines `__FB_HAS_USTRING__` for native type support. Optional
headers gate their UTF-8 overloads on this marker and `FB_NO_USTRING`, retaining
the existing compatibility switch for applications with a legacy `ustring`
identifier or macro.

## Examples and tests

- [Construction, conversion, slicing, and indexing](../examples/manual/strings/ustring.bas)
- [Search, trimming, casing, repetition, and alignment](../examples/manual/strings/ustring-functions.bas)
- [Procedures, records, arrays, and file I/O](../examples/manual/strings/ustring-io.bas)
- [Optional helpers and formatted output with all three types](../examples/manual/strings/ustring-helpers.bas)
- [Unicode filenames and portable wire strings](../examples/manual/strings/ustring-wire.bas)

Run `make ustring-test` after selecting a runnable host build compiler. The
runner builds its artifacts in a temporary directory, runs the language suite
and rejection tests across available native backends, and compiles and executes
all five examples with assertions and bounds checking enabled. It also checks
the API compilation fixtures, mixed optional overloads, both graphics libraries,
profiler output, and sound arguments through an observing C stub. Running with
`LC_ALL=C` verifies that wide text adapters do not depend on a UTF-8 locale.
On Linux it also runs address and undefined-behavior
sanitizers over every Unicode scalar, every case-table entry, malformed input,
allocation limits, aliasing, and temporary ownership, with both UTF-16 and
UTF-32 wide storage. The language suite is also discovered by the ordinary
FreeBASIC unit-test harness.

To regenerate casing data from an unpacked Unicode 17.0 UCD directory:

```sh
python3 build_scripts/generate-ustring-case.py /path/to/ucd
```

<!-- end of ustring.md -->
