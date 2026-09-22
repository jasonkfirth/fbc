<!--
    Project: FreeBASIC Runtime and Compiler
    File: compatibility-services.md
    Purpose: Document reusable compatibility helpers and compiler build isolation.
    Responsibilities: Public contracts, implementation boundaries, and tests.
    This file does not define another language's parser, object model, or events.
-->

# Compatibility runtime and compiler services

FreeBASIC 1.20.4 adds a small set of reusable byte-string, pathname, and
graphics services. It also hardens existing file and graphics operations and
isolates compiler-owned build products so independent compiler processes can
consume the same source tree safely.

These additions are ordinary declarations and runtime entry points. They do
not add compiler keywords, alter the historical dialects, or introduce a
Variant, object, event, clipboard, or source-language compatibility layer.

## Byte-string helpers

Include `string.bi`, or `vbcompat.bi` when the compatibility constant aliases
are useful.

```freebasic
#include once "vbcompat.bi"

Print StrComp("Caption", "caption", vbTextCompare)  ' 0
Print Replace("one two one", "one", "X", 5)         ' two X
Print StrReverse("sample")                          ' elpmas
```

| Function | Contract |
| --- | --- |
| `StrComp(string1, string2, compare = fbBinaryCompare) As Long` | Returns exactly -1, 0, or 1. Prefixes sort before longer strings. |
| `Replace(expression, find_text, replacement, start = 1, count = -1, compare = fbBinaryCompare) As String` | Replaces non-overlapping matches in the original input and returns the suffix beginning at the one-based start. Count -1 means unlimited. |
| `StrReverse(expression) As String` | Returns an independent string with its bytes reversed. |

The functions operate on FreeBASIC byte strings, including embedded NULs.
Binary comparison uses unsigned byte ordering. Text comparison folds ASCII
`A` through `Z` only, making the result deterministic across targets. It does
not provide locale collation, UTF-8 decoding, Unicode case folding, grapheme
reversal, or an implicit module comparison mode.

Invalid comparison values, `start < 1`, or `count < -1` set `Err` to
`FB_RTERROR_ILLEGALFUNCTIONCALL`. Allocation or checked output-size failure
sets `FB_RTERROR_OUTOFMEM`. Successful calls clear `Err`. Runtime temporary
arguments follow the existing consumption rules, ordinary caller strings are
not freed or modified, and arguments may alias.

The C entry points are `fb_StrComp`, `fb_StrReplace`, and `fb_StrReverse` in
`src/rtlib/fb_string.h`. Start and count use native-width `ssize_t` values,
including through the QB declarations.

## Pathname attributes

`GetAttr(filename) As Long` returns attribute bits or -1 on failure.
`SetAttr(filename, attributes) As Long` returns zero or a FreeBASIC runtime
error code. Both set `Err` and leave `Dir` enumeration state untouched.

The public values are read-only 1, hidden 2, system 4, directory 16, and
archive 32. `fbFileAttrNormal` is zero and clears writable attributes.
Historical `vbNormal` remains the existing `fbNormal` alias with value 33;
changing that shipped constant would break source compatibility.

| Target | Behavior |
| --- | --- |
| Desktop Windows | Uses native file attributes and the runtime's UTF-8-first pathname conversion. Writable changes preserve unrelated native flags. |
| Unix and Cygwin source layers | Uses `stat`, follows symbolic links, derives hidden state from the final dot-prefixed basename, and supports normal/read-only changes through mode bits. |
| Other targets | Returns illegal-function-call until that target supplies an attribute adapter. |

Null or empty paths, wildcards, directory mutation, unknown mask bits, and
unsupported special-file changes are rejected before mutation. POSIX
read-only changes remove all write bits; clearing read-only enables owner write
without inventing group or other permissions.

The C entry points are `fb_FileGetAttr` and `fb_FileSetAttr` in `fb_file.h`.

## Graphics helpers

### `FB.DrawStringSize`

```freebasic
#include once "fbgfx.bi"

Dim pixel_width As Long, pixel_height As Long
Dim status As Long = FB.DrawStringSize( _
    "Caption", pixel_width, pixel_height _
)
```

`FB.DrawStringSize(text, width, height, font_image = 0) As Long` measures the
same byte advances consumed by `DRAW STRING`. Width and height are 32-bit pixel
counts on every target. A null font measures the active renderer's built-in
font; an explicit font is validated as an ordinary CPU image using the native
font layout.

The function does not shape Unicode, interpret newlines or tabs, wrap text,
return ink bounds, or convert pixels into application-defined units. Invalid
arguments and malformed font geometry return illegal-function-call and zero
both outputs. The two output variables must be distinct.

gfxlib2 and gfxlib3 share `src/gfxlib2/gfx_font.h` for validation and
measurement. The validation repairs mixed-depth font checks, width-table
bounds, unsupported-glyph advance, and horizontal clipping behavior in the
existing drawing path.

### `FB.PaintPattern`

```freebasic
#include once "fbgfx.bi"

Dim status As Long = FB.PaintPattern( _
    0, 20, 20, Chr(128, 64, 32), 1, 0, 1 _
)
```

`FB.PaintPattern(target, x, y, pattern, foreground = 1, background = 0,
border = 1, relative = 0) As Long` performs a bounded flood fill with a packed
two-color pattern. The pattern contains one byte per row and may contain 1
through 64 rows. Bit 7 is the leftmost bit. Rows repeat by physical target Y
and columns repeat every eight physical pixels.

Target zero selects the work page and respects `VIEW`, `WINDOW`, Y direction,
and `STEP`. CPU images use image coordinates; gfxlib3 also accepts capable
opaque surfaces. Both selected colors may equal the border color because fill
discovery is kept separate from output pixels.

Malformed patterns, nonfinite or unrepresentable coordinates, and invalid
targets return illegal-function-call. Allocation failure returns
`FB_RTERROR_OUTOFMEM` before a CPU target is painted. Existing native
string-pattern `PAINT` behavior and ABI remain unchanged.

### Indexed readback

gfxlib3 screen `GET` now accepts indexed depth 1 and depth 8 work-page shadows.
The surface and destination both contain palette indices, so no palette-to-RGB
conversion belongs at this boundary. Image pitch and zeroed row padding remain
part of the existing readback contract.

## File copying

The Unix `FileCopy` adapter no longer truncates the destination before checking
whether source and destination name the same file. It opens without truncation,
compares device/inode identity, rejects nonregular inputs without blocking on a
FIFO, and handles interrupted reads, short writes, and close errors.

Windows `FileCopy` now uses the runtime pathname conversion and `CopyFileW`,
with the native ANSI path retained for Windows 9x. These changes do not promise
atomic replacement, rollback after a partial failure, or a snapshot of a
concurrently modified source.

## Concurrent compiler processes

Compiler-owned backend files, response files, link/archive object files, and
static-library metadata use a tag containing the compiler process ID and timer
value. This prevents simultaneous fbc processes from compiling, linking,
deleting, or archiving one another's intermediates when they consume the same
source paths.

Static-library metadata is built inside a private directory. The archiver still
stores the required member basename `__fb_ct.inf`, preserving the archive
format. Native DOS uses hashed 8.3-safe private names.

User-visible outputs retain their historical names:

- compile-only `-c` objects;
- objects retained by `-C`;
- explicit per-source `-o` objects;
- backend or final assembly emitted or retained by `-r`, `-rr`, `-R`, and
  `-RR`.

The focused regression is:

```text
python build_scripts/fbc-concurrent-smoke.py \
  --fbc bin/fbc.exe --ar <toolchain>/bin/ar.exe --jobs 6
```

It builds and executes six programs concurrently, creates six archives
concurrently, checks the first metadata member, exercises every public output
mode listed above, and rejects leaked files or private directories.

## Tests and qualification

The focused source tests are:

- `tests/string/vb-helpers.bas` and `tests/string/vb-helpers-limits.c`;
- `tests/qb/vb-string-helpers.bas` and
  `tests/threads/vb-string-helpers.bas`;
- `tests/file/path-attributes.bas` and
  `tests/file/path-attributes-posix.c`;
- `tests/file/copy-pathnames.bas` and `tests/file/copy-alias-posix.c`;
- `tests/gfx/drawstring-size.bas` and
  `tests/gfx/font-metrics-limits.c`;
- `tests/gfx/paint-pattern.bas`, `tests/gfx/paint-alloc.bas`, and its C/BMK
  allocation fixtures;
- `tests/qb/graphics-helpers.bas`;
- `build_scripts/fbc-concurrent-smoke.py`.

