<!--
    Project: FreeBASIC Runtime Library
    File: optical-freebasic-runtime.md
    Purpose: Record the Optical FreeBASIC command-set review and runtime APIs.
    Responsibilities: Evidence, API contracts, integration boundaries, validation.
    This file does not claim complete Visual Basic language or GUI compatibility.
-->

# Runtime services for Optical FreeBASIC

The September 2026 review adds `StrComp`, `Replace`, `StrReverse`, `GetAttr`,
and `SetAttr` to rtlib, and `FB.DrawStringSize` and `FB.PaintPattern` to both
gfxlib2 and gfxlib3. It also repairs existing file-copy, bitmap-font and indexed
screen-readback behavior that Optical depends on. These are declared helper
functions and existing-runtime fixes; no compiler keywords are added.

## Review inputs and findings

The supplied path was found at the sibling directory
`C:\Nextcloud\games\FreeBASIC_packages\Optical-Freebasic`. The review compared:

- `docs/HELP_VOCABULARY.md`: VBDOS and VBWIN1 language, graphics, sound,
  control, event, and toolkit inventories.
- `docs/VB3_VOCABULARY.md`: string comparison, date functions, attributes,
  object variables, DAO, OLE, and DDE.
- `docs/VB4_VOCABULARY.md`: filesystem calls, classes, properties, and providers.
- `docs/VB6_VOCABULARY.md`: later string helpers, file attributes, intrinsic
  objects, type tests, and the boundary with VB.NET.
- `docs/VERSION_RATIONALIZATION.md` and `docs/VB_FILE_FORMATS.md`: versioned
  language profiles, VB5 distribution/container evidence, and provider ownership.
- `docs/COMPATIBILITY.md`, `docs/GRAPHICS_COMPATIBILITY.md`,
  `docs/KEYBOARD_COMPATIBILITY.md`, and `docs/PICTURE_PRINT_COMPATIBILITY.md`,
  checked against `src/vbdos_graphics.bas`, the runtime list/print adapters,
  and omaGUI's rendering and clipboard code.

These are inventories and bounded compatibility reports, not executable
specifications of every command. In particular, the VB6 report says its main
language-reference CHMs are missing. VB5 has distribution evidence here,
not a separate complete command manifest. Microsoft's surviving classic
VBA references supplied the function-level cross-checks linked below; this
does not equate VBA, VB6, and FreeBASIC.

