# Runtime library warning audit

This audit compiles the Make-selected Win64 implementations of rtlib, gfxlib2,
gfxlib3 and sfxlib, including their single-threaded and multithreaded archives.
It uses GCC 16.2.0 and Clang 22.1.7 from the local UCRT64 toolchain.

## Latest GCC capture

The accepted final GCC receipt is
`build/c-warning-audit/gcc/final-gcc-zero-20261115e.json` (SHA-256
`b55543669005c30d5868c339a1d584bb32fd91175f170d3e44aa93d601126d4f`), with
the compiler log at `build/c-warning-audit/gcc/final-gcc-zero-20261115e.log`
(SHA-256 `092fc34c9aa28a3663e06dbaeb63335e80d655f653faf6347b2ab73dd25f76e2`).
Using GCC 16.2.0 from UCRT64, it rebuilt all **1,463 / 1,463** Make-selected
Win64 objects for rtlib, gfxlib2, gfxlib3 and sfxlib, including both threading
variants. It captured **0 warnings**: rtlib 0, gfxlib2 0, gfxlib3 0 and sfxlib
0. There were no compile errors, missing or unexpected object captures, and
the receipt records no selected-input changes. Its 1,832-file source manifest
also tracks sources outside the Win64 build; current differences from that
manifest remain outside the selected Win64 input closure.

The preceding GCC capture had 26 warnings, so the last batch removed 24
`-Wcast-align` warnings and 2 `-Wstrict-overflow` warnings. The first paired
capture recorded 9,234 GCC and 51,363 Clang warnings. The last full Clang
capture before this GCC-only cleanup recorded 27,304 warnings; Clang was not
recaptured after the final GCC fixes.

The census used GCC 16.2.0 at `-O2` with 42 explicit warning switches:
`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow`
`-Wcast-qual -Wcast-align=strict -Wformat=2 -Wundef -Wwrite-strings`
`-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations`
`-Wold-style-definition -Wredundant-decls -Wnested-externs -Wpointer-arith`
`-Wdouble-promotion -Wfloat-equal -Wnull-dereference -Wduplicated-cond`
`-Wduplicated-branches -Wlogical-op -Wformat-overflow=2`
`-Wformat-truncation=2 -Wstringop-overflow=4 -Wstrict-overflow=5`
`-Warray-bounds=2 -Walloca -Wvla -Wswitch-enum -Wswitch-default`
`-Wbad-function-cast -Wjump-misses-init -Wuninitialized -Winit-self`
`-Wmissing-include-dirs -Wtrampolines -Wdate-time -Wpacked -Wpadded`.
GCC has no single numbered warning level. The census did not use blanket
`-Werror`; the separate strict build did.

The `ci-gcc-zero` job in `.github/workflows/ci.yml` retains this census as a CI
gate using `--rebuild --fail-on-warning`.

## Library lint and regression gates

The full strict C check passed Cppcheck, clang-tidy and a GCC `--paranoid`
build with `-Werror` for the Make-selected Win64 sources in all four
libraries. The GCC build covered single-threaded and multithreaded profiles.
The source set does not include unselected OS backends. After the final runtime
edits, Cppcheck passed for `sys_parseargs.c`, `file_hconvpath.c` and Unix-only
`sys_execex.c`; clang-tidy passed for the selected Win64 `sys_parseargs.c`,
and the fresh strict GCC build passed for the selected Win64 sources.

The rebuilt compiler `.ci-host/fbc.exe` is FreeBASIC 1.20.5-2
(`2026-11-15`), SHA-256
`B86DA55E1C85FC5AE95162273A9513364E8576FEB32E6290F97140D60DA7E500`.
With that binary, `fbctests` passed 2,471 unit cases and 1,622,162 assertions
with zero failures, plus 1,933 log-test cases. All 340 warning reports
matched, and strict coverage passed for 2,636 BASIC sources, 258
keyword-table entries, 26 preprocessor forms and 11 selected CatPgMisc forms.

The rebuilt-compiler Exampleageddon report is
`build/c-warning-audit/exampleageddon-final-rebuilt-ucrt-20261115/report.md`
(SHA-256 `d530a0e5be2969cb7ffbf826df09677b2014e681b6b8f0c83ba8e5006fab70ac`).
It passed `--fail-on-self-contained`: all 655 self-contained examples passed,
with zero runtime failures or timeouts. The 1,775-file inventory had 1,434
compile passes and 341 compile failures, all classified outside that gate;
optional libraries, other platforms and unsupported multi-file examples are
not qualified by this run.

The graphics sentinel batch keeps cursor, full-circle, and arc-endpoint checks
exact using ordered comparisons, without adding a tolerance or changing the
public ABI. It removes 10 GCC and 10 Clang warnings across the single-threaded
and multithreaded objects.

The exact-comparison batch covers serial stop-bit values, ties-to-even,
CLongDouble equality operators, the RANDOMIZE sentinel, and mixer fast paths.
It preserves exact equality behavior and removes 20 GCC and 8 Clang warnings;
`-Wfloat-equal` is now zero for both compilers.

The tail-padding batch names the existing tail bytes in 28 private structures
and initializes those bytes in the fixed mode tables. Appending these fields
keeps the prior member offsets and struct strides; pointer-width guards avoid
adding tail bytes on 32-bit layouts that have none. It removes 56 GCC
`-Wpadded` warnings and 84 Clang warnings.

The next batch copies Win32 procedure addresses from `FARPROC` into the
existing `void *` slots after checking that the pointer widths match. This
preserves the dynamic-symbol ABI and removes 10 GCC `-Wpedantic` and 8 Clang
`-Wimplicit-void-ptr-cast` warnings. The archive verifier passes all eight
inventories with the original symbols retained and expected additions intact.
The paired receipts use a 1,832-file source snapshot because shared headers
changed during an earlier workspace build; each snapshot matched the live
source tree before and after its accepted compiles.

The GCC full-capture progression for this cleanup chain was 210, 182, 172, 148,
140, 128, 122, 118, 64, 62, 52, 42 and 22 warnings. The Clang captures fell
from 27,474 to 27,420, 27,414, 27,330, 27,322, 27,312 and then 27,304. The earlier
constructor cleanup uses function-pointer types for runtime constructor-section entries without
changing their sections or priorities. Other steps replaced signed endpoint
arithmetic with widened range checks, kept read-only string inputs const, and
split six long OpenGL shader sources into short fragments. The shader change
removes 12 GCC `-Woverlength-strings` warnings while preserving source order
for OpenGL.

The Win32 field-width cleanup checks values before narrowing them to
`SYSTEMTIME`'s `WORD` fields and console `COORD`'s signed `SHORT` fields. Valid
dates, times and dimensions keep their existing values; inputs that cannot be
represented are rejected instead of wrapping. It removes 22 GCC and 20 Clang
warnings, with no public signature or export changes.

The typed `DATA READ` cleanup makes the existing destination-type conversions
explicit for integer, wide-integer and integer-to-floating offset values. It
removes 36 warnings per compiler without changing the runtime's destination
types or conversion points. The two text-value `DOUBLE`-to-`SINGLE` conversions
remain for separate range-policy review.

The signed-length cleanup removes 38 warnings per compiler. It checks the
`mbstowcs()` and `wcstombs()` size results before converting them to the
runtime's signed lengths, bounds the wide-string allocation product, and casts
copy lengths only after their positive span has been established. The console
reader also bounds `fgets()`'s count to `INT_MAX`; valid input spans retain their
existing behavior.

The Win64 layout batch names existing alignment gaps in shared graphics, runtime,
Win32 and audio records. Each reserved field fills a compiler-inserted gap or tail
without moving the following fields or changing the measured record stride. The
`GFX_CHAR_CELL` and OpenGL positional initializers now explicitly initialize the
new padding members. Full builds removed 80 GCC and 1,084 Clang warnings, including
80 and 1,084 `-Wpadded` diagnostics respectively. Public function signatures and
exports are unchanged.

The Vulkan-record cleanup names the existing four-byte Win64 alignment gaps in
27 hand-declared ABI records. It adds 51 reserved fields while preserving all
original member offsets, record sizes and alignments; 389 layout assertions
generated from the pre-change layouts pass with both compilers. It removes 154
Clang `-Wpadded` warnings; the GCC count is unchanged.

The TCP length and timeout cleanup removes 18 GCC and 12 Clang warnings. Send
and read byte counts widen only after positive-result checks; chunk lengths stay
`size_t` until the socket helper applies its `INT_MAX` limit. Millisecond timeout
values are converted to `timeval` fields within their proven ranges. The Winsock
`FIONBIO` request is passed using the API's signed `long` type, and the string
reader now passes its const input directly to the const-correct descriptor helper.
These changes preserve the existing socket and string behavior.

The file-I/O range cleanup removes 48 GCC and 42 Clang warnings. Random
record alignment now rejects negative record lengths and reduces character
counts before scaling to bytes, preventing overflow in Unicode skip calculations.
Wide-string reads check the requested byte count before multiplication and reject
a device-reported read beyond that request before writing the terminator. TELL
checks putback-buffer bounds and avoids division by a nonpositive record length.
VFS mode values and string lengths are narrowed only after representability
checks. Valid file operations and exported signatures remain unchanged.