`examples/nuttx/fbhello_smoke.bas` obtains and validates one `FreeFile` handle,
then checks `Err` after every `Open` before using it. A failed RAM-filesystem
operation names the operation and ends the smoke, avoiding misleading output
from later reads or writes that have no open file behind them. Its multi-
dimensional `ReDim` check also exits before bounds or element access if an
unexpectedly empty result is observed.

`FBL-DOC-BIN-003` tracks each ordinary binary file handle until its first raw
`Put #` or `Get #` transfer, then reports one missing format-contract warning
at that transfer. A binary handle used only for `Lock` and `Unlock`, or a live
`OPEN COM` or `OPEN PIPE` byte stream, does not establish a serialized file
format and is not reported by this rule.

`FBL-STR-009` requires both a fixed-length `String` field in a UDT and a later
raw Binary or Random file transfer in the same source file. This retains the
portable file-layout warning for serialized records but excludes in-memory
records and external API/ABI declarations that are not file formats.

The NuttX smoke deliberately exercises FreeBASIC's formatted `Input #` and
`Write #` record syntax. Each such operation has an explanatory,
`FB-LINTER: DISABLE-NEXT-LINE FBL517` annotation. The suppression is limited to
one reviewed smoke statement, is honored only when requested, and leaves
`FBL517` active for every unannotated formatted file operation.

The same smoke names only literal fixtures under NuttX's `/ram` filesystem and
checks every output-file open immediately. Its output-open annotations are
limited to those RAM fixtures, leaving `FBL-IO-005` active for ordinary output
paths. The two raw binary round trips state their same-build byte contracts,
and the Random-record sample records its native-Integer plus fixed-six-byte
layout. Before it uses relative destructive operations, the smoke confirms
that `CurDir` is exactly `/ram/fbdir`; each `Rmdir` then checks that its named
directory exists and reads `Err` immediately after the removal.

In strict mode, `FBL-IO-005` is the specific review rule for non-temporary
output paths, so the broader legacy `FBL103` reminder is not duplicated. The
lighter profiles retain `FBL103` when an output `Open` is not immediately
followed by an `Err` comparison. This preserves review for unchecked writes
without flagging a stream whose acquisition is already checked. `FBL613`
likewise recognizes an immediate `Err` read after `Rmdir`, rather than
treating checked directory-removal failure as a stale-error read.

`examples/nuttx/fbfilecopy_smoke.bas` uses two fixed `/ram` fixture names and
one checked `FreeFile` unit. It treats a nonzero `fb_FileCopy` result as a
failed transfer, and its cleanup routine first confirms the source or
destination fixture with `Dir` before removing it. This makes every early
failure path release the open unit before it removes only a known smoke file.

The MicroSD smoke uses the same pattern for its single, fixed `/mnt/sd0`
fixture. It verifies allocation of the file unit, checks each open before I/O,
and conditionally removes the file after a failed readback as well as on the
successful path.

The combined NuttX device smoke treats `Screen 13` as its documented DVI-lab
mode, restores text mode on every later failure path, and gives its fixed mass-
storage fixture the same checked `FreeFile`, open, readback, and conditional
cleanup policy.

The GOSUB smoke declares its `fblite` runtime contract at the `Option Gosub`
statement and at each label `Return`. The graphics and HID smokes similarly
document their Screen 13 DVI/QEMU mode at the exact setup statement. The visible
demo's short sleep is bounded keyboard polling, not unattended pacing; every
graphics smoke restores text mode before a failure exit after graphics setup.

The PCRE class owns a top-level match-array table, one allocated match array
per result, and PCRE-provided substring storage within each match. It replaces
the table only after `Reallocate` succeeds, checks the vector and match-array
allocations before PCRE uses them, and clears every freed pointer plus its match
counts in `clean_up()`. A failed PCRE substring or allocation therefore leaves
no partial result owned by the object.

The NeHe MilkShape loader represents the packed MS3D version field as a
fixed-width 32-bit `Long`, restoring the format's 14-byte file header on
64-bit hosts. It verifies each count and record range against the loaded file
before parsing, rejects invalid mesh, triangle, vertex, and material indices,
and allocates only verified record counts. The temporary input buffer remains
loader-owned; parsed arrays and texture names transfer to `MODEL` and are
released idempotently by `Model_Delete`.

The companion NeHe BMP loader accepts only bounded, uncompressed 8-bit and
24-bit BI_RGB records. It validates the packed headers, dimensions, offsets,
palette size, row stride, file extent, and every read before returning an RGB
buffer. It honors BMP row padding and the one-based FreeBASIC binary `Seek`
contract for the zero-based `bfOffBits` field; a successful result transfers
the image and pixel buffer to the caller.

The NeHe picking example now tests its mouse-button bit explicitly for zero
before clearing the held-button state, avoiding bitwise `Not` precedence and
truth-value ambiguity. Its font and explosion atlas divisions are documented
as deliberate integer cell selection, its byte-array TGA header comparison is
identified as padding-free, and its fixed selection and viewport arrays state
their zero-based bounds.

The CACA metaball demo identifies its two shared render buffers as
single-threaded module state used only by sprite creation and compositing. Its
palette helper makes the original calculation explicit: each sine-modulated
component is rounded, integer-divided, and then added to the palette entry.
That preserves the original palette while making its intentional quantization
auditable.

The TinyPTC fblite sprite demo now distinguishes the shared display and sprite
arrays from its generic drawing-array parameters. The helper signatures remain
the same array forms and the call graph is unchanged; the distinct names make
it clear when a helper operates on a supplied target rather than the demo's
global display. Section markers and normalized local indentation make the old
multi-routine sample easier to navigate without changing its drawing logic.

The TinyPTC flower tunnel states zero-based bounds for every shared lookup
table it indexes that way. Its tunnel helper likewise distinguishes supplied
target, texture, angle, and depth arrays from the shared tables. The distance
routine returns a neutral value for empty or mismatched coordinate arrays before
traversing them, while retaining its original result for the fixed, matching
tables used by the demo.

The TinyPTC blob demo similarly states the lightmap's zero-based bounds and
separates the generic compositor target and lightmap arguments from its shared
arrays. The compositor checks both lightmap dimensions before querying their
bounds or indexing them. Its standalone `ErrorQuit` routine remains an explicit
process-termination path after it reports an unrecoverable error.

`FBL008` ignores only the commented `' Declare ...` form emitted by COM type-
library importers. Those comments describe external properties such as a
password field but carry no executable credential value. Executable credential
assignments and ordinary comments remain subject to the rule.

The QuickLZ compression example owns one codec state object and an input/output
buffer pair per operation. It verifies every allocation before use, releases
only resources already acquired when an operation fails, and clears each pointer
after release. Its file-handle variable is an `Integer`, matching `FreeFile`.
The manual LZO example follows the same ownership model, while avoiding a
decompression-workspace allocation because its `lzo1x_decompress` call uses no
workspace argument.
The zlib example uses `uLong_` for the binding's length parameters, seeds its
generated test input, and applies the same checked allocation and cleanup
policy to its source and destination buffers.
The manual `ALLOCATE` leak demonstration intentionally leaves its two
allocations unchecked, so each line carries a documented, narrow `FBL800`
suppression. Scans without suppressions continue to expose both bad examples.

`FBL015` through `FBL019` are presentation diagnostics available through the
explicit `style` profile; strict review remains focused on compatibility and
runtime safety. `FBL525` still reports indexed pointer casts with unknown
bounds, but recognizes that `New Type[count]` and `Operator New[]` describe an
allocation rather than a pointer dereference. `Operator Delete[]` declarations
and definitions have the same bracket syntax boundary, so they are likewise
excluded without weakening checks on actual indexed pointers.
Placement `New(storage) Type[count]` uses brackets for the same allocation
count, and is also excluded from `FBL525`; indexed use of its returned pointer
continues to be checked normally.

`FBL610` and `FBL-CF-001` track unreachable code only within one local-flow
region. `End Function`, `End Sub`, `End Property`, `End Constructor`, `End
Destructor`, and `End Operator` close that region before the next declaration
or module-level statement is reviewed. This prevents a getter's valid `Return`
from being carried into later code, while preserving diagnostics for code that
actually follows a terminator in the same routine. The linter regression suite
contains a property getter followed by a module-level declaration for this
boundary.

