# Runtime library warning audit

This audit compiles the Make-selected Win64 implementations of rtlib, gfxlib2,
gfxlib3 and sfxlib, including their single-threaded and multithreaded archives.
It uses GCC 16.2.0 and Clang 22.1.7 from the local UCRT64 toolchain.

## Latest completed capture

The `rtlib-context-padding-1.json` receipts capture all 1,463 objects per
compiler with stable selected inputs: **GCC 4,460 / Clang 31,764**, or 36,224
occurrences. This is 24,373 fewer than the accepted 60,597-warning baseline, a
40.2% reduction. Per-library totals are rtlib 1,248 / 17,472, gfxlib2 1,612 /
6,610, gfxlib3 818 / 6,736 and sfxlib 782 / 946. Both builds had zero errors,
missing diagnostic captures or selected-input changes. All sixteen archive
hashes match the preceding accepted build.

The Win64 padding and packed-record probes preserve sizes and existing member
offsets under GCC and Clang. The event and PUT header probes also confirm the
byte-packed representations; the PUT header byte comparison passes at runtime
on Win64. The earlier runtime/compiler and structure checks recorded below were
not rerun for these layout-only changes.

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

- `baseline.log` and `baseline.json` retain the first observed totals.
- `original-captured.json` records the accepted starting baseline, per-object
  counts and diagnostic hashes, complete command, compiler identity and source
  hashes.
- `current-captured.json` retains an earlier accepted checkpoint.
- `rtlib-context-padding-1.json` records the latest complete build for both compilers.
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

The original source is reconstructed once in a bounded 10.6 MB snapshot at
`build/c-warning-audit/original-src`. `original-source-reconstruction.json`
records the restored files, and `baseline-validation.json` binds the initial
source hashes and original archive symbol inventories to the captured baseline.

The current checkpoint occupies about 870.5 MiB, including both reusable object
trees, diagnostic history, regression products and the single original-source
snapshot. Layout-probe sources, compiler-generated assembly/value snapshots and
the small native probe executables and objects remain under
`build/c-warning-audit/gfx3-tail-layout-probe/` for repeatable ABI checks.

## Profiles and reproduction

Clang uses `-Weverything`. GCC has no equivalent switch, so the runner explicitly
enables broad conversion, format, bounds, overflow, declaration, alignment,
shadowing, padding and control-flow warnings. The exact flags live in
`build_scripts/audit-library-warnings.py` and are copied into every receipt.
No new warning suppression is used. Existing source pragmas and system-header
classification still apply.

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
captures and the 16-archive symbol comparison.

The native test links include `fbrt0.o`, which initializes the MT critical
sections and TLS before main. An initial MT probe supplied `fb_Init` alone,
omitted that startup object and failed. The corrected native link passes with
both compilers.

The exported defined-symbol inventories of all 16 archives match the original
archives. `archive-symbol-check-current.json` records that comparison with each
archive's SHA-256. Interactive fullscreen presentation and physical audio-device
qualification remain pending.

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

## Remaining diagnostic boundaries

The latest accepted captures report **4,460 GCC warnings** and **31,764 Clang
warnings**, or **36,224 total**. GCC's largest remaining categories are
`-Wsign-conversion` (1,178), `-Wcast-align` (770), `-Wstrict-overflow` (594),
`-Wconversion` (450), and `-Wpadded` (282). Clang's largest remaining
categories are `-Wunsafe-buffer-usage` (26,338), `-Wpadded` (1,512),
`-Wsign-conversion` (1,124), `-Wcast-align` (734), and
`-Wpre-c11-compat` (304).

Layout warnings require ABI review before changing structures. Buffer-usage
warnings include ordinary C indexing and pointer arithmetic, and compatibility
warnings include older C and C++ dialects which these GNU C sources do not
target. Conversion, alignment, SDK and third-party diagnostics still need
individual review; their presence is not evidence that every one is benign.
The `FB_EVENT_` structure remains 20 bytes with one-byte aggregate alignment.
Scoped packing preserves that layout; a struct-level `aligned(1)` replacement
does not, because it leaves the natural four-byte alignment in place. The
`FB_PUT_HEADER` probe preserves the MinGW bitfield bytes as well as its member
offsets.
The two remaining reserved-macro warnings identify the Windows SDK's
`_WIN32_WINNT` selector. The two remaining C++-keyword warnings identify the
x86 assembly `and` mnemonic.

This native Win64 audit does not qualify unselected Unix, embedded or 32-bit
drivers, nor does it prove interactive graphics or audio-device behavior.

<!-- end of library-warning-audit.md -->