The gfxlib2 pixel-dispatch cleanup removes 48 GCC and 42 Clang warnings.
Bit-masked pixel values are explicitly converted at their narrow stores, row
copy lengths are converted only after a nonnegative-pitch check, and integer
dispatch switches retain their existing fallback behavior for unsupported
values. SCREENCONTROL string lengths are bounded before signed conversion, and
stored pen coordinates are converted only when finite and representable. Public
interfaces and valid-pixel and graphics behavior remain unchanged.

The gfxlib2 conversion-boundary cleanup removes 86 GCC and 80 Clang warnings.
Unsigned image dimensions are range-checked before conversion to signed geometry,
and image byte totals are checked before multiplication. Pixel masks, screen
colors, UTF glyph bytes, palette values and old-header bitfields are narrowed at
their type boundaries. Negative wide-string lengths, keyboard sentinels and
nonpositive dirty-row lengths are handled before size conversions. Valid image
and pixel values remain unchanged.

The float-zero cleanup removes 64 GCC and 18 Clang warnings. Exact zero tests
now use `fpclassify`, and NaN self-comparisons use `isnan`; both preserve the
existing zero and unordered-value results. Exact sentinel and arc-endpoint
comparisons remain unchanged.

The byte-alignment cleanup replaces typed accesses through byte-oriented image,
framebuffer, audio and encoded-output buffers with same-width `memcpy` operations.
The native byte order and pixel/sample values stay the same, while the stores no
longer require alignment the interfaces do not guarantee. It removes 30 GCC and
36 Clang warnings; `-Wcast-align` falls from 56 to 26 for both compilers.

The allocation cleanup removes dynamic `alloca` use from the audited runtime
paths. Filename and command-line copies now use overflow-checked heap buffers
with matching cleanup, while the already-bounded small wide-string paths use
fixed local arrays under their existing `FB_LOCALBUFF_MAXLEN` checks. Allocation
failure returns the runtime's existing out-of-memory or execution failure path.
It removes 32 GCC and 32 Clang warnings; `-Walloca` is now zero for both
compilers. Explicit allocation result types also remove 16 Clang
`-Wimplicit-void-ptr-cast` occurrences.

The length and input-boundary batch removes 48 GCC and 70 Clang warnings from the
preceding paired capture. It checks string and array extents before narrowing or
multiplying, keeps screen input within its fixed buffer, and rejects oversized
pattern and driver-name lengths. These guards preserve public signatures and
valid-input behavior while avoiding signed overflow and out-of-bounds copies.

The SIMD-loop and page-copy batch removes 24 GCC and 10 Clang warnings from
the preceding paired capture. The SSE2 kernels track remaining elements
with unsigned counters instead of subtracting chunk widths from signed counts.
`fb_GfxFlip` checks view bounds and byte, row-offset and character-cell
products before copying; zero-sized views do not advance row pointers. The
boolean string helper now uses the size type expected by each length API.
Valid pixel order and public interfaces stay unchanged.

The alpha and color conversion batch removes 18 GCC and 16 Clang warnings.
The C blend kernels use unsigned weights after `fb_hPutBlend` clamps alpha
to eight bits. Color channel bytes are narrowed only after masking, and the
legacy signed palette boundary is explicit. Pixel values and draw order
remain unchanged.

The pmap scale and pixel-mask batch removes 16 warnings per compiler.
View dimensions are converted explicitly at the float boundary while the
integer inversion operation keeps its original order. Pixel masks stay in
the unsigned color domain, preserving the full-width mask value.

The `fb_StrSwap` cleanup makes five positive length differences explicit as
`size_t` after their existing signed comparisons. It removes 10 warning
occurrences per compiler without changing the buffer lengths or copy order.

The warning-class batch adds explicit no-op defaults to 24 clean switches and
explicitly promotes float values where C already promoted them. GCC drops 48
switch-default and 76 double-promotion warnings. Clang drops 48 and 76 in those
categories, but reports eight covered-enum and eight fallthrough warnings from
the added defaults. Seven Clang-only promotion sites remain.

The function-result cleanup stores each returned value in its original type
before applying the same cast. It removes 56 `-Wbad-function-cast` warnings
per compiler. Moving four terminal switch defaults to the start of their
switches also removes eight Clang fallthrough warnings. Ten cast warnings
remain in two locally modified files, and the seven promotion sites remain in
another locally modified file.

The formatter cleanup removes 12 warnings per compiler. It makes the
existing exponent conversions explicit and stores `floor()` and `fabs()` results
before the original integer conversions. The narrowing and exponent operation
order remain the same; exact comparisons with zero are retained for formatting
compatibility.

The COM-device conversion cleanup removes 10 GCC and 12 Clang warnings. It
preserves the serial write's const input, makes validated pointer lengths and
the existing unsigned file-width storage conversions explicit, and types the
option-buffer allocation result. The serial API and valid option values are
unchanged.

The Direct2D cleanup removes 16 GCC and 30 Clang warnings. Pointer conversions,
the key-state narrowing, and the DXGI integer-to-float conversion are now
explicit. The dirty-row scan reads only full word chunks inside the row array,
uses `memcpy` for those loads, and copies any remaining prefix bytes with a
length bounded by the row span. This removes its previous possible read before
the array start.

The Win32 window cleanup removes 12 GCC and 16 Clang warnings. It treats the
timer API's nonzero return as success, keeps the existing byte narrowing for
palette entries explicit, and stages `_beginthreadex()`'s integer handle before
the existing `HANDLE` conversion. The new switch default still reaches the
existing default window-message handling path.

The gfxlib2 palette cleanup removes 22 warnings per compiler. It makes color
packing shifts unsigned, spells out the existing six-bit-to-eight-bit
truncation, and bounds the dirty-row length conversion. The byte values and
palette masking remain unchanged; negative RGB shift behavior is now defined.
Archive checks confirm that all original defined symbols remain and the
expected additive symbols are unchanged.

The gfxlib2 LZW encoder cleanup removes 20 GCC warnings and eight Clang
conversion warnings. Hash indexes and the code counter now use unsigned
arithmetic within the existing table and code limits. Explicit byte casts
document the narrowing in the packed 12-bit output. Codes still use the same
byte packing order; the existing padding in the shared LZW table remains
because changing that internal layout is unnecessary for this warning fix.

The console line editor cleanup removes 20 sign-conversion warnings per
compiler. It keeps cursor columns unsigned after validating the terminal width,
uses subtraction-based bounds for downward movement, initializes the queried
dimensions, and clamps string lengths before passing them to signed-length
runtime helpers.

The gfxlib3 compatibility cleanup removes 24 GCC and six Clang warnings. Its
named helper preserves exact floating-point equality for zero sentinels and
degenerate windows, including signed-zero equality and NaN rejection, without
using an epsilon. The arc sample-count check also makes its exact conversion
from float to double explicit.

The GPU surface bounds cleanup removes 26 GCC and ten Clang warnings. Exact
scale, aspect, and full-circle checks remain exact; signed 32-bit endpoint
validation is kept in a typed helper. The guarded `floor` and `ceil` results
are stored before narrowing, and both GCC objects for this source now compile
without warnings.

The UTF-to-wide converter cleanup removes 22 warning occurrences per compiler.
The bounded UTF-8 length switches now state their existing default path, the
wide-character size switches retain their previous fallback, and the ASCII
encoding case still returns `NULL`. The narrow return casts explicitly pass
through `void *` because the one-byte `FB_WCHAR` configuration aliases `char`.

The encoded-file writer cleanup removes 42 GCC and 38 Clang warnings. It keeps
UTF-16 and UTF-32 little-endian conversion byte-aligned by loading each unit
through `memcpy`, checks lengths before narrowing them to the converter's
signed parameter, and releases converted buffers after a failed write. Valid
encoded output remains byte-for-byte unchanged.

The gfxlib2 image allocator cleanup removes 18 warnings per compiler. It makes
the high-bit flag check unsigned, keeps legacy header truncation explicit, and
stores and loads the saved allocation pointer through `memcpy` instead of an
aligned `void **` view. Header bit patterns and pointer bytes remain unchanged.

The wide-character UTF output converter cleanup removes 18 GCC and 24 Clang
warnings. It writes UTF-16 and UTF-32 code units through `memcpy`, so caller
buffers need no alignment beyond bytes while retaining the same native byte
order. Its explicit encoding checks preserve the `NULL` result for ASCII and
unsupported selectors.

The Win32 console input cleanup removes 20 GCC and 18 Clang warnings. The
console ring bounds the count passed to `WriteConsoleInput`, and explicit
casts retain the existing `WORD` key fields. Character classification now
receives only valid byte values, leaving larger mapped values unchanged.

The `fb_MemSwap` cleanup removes 18 GCC and 12 Clang warnings. Word-sized
exchanges now copy through local values, so byte buffers need no alignment and
the exchanged byte order stays identical. The final partial word remains a
byte-by-byte swap.

The gfxlib2 GET cleanup removes 16 GCC and 20 Clang warning occurrences. It
computes dimensions in wide intermediates, checks pitch and total pixel-byte
arithmetic before narrowing, and checks the full output header and pixel region
against bounded FreeBASIC array storage before writing. Valid header layouts
and pitch rules stay unchanged; invalid dimensions and undersized arrays now
return `ILLEGALFUNCTIONCALL` before any output is written.