`FBL-PTR-001` recognizes a routine-level null guard in the form `If p = 0
Then ... Return ... End If`, including the parenthesized spelling used by older
examples. After that guard, a later `*p` is safe until `p` is reassigned. The
proof deliberately does not cross an enclosing conditional, so a nested return
guard still reports a possible null dereference outside that conditional.

`FBL-PTR-001` and `FBL-PTR-002` also accept the exact branch forms `If p <> 0
Then` and `ElseIf p <> 0 Then`, including the established parenthesized
spelling. That proof exists only inside its active branch. The linter drops it
at `Else`, the matching `End If`, or an assignment to `p`; an unchecked path
therefore remains reportable. Safe and unsafe regression fixtures exercise the
normal branch, the `Else` boundary, and reassignment.

The bzip2 compression example uses caller-owned input and output buffers. It
validates the input file size and every allocation, keeps the old output buffer
until `Reallocate` succeeds, bounds output growth before arithmetic can
overflow, checks bzip2 stream-status results, and clears each pointer after
release. Empty inputs are accepted without a null-pointer dereference.

The libpng image-loader example checks graphics-mode setup before drawing and
keeps its file, libpng structures, row buffer, and partially created image
local until success. A shared cleanup helper releases and clears each resource
on every failed read; only the completed image is transferred to the caller.
The documented libpng error-message contract and bounded row-conversion loops
use narrow suppressions because their safety is established by the external API
and loop allocation relationship, not by a general pointer proof.

The libjpeg loader follows the same local-ownership model for its file,
decoder, one-row sample array, and incomplete image. It validates the graphics
mode, row dimensions, each row allocation, and image creation before pixel
access, while its caller rejects a null image before drawing. The fblite path
uses `Long` for `IMAGEINFO`'s legacy pitch output, preserving its 32-bit API
contract on a native-width host.

The manual GMP example checks the separately allocated GMP number and decimal
string before either is used. It clears the GMP value only after initialization,
then releases and nulls both owned pointers on their successful allocation
paths. This retains a concise library demonstration while making its allocation
contract explicit.

The manual `CAST` example retains its dereferenced-type helper macros but checks
both demonstration allocations first. Its `New`-allocated `Double` uses
`Delete`; its `Callocate`-allocated ZSTRING uses `Deallocate`. Each pointer is
cleared after release, making the two allocation families and their matching
ownership rules visible in the example itself.

The manual ZSTRING and WSTRING examples retain their fixed allocations for the
12-character `"hello, world"` value, including the ZSTRING terminator. Both now
check the buffer before dereferencing it, release it after displaying the value,
and clear the pointer. The failure branch therefore cannot reach a string
write through a null pointer.

The manual `WITH` example checks the five-element `rect_type` allocation before
entering its dereferenced `WITH` block. It frees and clears the rectangle-array
pointer after the loop, so the simple member-access demonstration has no
unowned dynamic storage.

The manual `DIM` example checks the dynamic ZSTRING allocation before assigning
the variable-length text. Its usual size output remains unchanged on success,
and the owned buffer is released and cleared after the final `Len(*s3)` use.

The NeHe BMP loader represents the on-disk file and DIB headers with their
specified 14-byte and 40-byte fixed-width fields. This keeps binary `Get #`
transfers independent of the host's native `Integer` width. The companion TGA
selector header and the manual `FIELD` example document the byte ranges that
their raw reads consume. `FIELD` obtains a checked `FreeFile` unit and prints
the dimensions only after its BMP header transfer succeeds; its one raw-format
linter suppression is tied directly to that documented transfer.

The compressed TGA loader owns its per-pixel scratch buffer until decode
completion. It rejects a failed scratch allocation before use, clears the
partially created texture image on a decode failure, and releases the scratch
buffer on the normal return.

The GTK text viewer and the manual whole-file text examples accept files up to
4 MiB. They measure `LOF` once into a `LongInt` before allocating the string,
retain the empty-file result, and handle oversize or failed reads without
attempting an unbounded allocation.

The manual `GET` example passes its caller-owned binary handle explicitly to
each scalar, array, and raw-buffer demonstration. Its temporary five-element
buffer is dereferenced only after allocation succeeds and is released and
cleared before the subroutine returns.

The manual `PUT` buffer example writes one documented byte record only after
its 256-byte buffer and `FreeFile` output handle are available. It closes the
file before clearing the buffer ownership and reports a failed open or write.

The manual UDT `GET` and `PUT` examples describe their 40-byte tutorial record
as a same-target FreeBASIC format: 32 fixed string bytes followed by one
`Double`. Each Save or Load owns its checked file handle only after opening it;
the record-layout and fixed-string linter suppressions cover exactly the
documented transfer and are not a portability claim.

The manual `RANDOM` examples use the same approach for their 11-byte length
and fixed-string record and their 24-byte fixed-name and `Single` score record.
Each tutorial format is same-target only, and each read/write pass uses a new
checked `FreeFile` handle.

The manual formatted-file examples use an `Integer` `FreeFile` handle for each
OUTPUT, INPUT, and APPEND pass. A read phase runs only after its write phase
succeeds. Their documented `FBL517` suppressions are restricted to the
`Write #` and `Input #` statements being demonstrated, so unreviewed formatted
file operations remain visible to strict linting.

The manual `FILEFLUSH` example uses a checked output stream and only acquires
its input stream after output opened; it retains the before/after visibility
sequence while closing the input before the owning output handle. The FREEFILE
examples obtain a new checked number for every independent stream and document
their one raw string transfer.

The manual encoding sample uses separately owned, checked handles for output,
byte inspection, and input. Its inspection buffer is capped at four mebibytes,
and its raw transfer is documented as the bytes written by the preceding
UTF-16 output step. The access example makes a checked same-target byte copy
only after its input length is positive, then opens its output stream.

The console and printer examples retain their explicit `CONS` and `LPT`
devices, checking `Err` immediately because those device forms are statement
syntax rather than `OPEN(...)` expressions. Their path-oriented lint
suppressions apply only to those device statements. Named PRINT, RESET, and
encoding fixtures remain inspectable manual output, with similarly narrow
path suppressions. The intentionally bad FREEFILE page continues to request
two numbers before opening either, but checks the expected collision and
closes the sole owned handle.

The remaining manual file-I/O pages use the same ownership rule for `PIPE`,
`ERR`, `SCRN`, `COM`, and `LPT` device handles, plus ordinary binary paths.
The continued printer form is statement-only and checks `Err` immediately;
the narrowly documented lint suppression accounts for the scanner's inability
to pair that continued form with its check. Raw Integer and array transfers
state their same-target record contract, and WRITE-record suppressions cover
only the three statements that demonstrate formatted records.

The FILEATTR example opens its named fixture through a checked `FreeFile`
handle before obtaining the corresponding CRT `FILE` pointer. FILESETEOF uses
a separate checked handle for creation, extension, and truncation, so no later
resize acts on a failed open. The BSAVE examples check `ScreenRes` before
drawing; the buffered version also checks `ImageCreate` before `GET`, `BSAVE`,
and `ImageDestroy`, and uses real division for coordinates combined with
non-integral circle aspects.

The CVA_ARG and VA_FIRST examples verify that the format-string pointer exists
before walking it and use a remaining-character counter to make the scan
boundary clear. The VA_FIRST family remains backend-specific: it is validated
with the native `gas64` generator because the C-family generators do not
support those ABI intrinsics.

The procedure-pointer vtable examples now test an object allocation before
using it. Their virtual-procedure helper selects the vtable branch only when
the compiler reports a non-negative virtual index; otherwise it uses the
ordinary procedure pointer. This retains the member-signature demonstrations
without unbounded raw pointer-index expressions at every call site.

The overloaded NEW example checks its aligned allocation before computing the
returned address. It uses byte-pointer arithmetic for the alignment layout and
documents the sole preceding pointer-sized header used to retain the original
allocation for the overloaded DELETE operation. Both the temporary original
pointer and the caller-owned result are cleared after release.

The SELECT CASE speed benchmark documents each scalar that follows the two
non-overlapping ranges ending at 30. Its narrow range-order suppressions apply
only to those six scalar cases, preserving a warning for an unreviewed range
ordering elsewhere. The linter's simple duplicate tracker also skips selectors
that use concatenation after literal stripping, because distinct `EXTCHAR &`
key sequences cannot be compared reliably in that reduced form.

The ERR, BLOAD/BSAVE workaround, AS, and error-handling pages use local
`FreeFile` numbers and close successful opens. The raw compatibility wrappers
reject a null caller buffer and a non-positive write length before calling the
runtime I/O functions; the caller remains responsible for sufficient BLOAD
storage. The QB ON ERROR page documents the handler scope at its declaration,
where its narrow lint suppression preserves that legacy teaching example.