| Command family | Existing FreeBASIC support or gap | Decision for Optical |
| --- | --- | --- |
| Classic strings and math | `Left`, `Mid`, `Right`, `InStr`, `InStrRev`, trims, case conversion, numeric conversions and arithmetic already exist | Reuse the compiler/runtime. Check VB empty-search and comparison-option semantics when lowering `InStr`/`InStrRev`. |
| `StrComp` | Intrinsic comparison has no optional ASCII case-folding interface | Added a normalized comparison function; useful for names, sorting, and property matching. |
| `Replace` | No public optional string replacement function | Added bounded substring replacement; useful for source and property text transformations. |
| `StrReverse` | No public byte reversal helper | Added as a small companion from the classic string reference. |
| `FileCopy`, `FileLen`, `FileDateTime` | Already declared in `file.bi`, but Unix copying could truncate an aliased source and Windows copying bypassed UTF-8 path conversion | Repaired those `FileCopy` paths; adapt source-language error behavior at the VB boundary. |
| `GetAttr`, `SetAttr` | `FileAttr` describes an open file number, not a pathname; using `Dir` to query metadata consumes enumeration state | Added independent pathname operations. Useful for filesystem controls and setup/install adapters. |
| Date/time | `datetime.bi` already has `Now`, serial/value conversion, component functions, `DateAdd`, `DateDiff`, `DatePart`, and `Second` | Reuse and retain version-specific parsing/formatting tests. |
| `Format` | `string.bi` already exposes numeric/date formatting | No duplicate API; this does not establish complete VB masks or Variant formatting. |
| `IIf`, `Choose`, `Switch`, `Round`, financial functions | Some spellings overlap, but evaluation, coercion, rounding and Variant contracts differ | Define those contracts in Optical first. FreeBASIC `IIf` short-circuits, so it cannot blindly replace VB's eager function call. |
| `Like`, `Option Compare`, `StrConv`, `IsNumeric` | Pattern grammar, collation, encoding and coercion depend on the source profile | Do not lower them to regexes, `Val`, or case conversion by spelling alone. |
| `Split`, `Join`, `Filter`, `Array` and Variant tests | Require array result ownership, lower bounds, and/or Variant representation | Keep as a separately specified compatibility layer. No Variant/SAFEARRAY ABI is introduced here. |
| `PSet`, `Line`, `Circle`, `Point`, `View`, `Window`, `Draw`, numeric `Paint` | Existing gfxlib2 and gfxlib3 compatibility entry points | Reuse drawing operations. Form/picture state still belongs to Optical. |
| VBDOS packed `Paint` | Optical implements 1..64 packed monochrome rows through a checked mask; gfxlib's native string tile format differs | Added `FB.PaintPattern` to both renderers. Optical can lower its checked monochrome operation to this primitive without reserving a pixel value or scanning the entire viewport. |
| `Scale`, `ScaleX/Y`, `CurrentX/Y`, `TextWidth/Height`, fonts | VB object units and selected fonts need object state, but native bitmap-font measurement was also missing | Added `FB.DrawStringSize` for the font actually consumed by `DRAW STRING`; retain unit conversion, host-font shaping and multiline layout in Optical. Do not use `PMAP` as an exact substitute for the drawing transform. |
| PictureBox `Print`, `Cls`, `AutoRedraw`, repaint events | Optical owns a retained byte-cell text surface; VBDOS cursor units differ from Windows VB | Keep the print/layout/event state with the object. gfxlib's global console cannot supply it. |
| `LoadPicture`, `SavePicture`, `PaintPicture` | Images, `BLoad`/`BSave`, `Get`/`Put` and gfxlib3 surfaces exist; indexed screen `GET` had lost its working readback route | Repaired indexed readback. Reuse these primitives underneath a typed picture provider. BMP support does not imply ICO/metafile/OLE support. |
| VB colors, `RGB`, `QBColor` | VB packed color/system-color values differ from gfxlib color representation | Decode at the importer/object boundary; do not change FreeBASIC `RGB` or palette behavior. |
| `Beep`, `Sound`, `Play`, `PLAY(n)`, `On Play` | Tone/MML generation exists, but asset playback status and active-voice counts do not describe the legacy PLAY queue | Reuse synthesis. A faithful queue query needs note/rest scheduling state first; see the sound findings below. Optical still owns event dispatch. |
| `DoEvents`, `On Timer`, form/control events, `SendKeys` | Graphics/input and timers exist, but the runtime does not own Optical's event queue, object lifetime, or reentrancy rules | Keep dispatch and callback policy in Optical. A generic callback-pumping runtime call would lack the necessary ownership contract. |
| Clipboard, dialogs, `AppActivate`, settings, printing | omaGUI already has bounded clipboard and dialog adapters; remaining services depend on the host | Keep explicit providers and capability errors. A process-local clipboard must not be advertised as the host clipboard. |
| COM/ActiveX, DAO/RDO/ADO, VBIDE, DDE, toolkit libraries | Separate object/library APIs in the reviewed inventories | Do not make their members intrinsic rtlib, gfxlib or sfxlib functions. |

## String API

Include `string.bi`, or `vbcompat.bi` for the VB constant aliases.
All arguments and results are FreeBASIC byte strings. Embedded NULs are data.

```freebasic
#include once "vbcompat.bi"

Print StrComp("Caption", "caption", vbTextCompare)   ' 0
Print Replace("one two one", "one", "X", 5)          ' two X
Print StrReverse("Optical")                          ' lacitpO
```