The gfxlib2 console scroll cleanup removes 18 GCC and 20 Clang warnings. It
checks character and pixel bounds in wide arithmetic before narrowing, and
validates character-page offsets and row sizes before moving cells. Negative
scroll counts now clear the character cells as they already clear the pixel
region. Valid scrolls retain their existing row order and rendering behavior.

The gfxlib2 CLS cleanup removes 18 warnings per compiler. It checks full-screen
and viewport pixel counts before multiplication, validates text-row bounds
before converting them to pixel offsets, and bounds the dirty-row span before
passing it to the memory helper. The existing pixel color bits and clear areas
stay unchanged for valid screen configurations.

The serial-time decoder cleanup removes 12 warnings per compiler. It makes the
conversion of each `long double` expression result back to `double` explicit
and explicitly promotes the input before the calculation. This keeps the
original extended-precision operations and narrowing points; the exact signed
zero checks for QB compatibility remain unchanged.

The Win32 cleanup typed native boundaries in the DirectDraw, GDI, Direct2D,
shared Win32 window and OpenGL driver code. It added checks for pixel-depth
narrowing, allocation arithmetic, dirty-row bounds, code-page conversion
buffers, OpenGL scale dimensions and mode-list growth. It also tightened
font-pitch and string-length validation, used wide intermediates for viewport
clipping, and kept the built-in font source const. These five Win32 C files,
the font helper and the string renderer now have no sign-conversion warnings
under either compiler. The changes touch an internal helper header but do not
change public API signatures or exports. The current source snapshot also
includes the separate TLS-key runtime change described below.

Win64 layout probes confirm the Vulkan surface and point records remain 24 and
32 bytes, with every existing member offset unchanged under GCC and Clang. The
event and PUT header probes also confirm the byte-packed representations; the
PUT header byte comparison passes at runtime on Win64. The four current gfxlib3
archives each contain the additive `fb_Gfx3DrawGlyphs` symbol. The four rtlib
archives contain the additive `fb_TlsGetVarKey` symbol from the separate TLS
change. No original defined symbol was removed; the other eight archive
inventories match exactly. The earlier runtime/compiler and structure checks
recorded below were not rerun for these layout changes or the new exports.

The gfxlib3 command queue now aligns the flexible payload member for its
64-bit protocol fields and uses a typed accessor that preserves that guarantee
for GCC and Clang. This removes 342 `-Wcast-align` occurrences per compiler
across the single-threaded and multithreaded objects. The Win64 payload offset
remains 40 bytes, and no public API signature changed. In gfxlib2, one-byte
aligned scalar views make the packed blitter loads and stores valid when row
pitches leave a pixel row unaligned. That removes a further 56 alignment
warnings per compiler without changing the pixel operations.
The BMP BLOAD converters remove 12 more alignment warnings per compiler, and
the byte-pitched software PUT paths remove 62 while retaining their pixel math
and row progression.

## Starting baseline and earlier CINT cut

The original source produces **60,597 warning occurrences** with these profiles.
The recount captures all 1,463 archive/startup objects independently for each
compiler. All 1,798 source hashes recorded before the first repair match the
pinned original snapshot.

| Complete per-object capture | GCC | Clang | Total |
| --- | ---: | ---: | ---: |
| Original source | 9,234 | 51,363 | 60,597 |
| Earlier CINT source cut | 6,867 | 34,103 | 40,970 |
| Removed | 2,367 | 17,260 | 19,627 |

Both CINT builds completed successfully with stable selected inputs and
complete object coverage. The reduction is 32.4% of diagnostic occurrences.
Both compilers now report zero missing-prototype, unused-parameter and implicit-fallthrough warnings. Clang also reports zero empty-expression-statement, missing-variable-declaration and missing-noreturn warnings.

| Library | Original GCC | CINT GCC | Original Clang | CINT Clang |
| --- | ---: | ---: | ---: | ---: |
| rtlib | 3,472 | 2,289 | 28,727 | 18,853 |
| gfxlib2 | 2,714 | 2,260 | 10,854 | 7,020 |
| gfxlib3 | 2,002 | 1,382 | 9,354 | 7,126 |
| sfxlib | 1,046 | 936 | 2,428 | 1,104 |

Counts include repeated header diagnostics, bundled decoders and both threading
variants. Library attribution follows the object being compiled, including its
shared headers. Each compiler's `counts.csv` tracks the named runs and marks
complete, frozen per-object captures as accepted.

## Historical aggregate observations

| Logged build | GCC | Clang | Total |
| --- | ---: | ---: | ---: |
| First combined log (incomplete) | 9,232 | 51,351 | 60,583 |
| First repair batch | 9,168 | 47,304 | 56,472 |
| Declaration repair batch | 8,856 | 46,929 | 55,785 |
| Original source through pipe capture | 9,169 | 51,287 | 60,456 |
| Declaration repairs through pipe capture | 8,856 | 46,713 | 55,569 |
| Guards and bounds through pipe capture | 8,852 | 45,247 | 54,099 |

These observations remain in the history with `accepted=False`. Validation
found lost or interleaved MSYS diagnostic output with inherited file handles,
a single-writer pipe and Make's output synchronization. The initial combined log showed 60,583; the complete per-object recount found
60,597, so 14 warning occurrences were lost in the aggregate. Counting complete compiler command lines alone
also missed this problem.

`capture-library-compiler.c` now forwards each invocation to the real compiler
and retains its output beside the object in a separate `.o.diagnostics` file.
The Python runner clears those sidecars before a run, checks each archive member
and startup object has a capture, then combines the captures sequentially into
a durable `.diagnostics.log`. Ten launcher checks cover both compilers, quoted
arguments, paths with spaces, version/target probes and failure exit status.
Captured bytes match direct compiler output. Four additional comparisons
recompile actual runtime and gfxlib3 units with each build's exact flags and
match their captured diagnostic bytes.

Receipts and logs live under `build/c-warning-audit/{gcc,clang}/`.

- `float-zero-classification-1.json` records the latest accepted GCC and Clang
  captures for exact-zero classification and NaN checks.
- `gfxlib2-conversion-boundaries-1.json` records the preceding accepted GCC and
  Clang captures for graphics image, pixel and palette conversions.
- `gfx-pixel-dispatch-boundaries-1.json` records the preceding accepted GCC and
  Clang captures for pixel conversions, switch fallbacks and graphics control
  boundaries.
- `file-io-range-guards-2.json` records the preceding accepted GCC and Clang
  captures for file-I/O length and offset guards.
- `baseline.log` and `baseline.json` retain the first observed totals.
- `original-captured.json` records the accepted starting baseline, per-object
  counts and diagnostic hashes, complete command, compiler identity and source
  hashes.
- `current-captured.json` retains an earlier accepted checkpoint.
- `gfx-drawstring-sign-1.json` records the latest accepted GCC and Clang builds.
- `gfx3-payload-align-1.json`, `gfxblitter-unaligned-1.json` and
  `gfx3-float-overflow-1.json` record the subsequent warning reductions and
  overflow-safe line interpolation.
- `gfx-image-convert-unaligned-1.json`, `gfx-put-blend-unaligned-1.json` and
  `gfx-bload-unaligned-1.json` record the later byte-alignment cleanups.
- `gfx-put-pixel-align-1.json` records the software PUT alignment cleanup.
- `win32-opengl-sign-3.json` and `win32-opengl-sign-2.json` record the preceding
  accepted GCC and Clang builds.
- `win32-core-sign-1.json` and `win32-sign-boundaries-1.json` record preceding
  accepted builds.
- `gfx3-vulkan-tail-padding-1.json` and `gfx3-vulkan-tail-padding-2.json`
  retain the preceding GCC and accepted Clang graphics-layout builds.
- `profiler-array-report-1.json` records the preceding accepted GCC and Clang
  captures for the profiler array and report-bound cleanup.
- `profiler-core-bounds-1.json` records the newer accepted captures for the
  profiler string-table and metrics bounds cleanup.
- `printusing-safe-bounds-2.json` records the preceding accepted captures for
  the PRINT USING range and integer-conversion cleanup.
- `gfx-screen-bounds-2.json` records the preceding accepted captures for the
  gfxlib2 screen resize and mode-bounds cleanup.
- `gfx-line-conversion-3.json` records the preceding accepted captures for the
  gfxlib2 line rasterizer conversion cleanup.
- `file-utf-read-casts-1.json` records the preceding accepted UTF reader
  conversion and switch cleanup.
- `file-utf-read-casts-2.json` records the preceding accepted UTF reader and
  Win32 OpenGL native-boundary cleanup.
- `gfx-draw-boundaries-2.json` records the preceding accepted GCC and Clang
  captures for DRAW parsing and coordinate conversion boundaries.
- `gfx-box-callback-boundaries-1.json` records the latest accepted GCC and
  Clang captures for BOX pixel callbacks and dirty-row lengths.
- `gfx-palette-boundaries-1.json` records the latest accepted GCC and Clang
  captures for palette color packing and dirty-row lengths.
- `gfx-lzw-code-packing-1.json` records the latest accepted GCC and Clang
  captures for LZW hash indexes and packed code conversions.