The ON ERROR, ERR, ERL, RESUME, and RESUME NEXT pages document their legacy
handler scope, handler labels, `Err` reads, and resume semantics at the exact
statements concerned. Their suppressions are deliberately limited to those
features. The dynamic-array sizing page similarly marks the unsized metadata
queries, one-based dimension numbering, and the sizing query after `Erase` as
intentional parts of that tutorial.

The CUSTOM, SCREENCONTROL, and regulated-animation examples check `ScreenRes`
before graphics work. CUSTOM also checks its image allocation, seeds its
dither once, names the three threshold positions, and documents its keyboard
poll pacing. The window-shake example similarly seeds its motion and documents
its non-blocking event-loop sleep. The regulated-animation example separates
initialized state, uses `phase` for its fractional animation angle, and skips
the render loop when graphics setup fails.

The regulated point-cloud animation likewise stops on an unavailable graphics
mode or an empty cloud. It allocates the fixed cloud once before filling it,
then uses the returned element count for its rotation, sort, and draw loops.
The perspective helper returns the original point for a near-zero eye depth and
clamps a near-zero projection denominator with its sign intact. Its Z-axis now
uses the Z coordinate for its normal 360-degree wrap.

The error-handling comparison retains its four distinct OPEN strategies:
unchecked legacy behavior, a returned result, an ON ERROR handler, and both
combined. It closes unit 1 only after a successful open and documents the
literal-unit, handler, and handler-label exceptions on the exact relevant
statements. A failed open therefore never leaves an owned handle.

The dynamic-memory tutorial keeps all four allocation families visible. Its
manual `Callocate` objects are constructed and destructed explicitly, and the
original pointer remains owned until `Reallocate` returns a replacement.
`New[]` pairs with `Delete[]`, while placement `New` uses exactly three
UDT-sized array slots and manually destructs them before `Erase`. The short
index-appending loops remain part of the teaching sequence and are documented
as fixed-size work rather than general string-building advice.

The overloaded-NEW manager tracks every successful `New[]` result in matching
pointer and byte-count tables. Its deletion path searches the actual table
bounds, then reduces the live table through one helper after compaction.
When its intentional memory limit is reached, its cleanup routine releases all
remaining tracked allocations before ending the example.

The NEW/DELETE operator page reports an allocation failure before the caller
uses the returned pointer, and clears each ordinary owner after its matching
delete operation. Its placement buffer has exactly two UDT-sized slots. The
one-object and two-object placement forms run their destructors explicitly and
never attempt to delete memory owned by that buffer.

The RTTI information pages check their object and mangled-name pointers before
they use the observed compiler layout. Their layout-specific object, base-link,
and name slots are documented immediately beside the raw accesses. These pages
remain demonstrations of generated data rather than a public ABI, and follow
base links only while the requested base index is negative.

The manual sample builder keeps explicit active counts distinct from dynamic
array capacity. Its special-build, directory, and source-file tables grow
geometrically, preserving the recursive scanner's existing active-entry loops.
It normalizes both ends of INI keys and values in one parser helper, and calls
the selected compiler through a clearly named path whose `Exec` result remains
checked by the existing build-result flow.

The dynamic-object lifetime page gives its UDT one ZSTRING owner. Its private
replacement helper allocates and copies text before releasing prior storage, so
failed allocation and self-aliasing assignment preserve the old value. A manual
destructor clears the pointer; subsequent display methods identify the released
state rather than dereferencing it, and the caller keeps the `New` result as
the owner even while using a ByRef alias.

The TLS-emulation page reserves slot zero in each generated table as a
sentinel, with positive indices representing live thread values. Its old-runtime
branch locks ThreadSelf refresh, use, and destruction together. The object move
used during table compaction now checks its temporary raw allocation, while the
macro-generated accessor declaration makes the resulting TLS namespace visible
to static analysis.

The recursion-to-iteration quicksort page uses one explicitly zero-based,
100-byte shared demonstration buffer. Both the recursive and explicit-stack
forms receive its documented `0` through `99` bounds, making the identical
sorting range clear without treating a fixed array as a potentially empty
dynamic input.

The EXTENDS ZSTRING page gives its derived value a single checked CAllocate
owner. Larger replacement text is copied before the old buffer is released, so
growth failure and aliasing retain the existing value. A null construction or
assignment value represents empty text, and the SELECT CASE comparison values
are constructed before the case list.

The ERASE bounds page deliberately prints both fixed-array and dynamic-array
bounds before and after ERASE. Its documented, line-specific exceptions retain
that comparison rather than treating the post-ERASE query as accidental use of
an old dynamic-array range.

The MUTEXCREATE and CONDCREATE circle pages document that ThreadUDT retains its
worker handle and synchronization objects until the thread has stopped, while
the Point2D remains live until then. They seed generated points once and use
explicit integer center coordinates; each callback member is documented as a
procedure-pointer type, not array syntax.

The critical-section display FAQs document one mutex around saved display state
where the page uses one. Their worker arguments represent only the encoded
values one through nine, and are converted through a pointer-sized integer
before they are used as coordinates, colors, delays, or displayed labels.

The frame-regulation include is guarded against repeated inclusion. Its Windows
time-period declarations identify the 32-bit StdCall API boundary, and its
standard and high-resolution regulators return zero for an invalid zero-FPS
request. The frame-rate helper similarly returns zero when the timer has not
advanced instead of dividing by a zero interval.

The integer-division page retains two non-integer divisor cases specifically to
show how `\` first converts its operands. The accompanying annotations apply
only to those expressions, preserving ordinary review of unintended mixed
integer-division operands.

The Operator Is page now rejects a null Object pointer before any RTTI query or
derived-type member access. It retains the same unknown-object report used for
a non-Vehicle instance.

The manual `Operator Let` example raises the standard out-of-memory runtime
error when its owned ZSTRING allocation cannot be made. Assignment first owns a
replacement copy, then releases the old value, so a failed replacement cannot
leave the existing object with a null string pointer.

The console `LINE INPUT` sample documents that `maxlength` includes the
terminating null byte. It checks its ZSTRING allocation before passing the
dereferenced buffer to the bounded input overload, and releases and clears it
after the input is displayed.

The file `LINE INPUT #` sample obtains a fresh `FreeFile` unit for every
operation, checks the result of every open and transfer, and never reads the
fixture after its write failed. Its bounded ZSTRING buffer is checked, released,
and cleared. Two narrow suppressions retain the intentional named `myfile.txt`
fixture and FreeBASIC's immediate `Err` status reads after `Close`; they do not
disable the allocation or file-handle rules.

`FBL-STR-002` no longer treats every `CAllocate(..., SizeOf(ZString))` call as
an undersized buffer. It reports a locally provable `CAllocate(Len(text),
SizeOf(ZString))` form that lacks terminator space. `FBL-STR-003` now scans a
dereferenced `LINE INPUT` target and reports only when it has no trailing
`maxlength` argument. Dedicated safe and unsafe fixtures cover both boundaries.

`FBL804` remains an object-safety warning for the simple form `Clear name`.
The linter excludes raw-storage forms that carry an explicit value and length,
such as `Clear *p, 0, bytes` and indexed elements, plus function-style and enum
uses. This preserves warnings for managed objects without misclassifying a
bounded byte-buffer operation.

The public graphics APIs are implemented by both gfxlib2 and gfxlib3 and are
included in their normal and multithreaded archives. The string and pathname
services are included in the applicable normal, MT, and PIC runtime archives.

The Win32 text drop-target example validates its COM result-pointer contract
before writing through it, compares requested interface identifiers with the
Windows field-wise GUID helper, and leaves the result pointer null on failure.
Its text transfer keeps `ReleaseStgMedium` on every successful `GetData` path;
`GlobalLock` is now checked before the returned text pointer is used or
unlocked.

The Win32 tree-view example identifies its GUI-thread module state, clears its
raw Win32 insertion structure with `SizeOf`, and separates each local type
declaration. Its text string remains alive through each synchronous
`TVM_INSERTITEM` message, while the class name remains alive through
`RegisterClass`; the source documents those API ownership boundaries beside the
corresponding narrow lifetime-review exceptions.

The lint project index records initialized `TYPE` fields by their declarator
rather than their initializer token, so implicit FreeBASIC member access stays
visible to declaration checks. Its return-address analysis likewise recognizes
that `@pointer->member` identifies caller-owned pointee storage, not the local
pointer variable itself.