| Function | Contract |
| --- | --- |
| `StrComp(string1, string2, compare = fbBinaryCompare) As Long` | Returns exactly -1, 0, or 1. Prefixes sort before longer strings; an empty string sorts before a nonempty string. |
| `Replace(expression, find_text, replacement, start = 1, count = -1, compare = fbBinaryCompare) As String` | Searches the original input from the one-based start, replacing non-overlapping matches. Returns only the suffix from start. Count -1 means unlimited. Empty find or count zero returns that suffix unchanged; empty replacement deletes matches; start beyond the input returns empty. |
| `StrReverse(expression) As String` | Returns an independent string with its bytes reversed. Empty input returns empty. |

The suffix rule for `Replace` is intentional and follows the
[classic Replace reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/replace-function).
Its normalized comparison results follow
[StrComp](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/strcomp-function).
[StrReverse](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/strreverse-function)
supplies the companion operation's name and basic contract.

`fbBinaryCompare`/`vbBinaryCompare` is 0: unsigned byte ordering.
`fbTextCompare`/`vbTextCompare` is 1: ASCII A-Z folds to a-z; other bytes retain
their values. This mode is deterministic across targets. It is **not VB's
locale collation**, and does not fold accented letters or decode UTF-8.
`StrReverse` likewise reverses bytes, not Unicode characters or graphemes.

Comparison values other than 0/1, `start < 1`, and `count < -1` set `Err` to
`FB_RTERROR_ILLEGALFUNCTIONCALL` (1). Invalid comparison returns 0 from
`StrComp`, so callers handling untrusted modes must also inspect `Err`.
Invalid replacement returns empty. Allocation failure or an output length
that cannot safely include rtlib's allocation slack sets
`FB_RTERROR_OUTOFMEM` (4) and returns empty. Successful calls clear `Err`.

There is no `Variant Null`, Access database comparison, or implicit
module-level `Option Compare`. Optical must resolve the selected profile's
comparison mode before calling these helpers. It must use a separate Unicode
provider when byte operations cannot preserve the source program's meaning.

The C entry points are `fb_StrComp`, `fb_StrReplace`, and `fb_StrReverse`,
declared in `src/rtlib/fb_string.h`. Start and count use `ssize_t`, mapped to
native-width indices by `string.bi`, including QB mode. The existing runtime
temporary-string convention applies: temporary arguments are consumed and
string results are runtime temporaries. Ordinary caller strings are neither
modified nor freed; arguments may alias. Allocation/descriptor access uses
the string lock in MT builds. Applications must synchronize their own writes
to shared strings.

## Pathname attributes

Include `file.bi` and `dir.bi`, or simply `vbcompat.bi`.

```freebasic
#include once "vbcompat.bi"

Dim attributes As Long = GetAttr("project.vbp")
If attributes = -1 Then
    Print "Attribute query failed: "; Err
ElseIf (attributes And vbReadOnly) <> 0 Then
    Print "Project has the read-only attribute"
End If

' On an application-owned file, check the status of every change:
' Dim status As Long = SetAttr("working-copy.vbp", vbReadOnly)
' If status <> 0 Then Print "Attribute change failed: "; status
```

`GetAttr(filename) As Long` returns attribute bits, or -1 on failure.
`SetAttr(filename, attributes) As Long` returns zero on success or a FreeBASIC
error code. Both set `Err` to their status. These are declared functions;
Optical must explicitly translate a failure into its source language's error
handling. They do not automatically raise VB's numbered errors or reproduce
VB's restriction on files open in the application.

Path parameters are borrowed `Const ZString Ptr`, following the existing file
helpers. They must point to terminated strings and cannot represent embedded
NULs. A null pointer or empty path is an illegal function call. No directory
enumeration state is read or changed.