- `con-readline-size-bounds-1.json` records the latest accepted GCC and Clang
  captures for console cursor bounds and signed string lengths.
- `gfx3-compat-exact-floats-1.json` records the latest accepted GCC and Clang
  captures for exact compatibility comparisons and arc bounds.
- `gfx3-gpu-surface-bounds-1.json` records the latest accepted GCC and Clang
  captures for GPU surface endpoint bounds and transform conversions.
- `utf-convto-wchar-switches-1.json` preserves the first UTF-to-wide switch
  cleanup capture; its covered-switch warning is corrected in the final pair.
- `utf-convto-wchar-switches-2.json` records the latest accepted GCC and Clang
  captures after removing that covered-switch warning.
- `file-encod-writers-bounds-1.json` records the latest accepted GCC and Clang
  captures for encoded-file writer length bounds and aligned UTF conversion.
- `gfx-image-header-bounds-1.json` records the latest accepted GCC and Clang
  captures for image header conversions and saved allocation-pointer access.
- `utf-wchar-output-align-3.json` records the latest accepted GCC and Clang
  captures for aligned UTF output stores and encoding dispatch.
- `win32-console-input-bounds-1.json` records the latest accepted GCC and Clang
  captures for console input count bounds and key-code conversions.
- `rtlib-mem-swap-alignment-1.json` records the latest accepted GCC and Clang
  captures for byte-safe word-sized memory swaps.
- `gfx-get-buffer-bounds-3.json` and `gfx-get-buffer-bounds-clang-1.json` record
  the latest accepted paired captures for GET dimensions and output bounds.
- `gfx-print-scroll-bounds-gcc-2.json` and
  `gfx-print-scroll-bounds-clang-1.json` record the latest accepted paired
  captures for console scroll and character-cell bounds.
- `gfx-cls-bounds-gcc-2.json` and `gfx-cls-bounds-clang-2.json` record the
  latest accepted paired captures for graphics and text clearing bounds.
- `datetime-serial-explicit-casts-gcc-2.json` and
  `datetime-serial-explicit-casts-clang-2.json` record the latest accepted
  paired captures for serial-time arithmetic conversions.
- `dev-com-explicit-conversions-gcc-1.json` and
  `dev-com-explicit-conversions-clang-1.json` record the latest accepted paired
  captures for COM-device pointer and width conversions.
- `str-format-explicit-numerics-gcc-1.json` and
  `str-format-explicit-numerics-clang-1.json` record the latest accepted paired
  captures for formatter numeric conversions.
- `d2d-dirty-scan-bounds-gcc-1.json` and
  `d2d-dirty-scan-bounds-clang-1.json` record the latest accepted paired
  captures for Direct2D conversions and dirty-row bounds.
- `d2d-prefix-copy-win32-warnings-gcc-1.json` and
  `d2d-prefix-copy-win32-warnings-clang-1.json` record the latest accepted
  paired captures for the bounded prefix scan and Win32 warning cleanup.
- `parsedate-span-sizes-1.json` and `gfx-core-conversions-1.json` record the
  preceding date-parser and graphics-core cleanup captures.
- `gfx-paint-boundaries-1.json` and `gfx-paint-row-boundary-1.json` record the
  preceding paint conversion and row-boundary cleanup captures, before the
  core and date-parser changes.
- `intl-core-aligned-bounds-1.json` records the preceding accepted GCC and
  Clang captures for Win32 string conversion alignment and size boundaries.
- `vulkan-private-padding-2.json` and `vulkan-private-padding-1.json` retain
  the preceding accepted GCC and Clang builds, respectively.
- `rtlib-context-padding-1.json` retains the preceding common checkpoint.
- `file-size-padding-1.json` retains the preceding accepted checkpoint.
- `packed-pragma-1.json` retains the packed-record cleanup checkpoint.
- `gfx3-tail-padding-2.json` retains the preceding graphics padding checkpoint.
- `tail-gaps-explicit-1.json` retains the pre-gfxlib3-padding checkpoint.
- `cint-double-explicit-1.json` retains the pre-tail-padding checkpoint.
- `fb-inputctx-padding-3.json` retains the complete pre-CINT checkpoint.
- `reserved-tail-cleanup-1.json` retains the complete pre-padding checkpoint.
- `../comparison.json` contains the accepted before/after counts by library.
- Each run's `.diagnostics.log` preserves the authoritative diagnostic text.
- `counts.csv` records the warning history for each compiler.
- Objects and archives are reused in place between runs.

The GCC `gfx3-tail-padding-1` row is retained with `accepted=False`. That
attempt selected MSYS GCC 15.3 with a Cygwin target and failed during archive
creation. Its warning count is excluded from the accepted totals.

The GCC `vulkan-private-padding-1` attempt is also retained with
`accepted=False`: its PowerShell environment did not put the MSYS utilities on
`PATH`, so Make could not find `grep` or `tr` and compiled zero objects. The
Clang `gfx3-vulkan-tail-padding-1` capture is retained as unaccepted because
`gfx3_exports.h` changed while its objects were compiling. The audit runner now
normalizes dependency paths before checking for changed selected inputs; the
stable full rerun is `gfx3-vulkan-tail-padding-2.json`.

The first OpenGL GCC attempt is retained as `win32-opengl-sign-1.json` with
`accepted=False`: the `make` shim did not provide the required MSYS tools, and
zero library objects compiled. The first OpenGL Clang pass is also unaccepted
because `fb_thread.h` and `thread_var.c` changed during compilation. The
current accepted GCC and Clang receipts use the stable source snapshot.

The GCC `gfx-line-conversion-2` capture is retained with `accepted=False`
because `src/rtlib/devices/dev_tcp.c` changed during its full build. The stable
paired rerun is `gfx-line-conversion-3.json`.

The original source is reconstructed once in a bounded 10.6 MB snapshot at
`build/c-warning-audit/original-src`. `original-source-reconstruction.json`
records the restored files, and `baseline-validation.json` binds the initial
source hashes and original archive symbol inventories to the captured baseline.

The current checkpoint occupies about 1,005.1 MiB, including both reusable object
trees, diagnostic history, regression products and the single original-source
snapshot. Layout-probe sources, compiler-generated assembly/value snapshots and
the small native probe executables and objects remain under
`build/c-warning-audit/gfx3-tail-layout-probe/` for repeatable ABI checks.

## Profiles and reproduction

Clang uses `-Weverything`. GCC has no equivalent switch, so the runner explicitly
enables broad conversion, format, bounds, overflow, declaration, alignment,
shadowing, padding and control-flow warnings. The exact flags live in
`build_scripts/audit-library-warnings.py` and are copied into every receipt.
One GCC diagnostic scope suppresses selected warning categories only while
including the bundled audio decoder sources. The first-party warning profiles
remain active. Existing source pragmas and system-header classification still
apply.

From a shell with the compiler and required POSIX build tools on PATH:

```text
python build_scripts/audit-library-warnings.py --cc gcc --target x86_64-w64-mingw32 --label next --rebuild
python build_scripts/audit-library-warnings.py --cc clang --target x86_64-w64-mingw32 --label next --rebuild
```

On Windows, select the MSYS2 Make executable with
`--make C:/msys64/usr/bin/make.exe`. These builds publish only below
`build/c-warning-audit/`; they do not replace installed libraries.

## First repair batch

- Release the temporary printer name when OPEN reuses an existing printer.
  Build checked-length names directly instead of formatting them through an
  `int`-sized snprintf result.
- Use unsigned shifts for opaque BMP pixels and packed RGB565 blend masks.
- Keep the DirectDraw fullscreen surface rectangle separate from window
  coordinates during teardown. The old local variable shadowed the initialized
  surface rectangle and was uninitialized on the fullscreen path.
- Declare the existing null-pointer-check export in its runtime header.
- Check profiler suffix capacity before copying the fixed extension.
- Spell CP437 glyph bytes as string escapes, preserving their byte values on
  signed-char hosts.
- Initialize empty numeric-format lengths, scope the QB random seed temporary,
  move three inline wide-string declarations before statements and remove dead
  breaks and duplicate Vulkan/SIMD declarations.

## Declaration repair batch

The existing graphics and audio exports now have shared typed declarations.
`src/gfxlib3/gfx3_exports.h` owns the gfxlib3 C export signatures; the existing
gfxlib2 and sfxlib headers own their corresponding helpers. Definitions retain
their original calling conventions and parameter types. Local declarations
which duplicated those contracts were removed.

Both compilers passed 94 affected-file syntax checks with missing-prototype and
redundant-declaration diagnostics treated as errors. All missing-prototype
warnings disappeared from the subsequent whole-library builds.

## Guard and bounds repair batch

- Replace 69 implementation-reserved graphics and sound include guards. Update
  the Vulkan shader generator's matching guard and retain its generated output
  contract.
- Parse decimal time fields within the supplied byte span. Bound AM/PM lookahead
  and reject oversized fields before narrowing their values.
- Check Windows file-mode capacity in release builds, return `EINVAL` for an
  oversized mode and release converted path storage when reopen is refused.
- Use the text length to bound reverse-search candidates and keep the text base
  pointer within its object after an unsuccessful search.