Expression-oriented lint rules skip `DECLARE` routine headers. In particular,
operator overload declarations that contain `/`, `*`, `<`, or mixed signed and
unsigned parameter types describe an interface; they do not evaluate an
arithmetic expression or compare values.

The Allegro starfield uses explicit zero-based bounds for every fixed render
array, names its local face pointer distinctly from the `FACE` type, and checks
the bitmap created for the offscreen renderer before use. The example documents
its two single-threaded shared-state groups. The linter recognizes Allegro's
documented `palette_color` external pointer without requiring a local shadow
declaration.

The compact OpenGL drawing helper now has consistent continuation indentation
and disables blending for an invalid blend-mode value, leaving the renderer in
a known state rather than retaining an earlier mode. The lint project index
also accepts tab-aligned `TYPE` field declarations before locating their `AS`
clause, so private members remain visible to declaration checks regardless of
that established layout style. Regression coverage includes an initialized
tab-aligned field.

The SDL/OpenGL quadrics sample gives each fixed OpenGL parameter vector an
explicit zero-based bound. Its generated texture likewise declares its three
zero-based dimensions, then traverses the dynamic array through `LBound` and
`UBound` so the producer matches the allocation. The texture and quadric
handles are documented as state owned by the single SDL event loop, and the
mixed continuation indentation has been made consistent without altering the
rendering sequence.

The NeHe bump-map lesson gives its OpenGL vector and matrix scratch arrays
explicit bounds, records the GLFW main loop as owner of its shared render
state, and retains its optional Windows extension dialogs behind documented
conditional-only lint exceptions. Logo loading now keeps separate alpha and
colour BMP owners until each transfer is complete, rejects missing,
dimension-mismatched, oversized, or unallocatable RGBA input, and releases all
temporary image and alpha storage through one helper cleanup path.

The NeHe grid game documents its single-loop state boundary and gives all
shared arrays explicit zero-based bounds. Its dynamic texture transfer buffer
has a stated nonempty-bound check before its `LBound` address is passed to
`BLoad` and the texture loader. The font-grid row calculation separates its
integer cell selection from its floating texture scale, preserving the
original coordinates while making the intended arithmetic clear.

The TinyPTC lens demo documents that its display loop is the sole owner of its
shared tables, makes the fixed table bounds explicit, and separates the output
array parameter from the module framebuffer name. Its distance helper rejects
empty or mismatched coordinate arrays before indexing them, then uses their
declared lower and upper bounds; its former mixed indentation and excessive
blank spacing are normalized without changing the lens calculation.

The Direct3D primitive sample now names each source vertex count and uses
`SizeOf(Vertex)` for every Direct3D lock, buffer, and `memcpy` byte count. This
keeps the API's byte-oriented contract explicit and corrects the triangle-list
upload that could exceed its six vertices and the triangle-strip upload that
could omit two required vertices. Its raw Direct3D presentation structure is
cleared using `SizeOf`, and the idle-loop delay is documented as queue-empty
render pacing.

The zlib usage sample documents ownership of its standard-error unit, buffers,
and initialized zlib streams. Its preset-dictionary calls now pass the string's
actual byte length rather than the size of its pointer, which is required on
64-bit targets. Main cleanup clears released buffer owners and closes the
successfully opened error unit; its complete compression, gzip, flush, sync,
and dictionary sequence is run as a native smoke test.

The NeHe masking lesson names its display, viewport, and camera constants;
sets the five-texture arrays to their exact zero-based range; and expresses
key-edge tests and toggles as Boolean comparisons. Its continuation indentation
is normalized without changing the two rendering scenes or texture uploads.

The NeHe Bezier lesson identifies the main render loop as the owner of its
patch and display list, checks its dynamic bitmap buffer before use, and keeps
the current list when a replacement cannot be generated. Bezier generation
now rejects invalid or unrepresentable divisions, checks display-list and
point-array allocation results, frees a failed new list, and clears the point
array owner after release. The bitmap transfer's immediate `Err` contract is
documented beside its narrow analysis exception.

The NeHe world-walk lesson documents ownership of its parsed triangle array,
checks the triangle count and allocation before filling it, and releases and
clears the sector at normal exit. Its local texture transfer checks `BLoad`
before creating textures, while its key-edge handling uses explicit Boolean
state comparisons and the established error-status boundary is documented.

The TinyPTC torus demo documents its single display-loop state, seeds its
random starting angles, and advances each rotation axis from its own value.
Its reusable pixel and smoothing helpers no longer shadow the framebuffer;
the smoothing path rejects empty or wrong-sized target arrays before applying
its screen-neighbour algorithm and uses their actual bounds.

The TinyPTC Julia-rings demo gives all four fixed tables explicit zero-based
bounds and documents that its one display loop owns the TinyPTC session. It
initializes the per-pixel iteration selector before its colour permutation, so
pixels outside the ring no longer inherit a prior pixel's selector.

The standalone OpenGL texture sample documents ownership of its shared terrain
texture and rejects a failed texture load before rendering. It ends naturally
after its module data declarations, avoiding an unreachable-data terminator.
Its texture loader validates positive, representable dimensions before it
allocates a zero-based transfer array, checks the `BLoad` result, and passes
the array's documented lower bound to the texture creation helper.

The NeHe particle lesson documents ownership of the SDL surface, immutable
palette, and particle array across its one SDL event loop. It seeds the random
particle state before either initialization or reset uses `Rnd`, and its image
upload and gravity-adjustment continuations use consistent nesting without
altering the OpenGL texture data.

The NeHe raw-image blitter lesson now documents the headerless RGB source-file
contract and owns each temporary image through setup failure and successful
OpenGL upload. Its checked dimensions, allocation arithmetic, raw reads,
blit rectangles, texture creation, and texture release preserve the original
128 by 128 RGBA demonstration while preventing failed setup from reaching a
pointer dereference or an out-of-range copy.

The NeHe texture-filter and fog lesson gives its bitmap transfer an explicit
zero-based array contract, checks both allocation shape and `BLoad` status,
and uses the actual array lower bound when creating each texture. It releases
all three texture names after normal rendering and after a partial creation
failure, while key-release tests use explicit zero comparisons.

The NeHe display-list font lesson now checks each bitmap load before texture
creation, validates the temporary array bounds, and releases its two texture
names and font lists on every initialized exit. Its module-only shared OpenGL
names are documented, while the 16-glyph font-row calculation remains an
explicitly documented integer division.

The NeHe GLU quadric lesson now validates its bitmap-transfer storage and
`BLoad` result before creating textures, checks that GLU creates the quadric,
and releases the quadric and all three textures after rendering or a partial
initialization failure. Its key-release state tests use explicit zero values.

The NeHe height-map lesson documents its exact headerless terrain-byte layout,
uses a fixed zero-based array with that exact capacity, and checks a caller's
array size, binary open, and raw transfer before rendering. The loader now
returns failure to module scope instead of terminating inside a routine, while
the module-owned terrain state and explicit render-toggle release are clear.

The NeHe texture-filter and blending lesson documents module-local OpenGL
state, checks all three generated texture names and the mipmap build before
success, and deletes any partial names when setup fails. Its decoded bitmap is
always released after transfer, successful texture names are deleted at normal
exit, and keyboard-release checks use explicit zero comparisons.

The GLUT OpenGL sample declares its three fixed light vectors with explicit
zero-based bounds and documents the GLUT-managed callback model. Reshape and
keyboard callbacks now use 32-bit `Long` C parameters, matching the GLUT
binding ABI on 64-bit hosts as well as 32-bit hosts.

The NeHe TGA font lesson documents its shared texture and display-list scope,
rejects an unavailable texture name or font-list base, and releases the
initialized OpenGL objects on every exit. Its fixed byte-header comparison is
explicitly documented as padding-free, and the 16-glyph font-row calculation
remains intentional integer division.

The NeHe radial-blur lesson validates the CPU texture allocation and generated
OpenGL name before use, allows initialization to return failure to module
scope, and documents the shared helix and blur-texture ownership. It rejects
zero blur passes before division and uses a small near-zero normal threshold
to avoid unstable normalization.

The SDL OpenGL sample documents SDL lifecycle ownership, requests its OpenGL
colour, depth, and double-buffer attributes before context creation, and uses
explicit zero-based light-vector bounds. The SDL video surface remains owned
by SDL until the display loop calls `SDL_Quit`.