The attribute bit values follow the classic
[GetAttr reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/getattr-function):
read-only 1, hidden 2, system 4, directory 16, archive 32, normal 0.
The writable VB-style mask is read-only, hidden, system, and archive;
directory and unknown bits are rejected, consistent with the separation in
the [SetAttr reference](https://learn.microsoft.com/en-us/office/vba/language/reference/user-interface-help/setattr-statement).

| Target | Query and mutation behavior |
| --- | --- |
| Desktop Windows | Uses native attributes. NT paths use the existing UTF-8-first/system-codepage-fallback conversion; Windows 9x uses ANSI APIs. Wildcards are rejected; the `\\?\` path prefix is permitted. Changes preserve native flags outside the writable VB mask. |
| Unix source layer and Cygwin | Uses `stat`, following symbolic links. Like `Dir`, synthesizes archive for nondirectories, system for special files, and read-only from the effective user/group mode-bit category. Hidden comes from the final dot-prefixed basename, excluding `.` and `..`. This is not an ACL/access check. Only normal (0) and read-only (1) changes are supported. |
| Remaining targets | The generic adapter returns illegal-function-call, with -1 for queries. There is no silent successful emulation. DOS, Windows CE, Xbox, Wii, NuttX and JavaScript need their own attribute adapters before these calls are usable there. |

On POSIX filesystems, setting read-only removes all three write bits; clearing
it enables owner write only. Read, execute and special mode bits are retained
subject to host `chmod` behavior. Group/other write permissions are not
restored. Unsupported attribute masks and special-file changes are rejected
before mutation. Paths can change between system calls; callers must own or
synchronize the paths they modify. The API does not change process `umask`.

Missing paths map to `FB_RTERROR_FILENOTFOUND` (2), permission failures to
`FB_RTERROR_NOPRIVILEGES` (8), other native I/O failures to
`FB_RTERROR_FILEIO` (3), and Windows path allocation failure to out-of-memory.
Attribute queries are snapshots; they cannot guarantee a later open succeeds.

The C entry points are `fb_FileGetAttr` and `fb_FileSetAttr` in `fb_file.h`.
The existing make source graph selects the generic, Unix, or Windows adapter
and includes the new string sources in normal, MT and applicable PIC archives.

### Compatibility constants

`vbNormal` remains the historical alias of `fbNormal`, whose value is **33**.
Changing a public constant that has shipped since 2006 would break existing
FreeBASIC and QB-dialect source. The new `fbFileAttrNormal` constant is **0**
and is the value to pass to `SetAttr` when clearing writable attributes.
Optical should lower VB's `vbNormal` source token to `fbFileAttrNormal` for an
attribute operation. It must not change the meaning of `vbNormal` for ordinary
FreeBASIC source.

`vbcompat.bi` also selects `Chr$` in QB mode when declaring its character
constants. This makes the umbrella header usable by the new QB smoke test.

## Graphics services and the closer source review

The object/provider boundary does not remove the need for reusable primitives
underneath it. The closer review traced Optical's `src/vbdos_graphics.bas`,
`omaGui/src/backend/backend_gfxlib.bas`, and its graphics/printing compatibility
reports into the actual drawing and readback implementations. That identified
two missing services and defects in existing ones.

### Measure the actual DRAW STRING font

```freebasic
#include once "fbgfx.bi"

' Requires a screen for its built-in font, or a valid explicit font image.
Dim pixelWidth As Long, pixelHeight As Long
Dim status As Long = FB.DrawStringSize("Caption", pixelWidth, pixelHeight)
```

`FB.DrawStringSize(text, pixelWidth, pixelHeight, fontImage = 0) As Long`
returns zero on success and sets `Err`. Output dimensions are **32-bit Long**
pixel counts, including on a 64-bit target. Invalid arguments/fonts or a width
exceeding `2147483647` return illegal-function-call (1) and zero both outputs.
The two output variables must be distinct. The C entry point is
`fb_GfxDrawStringSize(FBSTRING *, int *, int *, void *)`.

- It measures byte advances, including embedded NUL and control bytes, exactly
  as native `DRAW STRING` consumes them. It does not interpret tabs/newlines,
  shape Unicode, wrap text, or return ink bounds. Empty text has width zero and
  the full font height.
- A null font measures the active renderer's built-in font. gfxlib2 uses its
  selected built-in font; gfxlib3's current `DRAW STRING` uses its canonical
  8x8 font, independently of graphical `PRINT` row height. The query reports
  this existing distinction rather than concealing it.
- An explicit font is an ordinary readable CPU image in DRAW STRING's font
  format. It can be measured without a display and independently of screen
  depth. Actual drawing still requires font and target pixel depths to match.
  Old image headers must specify their pixel depth for standalone measurement.
- Unsupported bytes advance by the full custom font height; supported bytes
  may have zero width. The query leaves the pen, viewport, WINDOW transform,
  pixels and colors unchanged.

Both renderers now share `src/gfxlib2/gfx_font.h` for width-table validation
and measurement. The image's first row must contain the complete width table;
the sum of glyph widths must fit its visible pixel width, not row padding.
gfxlib2 previously omitted the first check and compared the second against
the **screen's** bytes per pixel. This rejected valid 8-bit font/image pairs
on a 32-bit screen and could accept malformed font geometry. Vertical clipping
also shortened the advance of unsupported glyphs. These defects are repaired,
along with horizontal clipping that could extend a glyph past the right edge
after clipping its left edge.

This is the primitive for an Optical adapter using native byte fonts.
omaGUI's own alpha-font glyph format still needs its existing metrics.
`TextWidth`/`TextHeight` must convert these pixel measurements into the selected
object's units; this function is not a complete VB font or printer service.

### Packed two-color border fills

```freebasic
#include once "fbgfx.bi"

' Native SCREEN 2 colors: zero and one. The target is the current work page.
Dim status As Long = FB.PaintPattern(0, 20, 20, Chr(128, 64, 32), 1, 0, 1)
```

`FB.PaintPattern(target, x, y, pattern, foreground = 1, background = 0,
border = 1, relative = 0) As Long` accepts `Single` coordinates, `ULong`
colors, and a `Long` relative flag. Nonzero relative selects STEP. It returns
zero on success and sets `Err`; malformed/empty/oversized patterns and
nonfinite or unrepresentable transformed seeds return 1. Allocation failure
returns 4. The C name is `fb_GfxPaintPattern`.

The pattern contains **one byte per row, 1 through 64 rows**, with bit 7 on
the left. A set bit chooses foreground; a clear bit chooses background.
Rows repeat at physical target `y modulo rowCount`; columns repeat at physical
`x modulo 8`. Both colors may equal the border color: flood discovery must
not mistake its own newly painted pixels for original boundaries.

Target zero selects the work page and respects VIEW, WINDOW, y direction and
STEP through the renderer's drawing transform. CPU images use their own pixel
coordinates and bounds. gfxlib3 also accepts its opaque surfaces. Colors use
normal gfxlib palette/RGB conversion at the target depth and overwrite raw
pixels, just like native string-pattern PAINT, without alpha blending.
An out-of-bounds seed or a seed on the border succeeds without changing pixels.

This directly supplies Optical's documented monochrome requirement. The
[original Microsoft QuickBASIC manual, section 5.8.2](https://www.pcjs.org/documents/books/mspl13/basic/qbprog/)
describes packed patterns including up to 64 rows in SCREEN 2. Optical's
`GRAPHICS_COMPATIBILITY.md` also records original-interpreter evidence for
physical pattern phase with VIEW, VIEW SCREEN and WINDOW. The new API
generalizes the two selected colors to other framebuffer depths; it does not
decode legacy multicolor bit planes or the legacy PAINT background-pattern
argument. `background` here is a **color**, not that legacy pattern argument.

gfxlib2 keeps its span algorithm, but holds the driver lock during discovery
as well as filling, indexes row storage relative to VIEW, and distinguishes
allocation failure from an already discovered span. It frees all spans and
returns the allocation error from `FB.PaintPattern` before painting any pixels.
Ordinary PAINT also avoids partial output on allocation failure, while keeping
its historical void ABI and `Err` behavior. Its native 8x8 raw-pixel pattern
ABI is retained.

gfxlib3 uses its existing bounded CPU flood and ordered readback/upload paths
for packed patterns. Its native GPU pattern command only supports 8x8 raw
pixels. An opaque surface therefore needs render-target, transfer-source and
transfer-destination capabilities for a packed fill. Unsupported capabilities
return an error; there is no claim that the new fill runs in a GPU shader.
Screen shadow/cache ordering follows the existing PAINT path. Failures leave
CPU targets unpainted when scratch allocation fails; this is not a general
transaction or rollback guarantee for a failing graphics driver.

Both helpers consume runtime temporary strings, retain no caller pointers,
and use the graphics lock. Callers own and must synchronize their image/font
memory and mutable strings. As with existing image APIs, a pointer must refer
to a live allocation large enough for its declared image layout; header
validation cannot prove the allocation's physical size.

In QB mode include `fbgfx.bi` and call the helpers without the `FB.` namespace.
No new compiler keyword is needed in either dialect.

### Indexed screen GET

The exhaustive packed-pattern tests reproduced a gfxlib3 screen GET failure
at depth 1. `image_api_ensure_work_shadow_locked()` rejected all indexed modes,
despite their native surfaces and FB.IMAGE buffers both using one byte per
palette index. The same issue affected depth 8. That unnecessary restriction
is removed; indexed GET now uses the coherent work-page shadow and preserves
indices, image pitch, and zeroed row padding. No palette-to-RGB conversion is
appropriate at this boundary.

## FileCopy fixes

Having an existing declaration did not establish that Optical could safely
use it for project assets. Unix `FileCopy(source, destination)` formerly opened
the destination with `"wb"` before comparing it with the source. Identical
paths, hard links and symlinks could therefore empty the source.

The Unix adapter now opens regular files without truncating first, compares
the opened descriptors' device/inode identities, and only then truncates the
destination. It handles interrupted reads/writes, short writes and close-time
errors. Nonblocking opens followed by regular-file checks reject FIFOs and
directories without waiting for another process. Cygwin uses this adapter.
The successful creation mode remains `0666` filtered by the caller's umask.

Windows now uses rtlib's UTF-8-first, system-codepage-fallback pathname
conversion and `CopyFileW`, with the native ANSI path on Windows 9x. Existing
Windows copy/metadata and alias behavior remains the operating system's job.
Invalid arguments return error 1; path-conversion allocation failure returns 4.
Other copy failures retain the existing error-1 contract.

Neither implementation provides atomic replacement, rollback after partial
write failure, or a snapshot of a concurrently modified source. The caller
must synchronize concurrent file-content writers. The generic CRT copy used
by some other targets was not replaced by the POSIX implementation.

## Sound findings and remaining API work

The closer sound review distinguishes music-asset status from MML scheduling:

- `src/sfxlib/sfx_play.c` computes pending time from active PLAY voices.
  Voice length is the articulated sounding interval. Standalone rests and the
  trailing silent part of a note are not represented as queue entries.
- Background phrases use a finite voice pool and an approximately two-second
  admission threshold. A dropped phrase or exhausted pool is not exposed as
  a faithful legacy PLAY-buffer state.
- `MUSIC STATUS` and the generic active-voice count consequently cannot stand
  in for VBDOS `PLAY(n)`. A correct `ON PLAY` threshold also needs the semantic
  queue, including rests and note completion; it is not solely an event-pump
  problem. The first review's broader reuse conclusion was too strong.

The next sound change needs a per-channel note/rest timeline, explicit queue
admission status, a defined treatment of the currently sounding note, and a
pollable threshold transition. Optical should dispatch the resulting event
at its own safe points. Adding a query that simply counts active oscillators
would make compatibility worse, so no such API is added here. This review
does not claim that those queue semantics have been implemented.

Other candidates remain useful but need separate contracts: byte/Unicode
`Like` and `StrConv`, dynamic-string-array `Split`/`Join`/`Filter`, Variant
coercion/type tests, eager `IIf`/`Choose`/`Switch`, financial functions, and
host clipboard/settings/printing services. They should not be treated as
unimplementable merely because they are object-adjacent. The inventory table
identifies the missing ownership, encoding or source-profile decisions; it
does not certify those families as already compatible.

## Validation and reproduction

Build the libraries with the normal target toolchain
(`make rtlib gfxlib2 gfxlib3`). No
compiler rebuild is needed. Applications need both the updated include files
and the rebuilt runtime archives; a previously installed library will not
contain the new symbols.

The automated tests are:

- `tests/string/vb-helpers.bas`: comparison, empty and binary strings,
  replacement limits, start semantics, errors, self-assignment, aliased
  arguments, fixed/zero-terminated conversions, and repeated nested temporary
  results beyond the runtime descriptor-pool capacity.
- `tests/file/path-attributes.bas`: private filesystem fixtures, native
  attributes, invalid masks, missing paths, constants, and a live `Dir` search
  across a query and a change.
- `tests/qb/vb-string-helpers.bas`: QB declarations, index width, constant
  aliases and runtime linkage.
- `tests/threads/vb-string-helpers.bas`: four concurrent callers, each making
  2,000 rounds of nested string calls and checking its own error status.
- `tests/string/vb-helpers-limits.c`: allocation failure injection, arithmetic
  limits without huge allocations, null descriptors, and aliased cleanup.
- `tests/file/path-attributes-posix.c`: actual POSIX mode bits, links, hidden
  directories, errors and cleanup. On MSYS2 use `MSYS=winsymlinks:lnk`; its
  default symlink operation can copy files instead of making a link.
- `tests/file/copy-pathnames.bas`: UTF-8 paths, self-copy rejection, complete
  content including NUL, and overwriting a longer destination.
- `tests/file/copy-alias-posix.c`: real hard-link/symlink identity, overwrite
  length, binary contents across multiple buffers, and nonblocking FIFO rejection.
- `tests/gfx/drawstring-size.bas`: public measurements, temporary strings,
  malformed font rows, mixed screen/image depths, clipping and padding.
- `tests/gfx/paint-pattern.bas`: every packed row count at depths 1/8/16/32,
  full-frame pixel comparisons, viewport/coordinate phase, CPU images, and
  gfxlib3 surface capabilities. GET supplies a single snapshot per screen fill
  so exhaustive checks do not issue a GPU readback for every pixel.
- `tests/gfx/font-metrics-limits.c`: malformed width tables and integer overflow
  without enormous allocations.
- `tests/gfx/paint-alloc.bas`, `.c`, `.bmk`: instrument gfxlib2's PAINT
  allocations, fail every row-table/span allocation in a small fill, and check
  unchanged pixels plus zero outstanding allocations. Source-tree runs compile
  the PAINT object with redirected allocators; staged package tests use linker
  wrapping because they intentionally omit the runtime source tree.
- `tests/qb/graphics-helpers.bas`: QB spelling, declarations and runtime calls
  for both helpers.

The string/pathname `.bas` suites use the existing fbcunit harness. Graphics
and QB fixtures are standalone executables. The C tests are small
standalone programs, built from the repository root, for example:

```sh
gcc -Wall -Wextra -Werror -O2 tests/string/vb-helpers-limits.c \
    src/rtlib/str_comp_vb.c src/rtlib/str_replace.c src/rtlib/str_reverse.c \
    -o /tmp/vb-helpers-limits
/tmp/vb-helpers-limits

gcc -Wall -Wextra -Werror -O2 tests/file/path-attributes-posix.c \
    src/rtlib/unix/file_attrs.c -o /tmp/path-attributes-posix
/tmp/path-attributes-posix
```

Verified on Windows x86-64 with installed FreeBASIC 1.20.2-14 and the rebuilt
runtime from this checkout:

- `make -j8 rtlib CC=/mingw64/bin/gcc AR=/mingw64/bin/ar` completed for
  `libfb.a` and `libfbmt.a`. The final incremental build had no diagnostics.
- Normal fbcunit build: 2,158 assertions across eight tests passed.
- MT build, including the four-thread stress: 2,166 assertions across nine
  tests passed. Both builds include UTF-8 and extended Windows path checks.
- A focused MT fbcunit shard linked all new string, file and graphics modules
  together and passed 2,813 assertions across 88 tests. This also verifies
  that standalone test helpers retain module-local linkage in the aggregate.
- The QB program compiled and exited 0, including indices/counts exceeding
  32 bits on this 64-bit target. Both QB helper sources also compiled for
  32-bit x86, where the public start/count ABI maps to a 32-bit `Long`.
- The standalone C string allocation/overflow checks passed with both the
  Windows and POSIX compilers. The POSIX attribute test passed under MSYS2's Cygwin-compatible
  compiler/runtime with `MSYS=winsymlinks:lnk`, including a verified symlink.
- Both normal and MT gfxlib2/gfxlib3 archives built successfully. The graphics
  builds had no compiler diagnostics. Through GFX_NULL, each of the four
  variants passed 4,438 font checks. Packed fill/readback passed 999,577 checks
  per gfxlib2 variant and 1,001,381 per gfxlib3 variant, including opaque surfaces.
- The existing `custom-font-smoke` produced matching gfxlib2/gfxlib3 hashes
  `B1C32E2D` and `5EDE6DDD`. gfxlib3's existing custom-font-depth and native
  paint-pattern-depth fixtures also exited zero.
- The QB graphics fixture passed against both renderers. The font-metrics C
  boundary test and instrumented gfxlib2 PAINT allocation test passed.
- The real POSIX copy-alias test passed with verified hard links, symlinks and
  FIFOs. The new C fixtures compile with `-Wall -Wextra -Werror`.
- fblint's strict Windows profile reported zero errors across the changed/new
  13 `.bi` and `.bas` files (24 warnings). Retained warnings concern existing include/layout
  declarations, dialect branches, deliberate `Err` assertions, test counters,
  bounded pattern construction, and fixtures protected by successful `MkDir`.
  It does not recognize the test assertion wrapper around checked SCREENRES.
- The log-test harness passed every new Optical-related case. Its complete
  `fblite`, `qb`, and `deprecated` language sweeps had no failures. The broad
  `fb` sweep reported eleven compiler conformance failures when the checkout's
  newer tests were compiled by installed FreeBASIC 1.20.2-14; those failures
  are in preprocessor, const/CVA, escaped-alias, layout, LRSET and WString
  tests, with no dependency on the changed include or runtime files.

For the public graphics fixtures, from the repository root:

```text
fbc -i inc -p lib/freebasic/win64 tests/gfx/drawstring-size.bas -x font-test.exe
fbc -i inc -p lib/freebasic/win64 tests/gfx/paint-pattern.bas -x paint-test.exe
```

Add `-d __FB_GFXLIB3__` for gfxlib3 and `-mt` for the threaded archives. Run
each produced executable and require exit zero. The installed compiler is
outside this checkout; `-p` is essential to select the rebuilt libraries.

The fbcunit support code emits five existing generated-C const-qualification
warnings when built by the installed compiler. The new C boundary tests
compile with `-Wall -Wextra -Werror`. The local POSIX compiler could not link
`-fsanitize=undefined` because `libubsan` is unavailable; the reported C runs
are ordinary native tests, not sanitizer runs.

Linux, other Unix targets, Windows 9x, hardware graphics backends and 32-bit
runtime builds were not executed in this review. The QB sources were compiled
for 32-bit Windows and their imported symbols were checked for the required
stdcall decoration. These checks do not establish a complete Optical source
translation, non-ASCII VB collation, or graphics/sound compatibility on every
target.

<!-- end of optical-freebasic-runtime.md -->