- Reuse the TLS destructor helper for explicit context deletion, preserving the
  destructor-before-free order and clearing the calling thread's slot afterward.

The time-span regression passes 259,273 checks, including every ordinary
hour/minute/second combination and inputs at a protected page boundary. The
Windows mode regression passes 35 checks. Both fixtures fail with the original
implementations. A differential check of 1,998,880 ordinary time inputs matches
the original parser. The public time parser also preserves the original
Win64 behavior on its 444-check compatibility corpus.

The reverse-search private-span fixture fails on the original helper and passes
79,305 checks after repair. Its archive-linked variant exercises the public
entry point against an independent reference, including embedded NULs, high
bytes and start positions around both ends of each string.

## Unused-parameter cleanup

The file-device, graphics and platform callbacks keep their required signatures;
implementation-only unused arguments are marked explicitly. Internal helpers
drop parameters they never used. Both complete compiler captures now report zero
`-Wunused-parameter` warnings, with no exported C signature changes.

## Fallthrough and dead-macro cleanup

Replace 57 intentional fallthrough comments in eight C files with the local
`FB_FALLTHROUGH` marker. Clang accepts the compiler-checked attribute at the
strict warning level, and both complete captures now have no implicit-
fallthrough warnings. Remove unused local WGL constants and other dead
macros, and keep the TLS fallback macros out of single-thread builds. Clang
unused-macro warnings fell from 92 to 8. This batch removes 198 warning
occurrences from the previous checkpoint; GCC remains at 8,576.

## Statement-macro and declaration cleanup

Convert event locks, dirty marking and the LZW input/output helpers to
single-statement `do { } while (0)` macros. This removes 54 Clang
extra-semicolon warnings without changing generated behavior. Add prior
declarations for the cursor boundary bytes and shared Win32/WinCE key table,
and mark the Win64 `fb_End` implementation as non-returning. The final Clang
capture has no `-Wextra-semi-stmt`, `-Wmissing-variable-declarations` or
`-Wmissing-noreturn` warnings. This batch removes 62 occurrences from the
previous checkpoint; GCC remains at 8,576.

## Win64 compatibility follow-up

Keep the Win64 CRT compatibility translation units nonempty without adding runtime symbols. Use explicit sequentially consistent atomic ordering on the supported Clang and GCC paths; the older GCC __sync fallback retains the same full-barrier behavior. Initialize the Direct2D minimum feature level with its API default, and scope XPAD and WGL macros to the platform code that needs them.

This batch removes 4 GCC and 11 Clang warning occurrences. Clang unused-macro warnings drop from 8 to 4. That capture reported 8,572 GCC warnings and 44,761 Clang warnings, with all 1,463 objects compiled for each compiler. All 16 defined-symbol inventories still match the original archives.

## Console cursor arithmetic follow-up

Horizontal movement now uses signed 64-bit deltas and separates screen-width rows from the remaining column movement before updating the cursor. This avoids narrowing long string positions to int and prevents signed overflow in the cursor calculation. String-derived deltas are bounded by FB_STRSIZEMSK, the runtime string length limit; row coordinates are saturated to the int range.

This change removes 16 GCC and 16 Clang warning occurrences from the previous accepted checkpoint. The Clang capture has no implicit-int-conversion-on-negation or shorten-64-to-32 diagnostics in con_readline.c. Both full builds compiled all 1,463 objects, and all 16 defined-symbol inventories still match the original archives. The existing runtime regression checks were not rerun after this cursor change.

## Platform macro and initializer cleanup

Limit the stb_vorbis alloca intrinsic to GCC targets where the bundled source does not provide its own declaration path. Keep COBJMACROS available to the active Direct2D COM headers. Remove the DXGI adapter initializer because the structure is only read after GetDesc succeeds.

This capture removes two Clang unused-macro warnings and two C23 empty-initializer warnings; GCC is unchanged. Two Clang unused-macro instances remain for the COM header configuration. Both full builds compiled all 1,463 objects, with no build errors and unchanged original archive symbol inventories.

## Direct2D C23 initializer cleanup

Replace active C23-only empty initializers with standard C zero initialization or explicit API defaults. Leave render-target and texture fields fully initialized before the Direct2D and D3D calls; keep the DXGI descriptor uninitialized only where GetDesc success is checked before any read.

This batch removes 16 Clang C23-extension warnings and leaves GCC unchanged. The full captures compile all 1,463 objects with no new enum-conversion or uninitialized-field warnings. At this point, Clang still reports C23-extension warnings from the shared array flag header; the next cleanup removes them.

## Array flag header cleanup

Represent `FBARRAY_FLAGS` as `uint32_t` and define its masks as typed macros. This avoids the C23 extension warning caused by the reserved enum value while preserving the flag values and the Win64 descriptor layout.

Clang removes 1,203 warnings from the shared header, and the C23-extension category is now absent. GCC is unchanged. Both full builds compiled all 1,463 objects with no build errors or missing diagnostics. All 16 defined-symbol inventories still match the original archives.

## Serial ABI assertion compatibility

Use the existing `STATIC_ASSERT_LEGACY` typedef macro for the serial ABI size checks. This keeps the same compile-time conditions while avoiding a C11-only expansion in the shared header.

Clang removes 2,610 `-Wpre-c11-compat` warnings, and GCC is unchanged. Both full builds compiled all 1,463 objects with no errors or missing diagnostics. All 16 defined-symbol inventories still match the original archives.

## Native wide-character type cleanup

Use the project `FB_WCHAR` alias for internal Windows wide-character declarations. GCC and Clang bind the alias to their exact native wide-character type through `__WCHAR_TYPE__`; other compilers retain the existing `wchar_t` spelling. The Windows declarations and API calls therefore keep the same type while avoiding repeated C++ keyword diagnostics.

Clang removes 2,672 warnings, taking `-Wc++-keyword` from 2,682 to 10. GCC is unchanged. Both full builds compiled all 1,463 objects without errors or missing diagnostics, and all 16 defined-symbol inventories still match.

## MinGW macro and keyword cleanup

Keep `NO_OLDNAMES` for MinGW-w64 x64, and retain the extra `_NO_OLDNAMES` define for other MinGW targets that may need the legacy `_ssize_t` path. Rename the `fb_Wait` parameter identifiers `and` and `xor` without changing its ABI.

Clang removes 1,305 reserved-macro warnings and eight C++ keyword warnings. The remaining two reserved-macro diagnostics are for the Windows SDK `_WIN32_WINNT` selector, and the remaining two C++ keyword diagnostics identify the x86 assembly `and` mnemonic. GCC is unchanged. Both full builds compile all 1,463 objects without errors or missing diagnostics; all 16 symbol inventories match.

## Reserved private type identifiers

Rename the private graphics cell and pixel header tags, console and COM state tags, printer information tag, and Windows mouse-tracking callback typedef. Their existing typedef names, fields, signatures, and object layout stay the same.

Clang removes 414 reserved-identifier warnings. GCC is unchanged. Both full builds compile all 1,463 objects without errors or missing diagnostics, and every archive symbol inventory still matches the original.

## C identifier and ABI label cleanup

Use ordinary C identifiers for the shared graphics and sound globals, gfxlib3
backend tables, legacy OpenGL state, console context, and private helper types.
`FB_C_SYMBOL` binds the renamed C declarations to their established assembler
labels, including the profiler section boundaries expected by GNU ld. Private
static GUIDs, mutexes, and TLS state use ordinary source names without changing
linkage.

The graphics global batch removes 1,312 Clang warnings. The shared sound,
backend, OpenGL, key-table, and console batch removes 656 more. Renaming the
remaining private C identifiers removes the final 38. Clang's
`-Wreserved-identifier` category is now absent; GCC is unchanged. The full builds
compile all 1,463 objects, and all 16 original defined-symbol inventories still
match.

## Validation

The printer regression failed on the original shared-open path with an extra
live allocation. It now passes 413 checks, including repeated sharing, failed
driver opens and allocation failures. The profiler-name regression passes 4,092
capacity and fallback checks. The pixel regression passes 196,608 comparisons
against independent BMP and RGB565 references; Clang also runs it with undefined
behavior traps enabled.

`build_scripts/test-library-warning-fixes.py` also checks the existing Unicode
word helpers, console bounds, RND sequences and 108 archive-linked C exports.
It accepts `--mt` to select the multithreaded archives. Logs and archive-bound
receipts are under `build/c-warning-audit/checks/`.

All 36 compiler/variant checks passed, with twelve single-threaded checks and
six multithreaded checks per compiler. The reverse-search public entry point
passes 79,301 comparisons per variant. TLS context deletion passes twelve
checks covering zero initialization, reuse, destructor calls and empty slots.

The 36 runtime/compiler-variant checks and 31 graphics structure graphs recorded
above were run before this identifier-only cleanup. They were not rerun in this
warning-count pass. The current source state was verified by both full compiler
captures. The latest archive comparison retains every original defined symbol
and adds `fb_Gfx3DrawGlyphs` to the four gfxlib3 archives.

The native test links include `fbrt0.o`, which initializes the MT critical
sections and TLS before main. An initial MT probe supplied `fb_Init` alone,
omitted that startup object and failed. The corrected native link passes with
both compilers.