The general array example now gives every fixed array explicit bounds, checks
that dynamic arrays are non-empty before asking for their bounds, and has
array-parameter helpers that safely ignore empty inputs. It keeps the dynamic
array alive until its final demonstration use, then erases it; the fixed UDT
field bounds are documented at the narrowly suppressed static-array check.

The Win32 browser-control example documents COM and browser ownership, splits
initialized declarations for readable message and toolbar setup, and gives the
disabled toolbar edit control a declared command identifier. Its local browser
owners no longer shadow the module browser object, and continuation indentation
is normalized without changing the browser or toolbar contract.

The GooCanvas grid example now expresses its displayed phi, pi, and middle-dot
symbols as ASCII XML numeric character references. Pango resolves the same
glyphs at markup render time while the source remains portable across editors
and compiler input encodings; its GTK widget ownership is documented.

The GTK3 custom-widget example documents the GObject and private-data ownership
sequence, gives its fixed 100-pixel box an explicit half-width, and identifies
the two parent-class references generated by `G_DEFINE_TYPE`. All overridden
GtkWidget and GObject virtual methods now use the C callback convention that
the GTK class structures require.

The GTK calculator documents that GTK's single main thread owns its display
state across generated button callbacks. Its GladeToBac implementation fragment
remains intentionally included in the executable module so generated widget
declarations and signal handlers retain their original linkage. Division keeps
the calculator's exact-zero input rule, which permits nonzero IEEE values that
users enter, including very small divisors.

The Win32 file-open example documents the dialog and menu ownership boundaries,
keeps the embedded-NUL OPENFILENAME filter in a local FreeBASIC string through
the synchronous dialog call, and keeps its class name in fixed zstring storage.
It also checks that window creation succeeded before showing or updating the
window and normalizes its continuation layout.

The two-connection OPEN TCP self-test documents ownership of the listener,
client, and accepted file handles, and gives each local socket a five-second
timeout so its line reads cannot wait indefinitely. It continues to verify two
different server messages with an actual localhost run.

The QB/FBlite `DIM` manual example retains its five type-suffix declarations
because teaching those compatibility spellings is its whole purpose. Each
strict-lint exception is narrowly documented beside the corresponding example
line, so ordinary modern declarations remain covered by the declaration rule.

The FBlite `OPTION BASE` manual example likewise retains its deliberately late
option statement and the surrounding implicit-bound declarations. Those exact
lines demonstrate that the option changes later array declarations, so their
small lint exceptions are documented rather than modernizing away the lesson.

The GFX_NULL manual example documents its one module-owned framebuffer and the
Win32 paint lifecycle that presents it. Its animated rectangles are now seeded
before `Rnd` use; its only startup `End` remains a documented module-level
failure path after window-class registration fails.

The `crt/setjmp.bi` binding now matches MinGW-w64 x86_64's two-argument
`_setjmp` ABI while retaining the historical one-argument FreeBASIC call form.
The optional frame argument uses the same null fallback as non-C compiler
backends; compiler-generated GOSUB code continues to provide a caller frame on
C backends where it is available.

The try/catch demonstration documents its process-local signal context and the
intentional null-pointer fault paths used to exercise it. Exception text copies
are allocated before replacing their owners, null exception inputs are rejected
before dereference, and repeated file/function assignments no longer leak the
previous copy. The Win64 smoke now traverses nested throws and signal catches
through normal process exit.

The strict linter recognizes type-first `Dim Shared ByRef As T name`
declarations in both its file parser and project index, so a valid shared
reference is available to later assignments and included source. The shared
library manual now documents that its caller-owned reference must outlive its
exported use, while its separate DLL-owned integer remains explicitly scoped to
the return-by-reference interface.

The AROS primary-colour smoke now keeps its framebuffer pointer check inside
one lock/unlock ownership region, so every path releases the gfxlib2 lock once.
Its fixed `GfxSmoke:` marker path remains the test runner's rendezvous file,
but a failed creation restores text mode and terminates instead of writing to an
unopened unit.

The FBlite non-escaped-literal manual intentionally changes `Option Escape`
after its first group of strings. Its exact strict-lint annotations preserve
that before-and-after comparison, including the distinct default, `!`, and `$`
literal forms; the native run confirms the documented backslash output.

The array-parameter manual now counts delimiter occurrences before one exact
`ReDim Preserve`, retaining any existing destination range without repeated
copying as each fragment is found. Its result display and the reusable splitter
both distinguish an empty dynamic array before asking for usable bounds.

The legacy procedure-local error-handling manual retains its `-e` requirement,
one `On Local Error` scope, and `Err`/`Erfn` reporting label. Its strict-lint
annotations are limited to those compatibility constructs, and the source is
compiled with `-e` as documented.

The regulated-graphics manual declares WinMM's `timeBeginPeriod` and
`timeEndPeriod` with explicit C calling convention and 32-bit `ULONG` argument
and result types. That preserves the same timer-resolution calls on Win32 and
Win64 without relying on native-size defaults.

The critical-section FAQ now states that its shared point and quit flag are
intentionally unsynchronized to expose missed and duplicate display points.
The nearby mutex calls remain the documented corrected variant, and the worker
sequence is seeded once before it starts.

The polymorphic `New[]`/`Delete[]` manual now reports a failed Cat or Dog custom
allocation before it can be used. It documents that each successful allocation
is released through the virtual launcher, and that its two-element allocations
are exactly the zero and one indices used to demonstrate derived indexing.

The smart-pointer macro manual now rejects a null root pointer before RTTI
access and declares its three-object smart-pointer array explicitly zero-based.
The three annotated RTTI dereferences are protected by that unchanged routine
parameter guard.

The recursion manual makes the fixed shared work array explicitly zero-based,
separates the initialized local sorting state, and establishes that a bound
pair is usable before the first sort call. The ZSTRING-chain manual now treats
its member as owned nullable storage: negative requested sizes leave it empty,
copy construction handles an empty source and allocation failure, and
assignment allocates a replacement before releasing the previous value.

The FBlite `OPTION DYNAMIC`, `OPTION STATIC`, and `OPTION ESCAPE` manuals keep
the deliberate option-order and literal demonstrations but state those exact
lint exceptions beside them. Dynamic-array initialization now uses its actual
bounds. The Windows `IsRedirected` manual derives and quotes both child paths
from its own executable, gives `start` its required empty title, and checks the
shell-launch result; its remaining narrow security annotation documents the
intentional command-interpreter example.

The strict linter now treats `Constructor`, `Destructor`, `Operator`, and
`Property` as language keywords during undeclared-name analysis. Valid
`Exit Constructor` and `Exit Operator` statements therefore remain visible as
control-flow syntax rather than producing false undeclared-identifier reports.

The VARIANT operator implementations now initialize every COM output `VARIANT_`
and temporary shift operand before passing it to an Automation function. Ordinary
operators return a valid empty variant if Automation fails, while compound
operators retain their prior value until a new result is available. The shared
primitive-operand macros follow the same result-lifetime rule. The strict linter
similarly recognizes `VariantInit` as initializing its addressed output
argument, with a regression boundary for later reads.

The sfxlib composer now documents the event-loop ownership of its note grid,
file units, and sound configuration, rejects a failed display before drawing,
and cancels an unrecognized file-entry mode. The MIDI synth player documents
its two parser/render-state groups, names the big-endian byte shifts, and widens
the sample rate before calculating a requested render length.

The libzip extraction example now treats archive names and per-entry input
handles as nullable libzip outputs. It reports and skips a malformed unnamed
entry, closes an already-open output unit if its input entry cannot open, and
guards the mutable parent-directory path before dereferencing it.

The graphics-input demo now gives its two fixed scratch arrays explicit lower
bounds and separates the initialized mouse-visibility flag. The PostgreSQL
example obtains optional connection settings from `FBC_PG_CONNINFO`, retains a
non-secret local default, and rejects failed connection or query handles before
they are passed to libpq. The ERASE and FBARRAY manuals retain their empty-array
and raw-descriptor demonstrations with narrow annotations that describe those
intentional compatibility boundaries.

The strict linter now recognizes `COLOR` as a FreeBASIC statement keyword, so
ordinary graphics colour changes do not produce an undeclared-identifier
diagnostic. The focused regression fixture preserves that parser boundary.

The DATA and READ manuals rely on normal module completion before their data
blocks rather than redundant `End` statements, and their five-element fixed
arrays state the zero lower bound. The RESTORE manual makes both fixed ranges
explicit and removes surplus blank records. The `__FB_LANG__` manual retains
its conditional non-fb `Option Explicit` demonstration with exact annotations.

The fbgl anti-aliased-line example seeds its randomized line colours once
before its draw loop and normalizes the two formerly mixed-indentation blocks.

The ERFN and ERMN manuals retain their small FBlite `On Error`/`Resume Next`
examples with a single annotation describing the required `-exx` build and
synthetic error boundary. The constant-expression manual converts its two
floating geometric values explicitly and keeps the sample value module-local.
The simple polymorphic New/Delete manual documents allocation ownership, makes
failed custom allocation visible, and releases earlier successful objects if a
later allocation fails.

The static-library varZstring implementation now has an explicit nullable
buffer ownership policy, returns a valid empty reference after an allocation
failure, and uses allocate-before-release assignment. The VIEW PRINT graphics
manual seeds its colour choice and uses separate nested `Next` statements. The
condition variable manual documents the mutex/condition ordering around its
shared state.

The composition/aggregation/inheritance manual now uses portable ASCII terms,
initializes its non-owning vehicle pointer, and reports a failed vehicle
allocation before the aggregation is dereferenced.

The manual libzip extractor now has the same nullable archive-name, entry-file,
and mutable-path safeguards as the standalone compression example. The libffi
closure manual uses a checked console stream before it binds that file unit as
callback user data, writes its string with `Print #`, and keeps the documented
console-stream lint exception separate from normal disk output paths.

The two cryptlib hash samples now retain their context ownership on every
failure path, use pointer-width file positions while bounding cryptlib's
32-bit block length, check each cryptlib status, and render every digest byte
as two hexadecimal characters in preallocated output. The second libffi
closure sample now uses the same checked console-stream and closure-lifetime
rules as the manual version.

The strict linter identifies source marked as an extracted FreeBASIC Manual
example before applying the broad shared-state boundary reminder. This keeps
the reminder active for ordinary application code while allowing a manual
sample to demonstrate `Shared` without a misleading warning.

The GDI+ circle and framework samples now initialize Win32 and GDI+ output
objects, reject failed gradient and graphics-context creation before use, and
document their single GUI-thread shared state. The WIDTH manual reports a
failed graphics-mode request before stopping, and fixed manual arrays show
their zero lower bounds directly. SDL cursor creation now rejects invalid
packed dimensions before allocating its masks; the SDL line demo seeds its
random generator once before drawing.

The CUnit example now uses a C-runtime anonymous temporary stream instead of a
predictable filename, matches its `fprintf` argument types to the C ABI, and
documents the suite-owned stream lifetime. The UDT byte-array formatter checks
its captured bounds before allocating or indexing. Procedure-pointer
declarations with a space before their parameter list are now recognized by
the strict linter as procedure pointers rather than implicit arrays.

The BLOAD, COLOR, PSET, POINT, FLIP, DRAW, DRAW STRING, GET, IMAGECONVERTROW,
IMAGECREATE, IMAGEINFO, LINE style, PAINT, PCOPY, PIXELPTR, PMAP, PUT, RGB,
RGBA, SCREEN Function, SCREENCONTROL, SCREENEVENT, SCREENGLPROC, SCREENLOCK,
SCREENRES, and SCREENSET graphics manuals now stop before drawing when
`ScreenRes` cannot select their requested mode. The BLOAD and GET image
examples also stop on image allocation or file-load failure, the
IMAGECONVERTROW sample cleans up both image owners after a later setup failure,
and the RGBA, custom DRAW STRING, and PUT manuals stop before pixel writes if
`ImageCreate` cannot allocate their demonstration images. Multi-image PUT
samples also release already-created sprites before reporting a later
allocation failure. SCREENEVENT documents its short polling pause so it yields
while no event is queued. GETMOUSE, MULTIKEY, and SETMOUSE likewise reject a
failed graphics-mode request; their documented polling delays yield while the
examples wait for input rather than busy-spinning.

The gfxlib image-header FAQ checks both its graphics mode and its working
image before displaying the header details.

The FreeType character examples now initialize their library and face handles
to empty values, release each successfully acquired handle after every later
failure or final display, and reject a failed graphics mode before traversing
the rendered glyph bitmap.

The FreeImage loaders now validate their vertical flip, 32-bit conversion,
dimensions, FreeBASIC image allocation, and source-bit pointer before a row
copy. Each failure releases every already-owned FreeImage object; the general
loader also verifies its display setup and does not destroy an empty result.

The GIF loader checks its display setup, decoder steps, frame dimensions, color
map, palette count, raster pointer, and color indices before drawing. Its one
close helper releases the GIF decoder on every early failure and normal return,
and partial image output is released before a malformed palette index returns.

The GTK version checker now reports a failed GTK initialization without trying
to display its fallback prompt in an unavailable graphics mode.

Manual array, control-flow, graphics, operator, procedure, iterator,
recursion, reference, UDT, and scope examples now keep independently
initialized values in separate declarations. The two samples whose lesson is a
colon-separated one-line sequence retain it with an exact annotation. The
member Rational operator also returns a defined value for a public
zero-denominator instance rather than performing a division by zero.

The DOS, OpenGL, Win32, JIT, and COM examples also separate independently
initialized values. NeHe's morphing-model loader now gives the `sscanf` vertex
count a 32-bit `Long`, matching C's `%d` ABI on 64-bit hosts, and documents the
scene-buffer ownership and its intentional module-level render state.

Array-bound manuals now distinguish fixed non-empty initializer tables from
dynamic input. The dimension and function examples capture bounds and return a
zero count for an empty dynamic array; exact annotations preserve examples
whose purpose is to show the empty or fixed-bound convention. The Java JNI
example derives its fixed option count from storage size, and the TLS example
captures and tests its dynamic-array bounds before traversing or extending it.

The strict linter now distinguishes array bounds from parentheses in `TypeOf`,
fixed STRING/ZSTRING/WSTRING type expressions, generic type macros, and
generated identifier macros. Its focused fixture and complete proposed and
recurring rule suites protect that boundary while ordinary implicit arrays
remain diagnosed.

Pointer and comparison manuals now state the zero lower bound of their fixed
example arrays directly, without changing their element counts or addresses.

Console, Cairo, graphics, audio, timer, browser, and manual polling examples
now document their intentional `Sleep` behavior: it either paces an animation
or audio update, yields to an event loop, or avoids a busy input poll. Each
uses a narrow rule annotation rather than weakening the general loop review.

The browser WebSocket example documents its module-level callback state and the
URL string lifetime through socket creation, retaining the asynchronous
browser-event ownership model without a misleading shared-state or escaping
pointer warning.

The pure-abstract COM optimizer examples document their intentional
module-level interface outputs, which are read by the module-scope optimizer
exercise rather than serving as unbounded application state.

OpenGL extension and NeHe examples now state the zero lower bounds of their
texture, wave-grid, and temporary bitmap arrays directly, preserving the
existing indexing and buffer sizes.

The DOS text, Mode X, and VGA demonstrations now seed their screen-noise
generation once before entering their draw loops, so repeated program starts
do not begin from the same pseudo-random sequence.

The MySQL, GTK, PCRE, and TRE examples now state zero lower bounds for ordinary
fixed result, image, XPM, and match arrays. GTK's RGB buffer also documents
that module initialization fills it before the expose callback reads it.

The CACA, DOS ISR, FreeImage, Allegro, FreeType, OpenGL, SDL, TinyPTC, GTK,
Win32, wxWidgets, sfxlib, and Expat examples now identify the intentional
module state used across their callbacks, loaders, or drawing helpers. Each
uses an exact shared-state annotation, retaining the lifetime and API boundary
without weakening checks for undocumented global state.

Win32 window-class and folder-dialog examples now state that their string
owners outlive the synchronous API calls that borrow their pointers. The
ZSTRING extension example likewise records its object-owned return buffer,
retaining the public conversion behavior with an exact lifetime annotation.

Placement-new, UDT, and allocator demonstrations now check owned allocations
before dereference and retain null propagation where an overloaded `New`
operator must report allocation failure. The PCRE wrapper initializes every
owner before setup, checks compile metadata and match-vector allocation, and
returns no match or no result when the parser is not ready.

Threading tutorials now state whether a mutex lock protects shared predicates,
handles, output, or screen state. The deliberately unsynchronized flag lessons
say that this is the contrast being demonstrated, rather than implying a
general thread-safety guarantee.

The DOS and manual graphics lessons now identify `Screen 13` as their intended
compatibility mode. This retains the historical DRAW, LINE, PAINT, PALETTE,
page-copy, WINDOW, and WINDOWTITLE behavior without presenting it as a general
target fallback.