Twelve of the sixteen current archive symbol inventories match the original
exactly. The four gfxlib3 archives each add `fb_Gfx3DrawGlyphs`; none removes an
original symbol. `archive-symbol-check-current.json` records the current
archive hashes and symbol differences. Interactive fullscreen presentation
and physical audio-device qualification remain pending.

The graphics structure checker passed all 31 graphs. The runtime structure
checker reported one header-format issue in the pre-existing untracked
`src/rtlib/drivers/dos/file_attrs.c`, which this audit leaves unchanged.

Concurrent edits to unselected DOS drivers are recorded by the source guard.
They do not invalidate the Win64 build unless they occur in its compiler
commands or dependency closure. Selected input changes do invalidate the run.

## FB_INPUTCTX padding cleanup

Name the four-byte Win64 gaps before `FBSTRING str` and at the end of
`FB_INPUTCTX` only when the compiler reports an eight-byte pointer size. These
members occupy bytes that were already implicit padding: the 48-byte Win64
layout and all existing member offsets stay fixed. The 24-byte Win32 layout is
unchanged. Compile-time layout probes pass with GCC and Clang for both pointer
widths.

This removes 1,305 GCC and 20 Clang `-Wpadded` occurrences from the prior
accepted checkpoint, taking the combined total from 43,063 to 41,738. Both
final captures rebuilt all 1,463 objects with no errors, missing diagnostics
or selected-input changes. All archive hashes match the pre-padding checkpoint.

## Explicit CINT double arithmetic

`CINT` already rounded float inputs in double precision because its constants
were double literals. Cast the inputs explicitly to preserve those results
while avoiding implicit-promotion warnings. The macro still evaluates its
argument the same number of times, and no in-tree call site passes `long double`.
This removes 384 `-Wdouble-promotion` occurrences from each compiler and lowers
the combined total from 41,738 to 40,970. The full captures compiled all 1,463
objects with stable selected inputs; all archive hashes remain unchanged.

## Explicit runtime and audio tail padding

Name the existing Win64 tail bytes in `FB_DROP_ITEM`, `FB_GFXCTX` and
`FB_SFX_ASSET`. The explicit bytes consume padding already required by each
structure's alignment. GCC and Clang layout probes confirm the sizes and
existing member offsets on Win32 and Win64.

This removes 554 GCC and 434 Clang `-Wpadded` occurrences, taking the combined
total from 40,970 to 39,982. Both accepted full builds captured all 1,463
objects with stable selected inputs.

## gfxlib3 tail padding

Name the existing Win64 tail bytes in `FB_GFX3_COMMAND_QUEUE`,
`FB_GFX3_BACKEND_CONFIG`, `FB_GFX3_LOGGER`, `FB_GFX3_RENDERER_CONFIG`,
`FB_GFX3_INPUT_STATE`, `FB_GFX3_IMAGE_CACHE_ENTRY` and `FB_GFX3_IMAGE_VIEW`.
The three-byte `FB_GFX3_POINT_CACHE` tail is explicit on both pointer widths;
its size and member offsets stay fixed. GCC and Clang layout probes compare all
recorded sizes and existing offsets for Win32 and Win64.

This removes 384 GCC and 234 Clang padding warnings, taking the combined total
from 39,982 to 39,364. The complete builds compiled all 1,463 objects, with no
errors, missing captures or selected-input changes.

## Packed record annotations

Use scoped `#pragma pack(push, 1)` and `#pragma pack(pop)` for `FB_EVENT_` and
`FB_PUT_HEADER`. Preserve MinGW's `gcc_struct` convention on `FB_PUT_HEADER`,
which controls its historical bitfield allocation. Compile-only probes compare
record sizes, aggregate alignment and ordinary member offsets for GCC and Clang
on Win32 and Win64. A Win64 byte comparison also confirms the PUT header's
bitfield representation.

This removes all 1,469 GCC `-Wpacked` occurrences without changing archive
hashes; Clang's warning count is unchanged. GCC no longer reports
`-Wpacked` for these records.

## FB_FILE size alignment

Name the existing four-byte Win64 gap between `FB_FILE.encod` and `FB_FILE.size`.
The explicit field begins at offset 12, while `size` remains at offset 16; the
80-byte structure and every existing member offset remain unchanged. The field
is conditional on an eight-byte pointer, so the Win32 definition stays as it
was. GCC and Clang compile-only layout probes confirm both pointer widths.

This removes 1,305 Clang `-Wpadded` occurrences and lowers the combined total
from 37,895 to 36,590. GCC's warning count is unchanged.

## Runtime context argv alignment

Name the four-byte Win64 gap after `FB_RTLIB_CTX.argc`. The field starts at
offset 4 and `argv` remains at offset 8; the 20,864-byte context and its other
member offsets stay fixed. The field is absent on Win32. GCC and Clang layout
probes confirm both pointer widths.

This removes 366 Clang `-Wpadded` occurrences and lowers the combined total
from 36,590 to 36,224. GCC's warning count is unchanged. Both latest complete
captures compile all 1,463 objects with no errors or missing diagnostics.

## Vulkan tail padding and export follow-up

Name the existing 64-bit tail bytes in eight Vulkan ABI records in
`gfx3_vulkan_private.h`, then in `FB_GFX3_VULKAN_SURFACE` and
`FB_GFX3_VULKAN_POINTS` in `gfx3_vulkan.h`. These fields occupy compiler-created
tail padding; the Win64 surface and point records remain 24 and 32 bytes, and
all existing member offsets match the staged pre-change header in GCC and Clang
compile probes. GCC `-Wpadded` occurrences fell from 282 to 158; Clang's fell
from 1,512 to 1,464.

The stable Clang capture includes a concurrent `gfx3_exports.h` declaration
for `fb_Gfx3DrawGlyphs`. The four current gfxlib3 archives each add that symbol
to the original defined-symbol inventory, with no removals. The declaration
also coincided with 50 more Clang `-Wunsafe-buffer-usage` occurrences across
`gfx3_gpu_surface.c` and `gfx3_context.c`, so the total Clang count is two
higher than the preceding accepted checkpoint despite the padding cleanup.
`archive-symbol-check-current.json` records the current archive hashes and
symbol differences.

## gfxlib3 command payload alignment

The command allocation is zero-initialized with `calloc`, and its payload
records contain 64-bit handles and addresses. Align the flexible payload member
to `uint64_t` and pass typed views through `__builtin_assume_aligned` on GCC and
Clang. The Win64 payload offset remains 40 bytes. This removes 342
`-Wcast-align` occurrences per compiler across both threading variants, with
all 1,463 objects captured and no public API changes.

## gfxlib2 packed pixel alignment

The software blitter reads and writes packed 16-bit and 32-bit pixels through
byte-addressed scanlines. Odd-width rows can start at a two-byte offset, so
one-byte-aligned scalar aliases make those accesses valid without relying on
word alignment. This removes 56 `-Wcast-align` occurrences per compiler; the
pixel masks and iteration order remain unchanged.

## gfxlib3 transform precision and line arithmetic

Explicitly convert float inputs to double where transform bounds and inverse
matrix terms already use double intermediates. The values are still rounded to
float when stored in the matrix, and this removes 46 `-Wdouble-promotion`
occurrences per compiler. The shadow line rasterizer now advances with a 64-bit error accumulator seeded at half the major-axis distance. This preserves the existing rounded pixel path while avoiding signed delta products and per-pixel division. The full GCC build removes 48 `-Wstrict-overflow` occurrences; Clang's warning count is unchanged.

## gfxlib2 SCREENCONTROL width checks

`SCREENCONTROL` routes pointer-sized values through a legacy window callback and GL setting
record whose fields use `int`. Check values against `INT_MIN` and `INT_MAX` before narrowing;
out-of-range values leave those fields unchanged. Values in range preserve prior behavior. Pen
coordinates still convert to the existing `float` storage, now through an explicit cast.

This removes 38 GCC `-Wconversion` occurrences and reduces the total warning count by 42 in
each compiler's complete 1,463-object capture. Public signatures and record layouts are
unchanged.

## gfxlib3 null backend transform precision

The inverse matrix stores coefficients as `float`, while pixel-center transforms use `double`
intermediates. Convert the coefficients explicitly at the multiply/add boundary to preserve the
existing double-precision calculations. This removes 18 `-Wdouble-promotion` occurrences per
compiler across normal and MT objects, with no API or structure layout changes.

## gfxlib3 OpenGL ellipse bounds

Ellipse centers and radii are stored as `float`, but bounds are calculated in `double` before
the `int32_t` range checks. Convert both operands explicitly at that boundary to preserve the
existing double-precision calculation. This removes 16 `-Wdouble-promotion` occurrences per
compiler across normal and MT objects, with no API or structure layout changes.

## gfxlib2 image conversion byte alignment

The format converters write 16-bit and 32-bit pixels through byte-addressed destination
pointers. Use one-byte-aligned, alias-aware scalar views for these stores so packed-row offsets
do not rely on stronger alignment. Pixel values and iteration order are unchanged. This removes
20 `-Wcast-align` occurrences per compiler across normal and MT objects.

## gfxlib2 BLEND byte alignment