The strict linter now recognizes `Not (condition)` as an explicit precedence
boundary while retaining its warning for unparenthesized `Not value = ...`.
OpenGL keyboard handling and logical-operator lessons use the explicit form;
the extracted comparison lesson retains a documented notation example.

The TGA loader now records that its raw comparisons are confined to exact
header arrays. The COM browser control examples likewise record fixed GUID
comparison, and the client site keeps the two interface tests as named results
before selecting its interface pointer.

OpenGL viewport dimensions, GLib inactivity timeouts, and the HTTP service
port now have named constants. GooCanvas examples document their fixed logical
canvas bounds beside the corresponding API calls. The HTTP client also makes
its socket-error sentinel conversion explicit across signed and unsigned
socket definitions.

Array tutorials now distinguish an intentional replacing `ReDim` from a
preserving resize. UDT assignment examples document that their destination is
resized and then completely overwritten by the source-array copy.

Pointer examples now check allocated or renderer-derived pointers before use.
The variable-lifetime lesson preserves the expired local address for comparison
but no longer dereferences it after scope exit; its static-storage comparison
remains readable and valid.

GOTO, generated-label, FBlite-label, and recursion-to-iteration lessons now
identify their deliberate jump site. The general flow diagnostic remains active
for ordinary code while these examples retain the language feature they teach.

DOS and SDL random visual examples now seed their generators before drawing.
The SDL_ttf sample also uses the binding's 32-bit `Long` output type for text
metrics, matching the C ABI on both 32-bit and 64-bit hosts.

GTK formatting now creates repeated decimal places in one operation, and the
WHILE/WEND reversal lesson preallocates its output. The PCRE test records that
its bundled regression input is intentionally short where line input preserves
the demonstration's text form.

Conditional and FBlite option lessons now state why an option intentionally
follows earlier declarations: each demonstrates the before-and-after behavior
of GOSUB, default argument passing, or linkage.

The Windows DDK driver sample now gives `DriverEntry`, `fb_RtInit`, and
`KeTickCount` definitions the same `StdCall` aliases declared for their driver
and runtime entry points, preserving the linker-visible ABI.

The ANY, implicit-operator, and PCRE examples now state their fixed or checked
pointer-index bounds beside the indexed operation, retaining their teaching and
wrapper behavior without treating those cases as unbounded pointer access.

DOS low-memory and gas64 preprocessing lessons now identify their inline
assembly as target-specific examples at the assembly boundary, retaining their
documented source-target requirements.

Manual numeric and declaration pages now keep their deliberate QB suffix,
negative-value, integer-coercion, and line-continuation examples while making
their teaching intent explicit. The line-continuation page compiles one
declaration and retains its other legal layouts in a disabled documentation
region, so readers can copy any form without duplicate-definition errors.

DirectDraw setup now zeroes its C ABI structures with their declared structure
sizes, avoiding ambiguous `SizeOf` resolution. String-buffer output records a
pointer-sized address value before formatting it. Printer and AROS smoke
examples check an `Open` result before using the handle, and the AROS runner's
named output volume is documented as an intentional test location.

The strict linter now defers the QB `Option Private` check until it has read
the complete module. It warns only for compatibility modules that define
routines and lack the option, so standalone QB syntax and sound examples no
longer receive a misleading module-privacy warning. Safe and unsafe fixtures
cover both sides of that decision.

Manual operator, callback, threading, string, numeric, Unicode, and control
flow examples now make their integer-width conversions, refreshed string
borrows, bounded thread counts, numeric bit patterns, and deliberate legacy
forms explicit. The timer worker now yields for an invalid state instead of
polling continuously, and the TCP sample documents both its socket-handle
ownership and its bounded readiness wait.

The strict linter also distinguishes an `Integer Ptr` type cast from a pointer
converted to an Integer value, and limits its large-static-array check to a
`Dim` declaration rather than an ordinary large-number expression. Regression
fixtures retain positive detection while covering both false-positive
boundaries.

The VARIANT WSTRING-pointer conversion now returns an independent BSTR and
clears the temporary conversion result. Its ownership contract names
`SysFreeString` as the matching caller cleanup, preserving the pointer-return
API without retaining an unowned temporary BSTR.

Strict lint now concentrates on correctness, safety, ABI, and compatibility.
Mixed indentation and long blank-run guidance remain available unchanged from
the dedicated style profile. Decimal loop steps are no longer confused with a
zero step, and the callback examples retain their nonzero floating increments.

Manual RESUME and Expat examples now acquire their file units through
`FreeFile`; both Expat versions close their successful input before releasing
the parser. The manual Expat callback has the binding's explicit character-data
handler type, and method calls in the TUI property example are qualified with
`This` so the receiver is clear to both readers and static analysis.

Unicode source with an explicit BOM or the compiler's Unicode macro now carries
its encoding contract into lint analysis. Unmarked non-ASCII source continues
to receive the portability warning. Dynamic-array lessons query their actual
bounds and document the established nonempty cases where construction or a
preceding `ReDim` proves access safe.

The complete 1,722-source Windows strict lint pass is warning-free with
documented narrow teaching exceptions honored. The separate style profile
now accepts an explicit `--indent-width auto` policy for source collections
that deliberately preserve stable per-file indentation widths. It infers a
file's smallest consistent non-tab indentation level, while one-space or
mixed-width files retain the configured four-space policy and its diagnostics.
Inference uses logical statement starts, so continuation alignment does not
distort the selected width or create a false style policy. The default remains
four spaces. The manual corpus is clean under the explicit
automatic policy after its four genuinely mixed examples were normalized; the
output-open regression fixtures retain both boundaries: an unchecked output
stream still reports `FBL103`, while an immediately checked stream does not.

The style pass now also declines byte-oriented layout checks for source that
contains embedded NUL bytes, such as UTF-16 and UTF-32 tutorial sources. Its
semantic passes remain active for those files; only whitespace and comma
diagnostics, which cannot be interpreted safely without decoding, are omitted.

When a source has many width-policy violations, style reporting now emits one
`FBL015` result for that file. The result retains the first affected line and
the complete violation count, avoiding a repeated-line flood without hiding
which source still needs a presentation decision.

`FBL009` now skips a line that follows an explicit continuation. That line is
horizontally aligned within its already-open statement, rather than introducing
a block indentation level, so its visual tab-and-space alignment does not
produce a mixed-indentation result. Non-continuation lines retain the existing
diagnostic.

The complete 1,722-source examples collection is now clean under the explicit
automatic-width style policy. Its final layout repairs normalize nested drawing,
clipping, and pixel-copy blocks while leaving their operations and compatibility
contracts unchanged.

The style parser recognizes a combined `NEXT inner, outer` as closing its
contiguous nested `FOR` frames. It accepts both established indentation forms:
aligning the combined terminator with the outer loop or with the inner loop.
The comprehensive loop check validates every counter from inner to outer and
reduces its tracked nesting depth by the same count, retaining the diagnostic
for a mismatched counter. The manual nested-loop example uses the outer-aligned
form and preserves the combined-terminator lesson without a suppression.

Example helper headers for DLLs, OpenGL textures, Win32 resources, manual
library and procedure tutorials, generics, sfxlib, timer code, VARIANT
operators, and WinMM now use individual preprocessor guards. This keeps their
existing declarations and teaching text intact while making a repeated include
safe in a single compilation unit.

The Windows include qualification checks every leaf below `inc/win` both with
and without `windows.bi`; all 265 headers pass source emission for native x64
and `-target win32`. Related direct-include repairs cover IUP, CD, libxml,
libxslt, VLC, JSON-C, Chipmunk, Flite, GDSL, MySQL, wx-c, libart, Fontconfig,
Freeglut, and CUnit without changing their umbrella include paths.

The native Exampleageddon runner has explicit rules for multi-file companions,
`-entry`, QB sources, GTK3/GooCanvas, and gas64-only vararg examples. It removes
successful per-example work trees by default and retains them with
`--keep-work`. The last clean compile-only sweep recorded 1,545 successful
examples and 129 classified failures; the last complete run baseline recorded
652 self-contained examples with no run failures.

Optional DOS threading, networking, and sound providers are documented
separately in [FreeDOS providers](freedos-providers.md),
[DOS AC'97](msdos-ac97.md), and [SuperDuel integration](msdos-superduel.md).
The normal DOS profile remains single-threaded and TCP-disabled unless those
providers are selected explicitly.

<!-- end of compatibility-services.md -->