The BLEND routine walks byte-pitched source and destination rows, then handles 16-bit pixels
in 32-bit pairs. Use one-byte-aligned, alias-aware scalar views for those reads and writes so
row offsets do not rely on word alignment. Blend arithmetic, masks and row progression remain
unchanged. This removes 14 `-Wcast-align` occurrences per compiler across normal and MT objects.

## gfxlib2 BLOAD byte alignment

The BMP BLOAD converters write packed pixels into byte-addressed rows. Use
one-byte-aligned, alias-aware scalar views for their 16-bit and 32-bit stores so
the destination address does not need stronger alignment than the row
provides. Conversion values and row progression are unchanged. This removes
12 `-Wcast-align` occurrences per compiler across normal and MT objects.

## gfxlib2 BMP bounds and ellipse rasterization

The BMP loader rejects `INT32_MIN` before height negation, promotes header offsets before addition, checks both seeks, calculates row allocation with `size_t` and rejects overflow, and rejects unsupported conversion depths. Across the normal and MT objects, `gfx_bload.c` warning occurrences fell from 48 to zero under GCC and from 210 to 160 under Clang.

The circle and ellipse path replaces a viewport-sized VLA with checked zeroed heap storage for fill coverage and frees it on every exit. Explicit conversions preserve existing float-to-int pixel rounding and arc precision. Across normal and MT objects, `gfx_circle.c` warning occurrences fell from 88 to 2 under GCC and from 90 to 12 under Clang. The remaining direct floating comparison is the legacy full-circle sentinel; Clang also reports raw buffer operations in this file.

## PNG Paeth arithmetic

The PNG encoder and decoder use byte samples for the Paeth predictor. Adding
the same 255 bias to the prediction and each sample reference leaves all three
distances unchanged while keeping them in unsigned arithmetic; the original
tie ordering is preserved. The row unfilter also rejects unexpected filter
values in its switch. Across the normal and MT objects, this removes 84 GCC
`-Wstrict-overflow` and 4 `-Wswitch-default` warnings, plus 4 Clang
`-Wswitch-default` warnings.

## gfxlib2 BSAVE row bounds and pixel loads

BSAVE now checks header dimensions and pitch before narrowing them to `int`,
calculates BMP rows and file sizes with checked `size_t` arithmetic, and rejects
files too large for the BMP size fields. Indexed rows copy only the bytes that
the BMP row emits, and 16-bit and 32-bit source pixels use `memcpy` so byte
payload alignment does not affect the load. Output pixel order is unchanged.
Across normal and MT objects, `gfx_bsave.c` warning occurrences fell from 42
to zero under GCC and from 80 to 56 under Clang; the remaining Clang warnings
in this file are raw buffer access and pointer arithmetic.

## gfxlib2 WinMM joystick conversions

The WinMM axis conversion keeps the original float subtraction, double
calculation and final float rounding. Explicit casts document each step and
prevent implicit conversion warnings. The validated joystick index is also
explicitly converted to the WinMM parameter types. `gfx_joystick.c` warnings
fell from 40 to zero under GCC and from 40 to 2 under Clang; the two remaining
Clang reports are the bounded `joy[id]` array access in the normal and MT
objects.

## gfxlib2 software PUT byte alignment

The C PUT fallbacks read and write packed 16-bit and 32-bit pixels while moving
between byte-pitched rows. Use one-byte-aligned, alias-aware scalar views for
those accesses, including the alpha and alpha-mask paths. Keep the existing
pixel arithmetic, masks and pointer progression unchanged. This removes 62
`-Wcast-align` occurrences per compiler across normal and MT objects; public
signatures and exports stay unchanged.

## gfxlib3 Vulkan scratch alignment

The draw and blit paths advance typed pointers through checked scratch-buffer regions, using the element size that defines each region. This replaces byte-offset pointer casts without changing allocation sizes or record contents. It removes 18 `-Wcast-align` occurrences per compiler across normal and MT objects.

## gfxlib2 core pixels and gfxlib3 point packets

Use one-byte-aligned, alias-aware scalar views for gfxlib2's packed framebuffer reads and writes. In the Vulkan point path, retain the byte offset once, check it against the command record's alignment, then use the aligned view. These changes remove 12 `-Wcast-align` occurrences per compiler across normal and MT objects; existing pixel arithmetic and command layout stay unchanged.

## gfxlib2 C flexible PUT header tail

Declare `FB_PUT_HEADER.data` as a standard flexible array in C. Keep the existing zero-length array spelling for C++ consumers. The Win64 packed layout probe still matches size, alignment, field offsets and initialized bytes under GCC and Clang. This removes 164 GCC `-Wpedantic` and 164 Clang `-Wzero-length-array` occurrences without changing the record layout.

## gfxlib2 OpenGL dimensions and conversions

Use `size_t` for extension-string lengths, validate dimensions before narrowing
or rounding them up to powers of two, and check texture allocation arithmetic
before allocating. Return cleanly when dimensions are invalid or allocation
fails. Guard the 256-entry palette and make the existing float-to-double
normalization explicit so valid colors keep their original rounding. Add
explicit default branches that preserve the old no-operation behavior for
unsupported depths, and cast both operands in the screen texture ratios.

This removes 40 GCC and 38 Clang warnings from gfxlib2, including the source's
signed conversions, floating promotions and missing switch defaults. GCC now
reports only padding warnings from headers included by `gfx_opengl.c`; Clang
still reports unsafe-buffer diagnostics for its extension, pointer and array
accesses. The exported interface is unchanged.

The final `opengl-bounds-footer-1` GCC and Clang captures each compiled all
1,463 objects. Both have zero errors, missing or unexpected diagnostic records,
and selected-input changes. The 16-archive comparison retains every original
defined symbol and the eight previously identified additions; see
`archive-symbol-check-current.json`.

## rtlib RND conversion clarity

Compare single-precision RND arguments with `0.0f`, store 32-bit generator
results in typed locals before converting them, and make the QB path's final
float-to-double return conversion explicit. Keep the algorithm selector in an
`int` until its existing conversion to the `uint32_t` state field. These edits
preserve the exact zero sentinel, generator call order, seed updates and QB
float division. One temporary declaration moved to the function declaration
block to clear Clang's declaration-after-statement warning.

This removes 26 GCC and 34 Clang warnings from `math_rnd.c`. GCC's remaining
direct warnings in that file are exact floating-point comparisons. Clang still
reports its C dialect, unsafe-buffer and exact-comparison diagnostics there.

## rtlib profiler arrays and report bounds

Keep the procedure-array sort views const-correct and skip `qsort()` for arrays
with fewer than two entries. Guard array growth against invalid counts, signed
doubling overflow and allocation-size overflow. Saturate the existing integer
allocation metric instead of allowing it to wrap. Grow the call-tree leader
buffer using checked unsigned capacities within its `ssize_t` range. The raw
string report copies each record header into an aligned local object and checks
record bounds and the final NUL before printing. Valid report formatting and
the public profiler interface are unchanged.

The accepted full captures remove 34 GCC and 22 Clang warning occurrences.
GCC no longer reports the procedure-array conversions, cast qualifiers, call
tree overflow, string-header alignment or variadic float promotion. Clang's
alignment cast report for the string header is also gone; its remaining
unsafe-buffer reports cover the bounded byte accesses and other existing C
array operations.

## rtlib profiler string-table bounds

Bound string-name scans to the largest record the fixed table can hold, and
calculate padded record sizes with `size_t` before narrowing. Reject invalid
table usage counts and block-id overflow. Read string headers into aligned local
objects and check each record before updating maximum lengths or metrics. Use
the hash modulus result only after an explicit bounded conversion, and saturate
byte metrics at `INT_MAX`. Filename copying retains the existing bounded,
zero-padded behavior. Valid records, table layout and public function
signatures remain unchanged.

Across the complete captures, `profile.c` warnings fell from 34 to 2 under GCC
and from 54 to 38 under Clang. The remaining GCC reports are the existing
record-writer alignment cast; Clang also retains unsafe-buffer reports for the
bounded byte operations. Those newly visible scans add two
`-Wunsafe-buffer-usage` reports, while Clang's total for the file falls by 16.

## rtlib PRINT USING range and integer boundaries

Keep the existing format grammar and validate field byte and padding lengths
before allocation or conversion to `size_t`. UTF-8 output lengths are narrowed
only after using the encoder's one-to-four-byte result; the local output buffer
has room for a complete encoded scalar. Explicit default arms retain the
existing no-op behavior for unmatched format characters. Double scaling masks
the IEEE exponent before narrowing, and signed LONGINT magnitude uses unsigned
subtraction so `LLONG_MIN` does not overflow. Public signatures and string
layouts remain unchanged.

Across both object variants, `io_printusg.c` fell from 36 to 4 GCC warnings and
from 138 to 114 Clang diagnostics. The remaining GCC warnings are the context
padding and the const cast required by `fb_UStrPrint`'s mutable descriptor.
Clang retains its existing buffer-access, declaration-order and floating-point
promotion reports in this formatter.

## gfxlib2 screen resize and mode bounds

Check the graphics depth along with the existing page, pitch and dimension
guards before resizing. Convert validated row widths and pitches explicitly
before size arithmetic. Store and recover the allocation base pointer with
`memcpy`, avoiding typed pointer access immediately before the 16-byte-aligned
page while preserving the existing allocator layout. Keep the exact-zero aspect
sentinel and make the existing float/double calculation explicit; public
signatures, mode layout and pixel operations are unchanged.

Across the normal and multithreaded objects, `gfx_screen.c` warnings fell from
38 to 4 under GCC and from 128 to 94 under Clang. The remaining GCC warnings
are structure padding and the intentional exact-zero aspect comparison.

## gfxlib2 line rasterizer conversions

Keep line-style masks unsigned through bit reversal and pixel selection, and
make the legacy color and span conversions explicit at the pixel setter
boundary. Explicit float and double casts preserve the existing WINDOW
coordinate transform. Clipped horizontal and dirty-row spans are converted to
`size_t` only after their endpoints are ordered. Public signatures and line
rasterization are unchanged.

Across normal and MT objects, `gfx_line.c` warnings fell from 28 to zero under
GCC and from 38 to 6 under Clang. The remaining Clang diagnostics are its
unsafe-buffer reports for the bounded framebuffer and dirty-row pointer access.

## rtlib encoded-file reader boundaries

Make the byte narrowing explicit after the existing 255-value replacement
checks, and make UTF-16 and `FB_WCHAR` stores explicit at the width-selected
conversion points. Keep the current decoding and replacement behavior. The
encoding dispatch still returns zero for ASCII or unsupported selectors, and
the size switches retain their zero-result behavior for unsupported widths.

`dev_file_encod_read_core.c` warnings fell from 28 to zero under GCC and from
106 to 78 under Clang. The remaining Clang diagnostics are unsafe-buffer
reports on the decoder's byte tables and pointer iteration.

## gfxlib2 Win32 OpenGL boundaries

Keep configured component sizes as full integers in the WGL attribute list and
cast explicitly only when storing the `PIXELFORMATDESCRIPTOR` byte fields.
Convert the WGL procedure address through the shared byte-copy helper, which
checks pointer representation sizes, and cast the dynamic-library handle at
the `HMODULE` boundary. No public signature or export changed.

`gfx_driver_opengl.c` warnings fell from 26 to zero under GCC and from 108 to
82 under Clang. The remaining Clang diagnostics are unsafe-buffer reports for
the WGL attribute array.

## gfxlib2 DRAW parsing and coordinate boundaries

Accumulate signed DRAW numbers in the negative range so parsing can represent
the full `intptr_t` range without signed overflow; reject numeric overflow
before it reaches drawing operations. Check lengths before narrowing them to
the rasterizer's `int` type. Preserve the previous float conversion,
rounding and truncation for representable coordinates, while rejecting
non-finite or out-of-range coordinates before float-to-int conversion. Keep
masked colors and the high-bit default-color selector explicit at their
integer API boundaries. No public signatures or exports changed.

`gfx_draw.c` is now warning-free under GCC. The changes remove 36 Clang
conversion and signedness diagnostics across the normal and MT objects. Clang
still reports unsafe-buffer usage for pointer traversal through the
NUL-terminated DRAW command and number parser.

## Win32 international conversion alignment and size boundaries

`FBSTRING_ACCESS` payloads may be byte-aligned, so convert UTF-16 through
malloc-aligned `WCHAR` buffers and copy their bytes into the runtime
string. Check string sizes against the Win32 signed-`int` API counts before
narrowing, bound allocation arithmetic, and stop locale-buffer growth before
its API count can overflow. The existing UTF-16 byte layout, terminators and
public ABI remain unchanged.

Across the normal and MT objects, this removes 30 GCC and 30 Clang warnings.
GCC now reports none in `intl_core.c`; Clang retains one unsafe-buffer
diagnostic per object at the output terminator store.

## gfxlib2 PAINT range and callback boundaries

Make the color and span-length conversions explicit at the legacy pixel
callback boundary. Check viewport height before converting it to `size_t` for
allocation, and make the existing float/double coordinate transform explicit
after validating its range. Packed pattern rows stay within the existing 1..64
limit before their lengths reach `memcpy`. Use `tail->y > context->view_y` for
the previous-row boundary instead of subtracting one from a signed coordinate.
These changes preserve the public signatures and the flood-fill traversal.

Across both threading variants, the paint cleanup removes 28 GCC and 30 Clang
warnings. GCC reports no warnings in `gfx_paint.c` or its packed-pattern helper.
Clang retains private-structure padding and unsafe-buffer diagnostics for the
bounded pattern, span and framebuffer accesses.

## gfxlib2 core context and pixel boundaries

Check screen and packed-header heights before converting them to allocation
sizes, and guard both line-array allocations against `SIZE_MAX` overflow.
Reject new packed-header dimensions or pitches that cannot fit the context's
signed fields before narrowing them. Make color-mask, pixel-width and window
coordinate conversions explicit; the pixel masks and float transform remain
unchanged. Keep the default coordinate-type branch as the existing no-op.

Across the normal and MT objects, this removes 28 warnings per compiler from
`gfx_core.c`. The pixel-width casts preserve the low bytes used by the existing
1-, 2- and 4-byte pixel paths. Public function signatures and framebuffer
layout are unchanged; Clang retains its unsafe-buffer diagnostics for the
bounded pixel and event arrays.

## rtlib date-parser span conversions

Convert forward pointer differences to `size_t` explicitly and select token
lengths with a size-aware helper. The parser's date order, numeric grammar and
two-digit-year adjustment stay unchanged; no exported interface changes.

This removes 24 warnings per compiler across the normal and MT objects.
GCC now reports none in `time_parsedate.c`; Clang's remaining diagnostics are
its unsafe-buffer reports for the parser's input traversal.

## gfxlib2 BOX callback and span widths

Pass raw colors through the legacy callback's `int` parameter explicitly,
convert clipped row lengths to `size_t` at the pixel and dirty-row boundaries,
and keep the rotating 16-bit style mask unsigned. The clipped rectangle and
style-bit iteration remain unchanged.

This removes 22 warnings per compiler across the normal and MT objects.
Clang retains its unsafe-buffer reports for the bounded row and framebuffer
accesses; public signatures are unchanged.

## Intermediate paired-capture snapshot (superseded)

At this intermediate point, paired captures reported **424 GCC warnings** and **27,658 Clang warnings**, or **28,082 total**. These counts precede the final GCC cleanup above and are retained here as historical context.

The largest GCC categories are `-Wsign-conversion` (120), `-Wconversion` (58), `-Wpadded` (56), `-Wcast-qual` (38), `-Wpedantic` (30), `-Wfloat-equal` (30), `-Wcast-align` (26) and `-Wstrict-overflow` (20); `-Wswitch-default` is zero. The largest Clang categories are `-Wunsafe-buffer-usage` (26,340), `-Wpre-c11-compat` (308), `-Wimplicit-void-ptr-cast` (234), `-Wpadded` (226), `-Wsign-conversion` (156), `-Wdeclaration-after-statement` (112) and `-Wimplicit-int-conversion` (26). Clang's `-Wfloat-equal` count is 18. Both compilers report zero `-Walloca` warnings.

At that stage, per-library counts were GCC: rtlib 312, gfxlib2 28, gfxlib3 60, sfxlib 24; Clang: rtlib 16,624, gfxlib2 4,320, gfxlib3 5,858, sfxlib 856. Both paired captures compiled all 1,463 objects with no errors, missing or unexpected diagnostics, or selected-input changes. That source snapshot included the Xbox-specific `fb_FileRename` branch.

The next GCC pass targeted sign and width conversions, then proceeded through
the final cast-alignment and overflow fixes. At the intermediate point,
sign and width conversions were the highest-yield bug classes: 120
sign-conversion warnings span 56 file/line sites, and 58 other
conversion warnings span 28 sites. A full pass would address 178 occurrences
across 84 file/line sites, but each site needs range or FreeBASIC
conversion-semantics review. Clang has another 156 sign-conversion occurrences,
so a shared signed-boundary pass has up to 276 sign-conversion warnings in play
across the paired builds. The 56 remaining `-Wpadded` warnings occupy 28
structures and are a separate layout cleanup, not 56 runtime bugs.

The Win64 padding batch replaces measured compiler-created gaps and tail bytes with named reserved fields, keeping the existing field offsets and record strides. The Vulkan records have field-by-field Win64 layout checks; other remaining layout warnings still need individual ABI review. Buffer-usage warnings include ordinary C indexing and pointer arithmetic, and compatibility warnings include older C and C++ dialects that these GNU C sources do not target. Conversion, alignment, SDK and third-party diagnostics still need individual review; their presence is not evidence that every one is benign. The `FB_EVENT_` structure remains 20 bytes with one-byte aggregate alignment. Scoped packing preserves that layout; a struct-level `aligned(1)` replacement does not, because it leaves the natural four-byte alignment in place. The `FB_PUT_HEADER` probes preserve the MinGW bitfield bytes as well as its member offsets.

At that intermediate point, two reserved-macro warnings identified the Windows SDK's `_WIN32_WINNT` selector, and two C++-keyword warnings identified the x86 assembly `and` mnemonic.

This native Win64 audit does not qualify unselected Unix, embedded or 32-bit drivers, nor does it prove interactive graphics or audio-device behavior.

<!-- end of library-warning-audit.md -->
