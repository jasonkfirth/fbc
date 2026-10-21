# Classic AmigaOS

This port targets AmigaOS 3.x on a 68020 or later CPU. The compiler emits C
for `m68k-amigaos-gcc`, which links native Hunk executables. The runtime,
gfxlib2, and sfxlib use native DOS, Exec, Intuition, graphics, and device APIs.
An FPU and the disk-based IEEE math libraries are not required.

The starting SDK contract comes from FreeBASIC-NG revision
`a60609d219e70ba1c145109b157960d85b87f019`. Runtime file, threading, graphics,
and sound work also draws on this tree's AROS implementation. Classic AmigaOS
has its own platform adapters because its newlib SDK and native DOS handles
are different from AROS POSIXC.

## Building on Linux

The workflow needs a working host FreeBASIC compiler, GNU make, Python 3,
host C build tools, Autoconf, Automake, and Libtool. Docker is needed only
when provisioning the SDK. FS-UAE and Xvfb are needed to run native tests;
`xdotool` and ImageMagick are optional for timeout screenshots.

```sh
build_scripts/debianubuntu-build-freebasic-amiga.sh --jobs 8
```

The SDK is extracted under `out/amiga/toolchain`. Its image is pinned to:

```text
amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed
```

The SDK contains GCC 6.5.0b and native headers. The build relocates its
container-rooted symbolic links, then applies hash-checked repairs to the
inspected newlib startup objects. Those repairs preserve the exit stack
frame and pass the actual `argv` value to `main`.

The target archives are written to `lib/freebasic/amiga-m68k`. Normal and
multithreaded runtime, graphics, and sound variants use the same
`-m68020 -msoft-float -fno-common` contract. Additional providers are:

| Archive | Purpose |
| --- | --- |
| `libfbsoftfloat.a` | CPU floating arithmetic and conversions without external math libraries |
| `libpthread.a` | AmigaPorts pthread implementation with stable thread and TLS records and repaired cleanup |
| `libffi.a` | libffi 3.4.8 adapted to Hunk symbols, m68k pointer returns, and Exec cache maintenance |

The providers are built from pinned, hash-checked inputs. Their build scripts
record the upstream revisions and preserve the license notices. GCC helpers
retain GPLv3 and the GCC Runtime Library Exception; libffi retains its MIT
license. The runtime install also copies provider licenses and the libffi
headers under the target library directory.

The pinned CPU arithmetic source is verified before bounded build-time repairs
are applied. Those repairs retain product guard/sticky bits, correct an
incomplete single-product mask, classify infinity times NaN, and preserve the
sign of underflow to zero. They are covered by native comparisons with host
hardware results; the original downloaded source and its license remain intact.

For an existing SDK, use `--skip-toolchain --toolchain-root /path/to/sdk`.
`AMIGA_ROOT` selects the support build directory and `--skip-compiler` keeps
an already-built host compiler. Temporary build files stay in that support
directory rather than relying on space in `/tmp`.

## Compiling programs

From the source tree:

```sh
export PATH="$PWD/out/amiga/toolchain/bin:$PATH"
bin/fbc -prefix "$PWD" -i inc -i inc/amiga -target amiga-m68k hello.bas -x hello
```

`amiga`, `amigaos`, and `m68k-amigaos` are target aliases. The driver selects
the GCC backend and native library order automatically. Unsupported CPU
families, other code generators, and `-dll` are rejected before tool execution.
The Hunk linker must select `libfbsoftfloat.a` before the SDK's indirect
floating-point aliases; the compiler and bootstrap link rules do this.

The resulting program can be copied to an Amiga filesystem and started from
its shell. Graphics and sound archives are selected through the ordinary
FreeBASIC commands and include files.

Cross-built Hunk objects and libraries retain FreeBASIC dependency metadata.
The driver reads both ordinary ar containers and the concatenated Hunk units
produced by the native SDK archiver, so dependencies declared by a library
are also selected when linking its callers. External libraries still need
native Amiga builds; host Linux archives cannot satisfy those dependencies.

The native compiler front end can also be built:

```sh
build_scripts/debianubuntu-build-freebasic-amiga.sh --native-compiler
```

Its binary is `out/amiga/native-fbc`; building it preserves the host
`bin/fbc`. A complete native compile and link needs an Amiga-hosted GCC
installation. The Linux SDK's tools are cross tools and cannot run inside
AmigaOS. With the front end alone, `-r -m module module.bas` emits C, which
can be compiled and linked by the cross SDK. The native qualification checks
this complete sequence and runs the resulting Hunk program, including its
command-line argument check.

Native include handling recognizes `volume:directory/file` paths and resolves
assigns through DOS. Shared BASIC headers can keep their `.` and `..`
components: the runtime translates them to native parent components before
opening a file. Empty components in existing native paths retain their
AmigaDOS meaning.

Worker threads inherit their creator's native standard streams. BASIC global
destructors can signal and join workers before the runtime performs its final
task cleanup. That final wait still precedes runtime-state release and Hunk
unloading, because live native tasks share the command's code and data.
Sound and graphics contexts reopened by a destructor are closed before that
wait, while the runtime's locks, streams and thread-local storage still exist.

## Graphics and sound

gfxlib2 uses the `AMIGA` driver. It presents dirty framebuffer rows through
Intuition and graphics.library. It can use a CyberGraphX public screen when
available, or open a private chipset screen. Indexed framebuffers preserve
their palette. Higher-depth BASIC framebuffers are converted to the available
chipset palette, including RGB332 on an eight-plane AGA display.
True-colour conversion selects the closest available component levels using
tables prepared when the screen opens. It does not change the software
framebuffer's pixel values. Indexed modes continue to use the program's
palette, including changes made without drawing another frame.

The requested software canvas can exceed the physical display. The chipset
backend bounds its display-size request to the selected monitor's available
ordinary modes and keeps the original nominal aspect ratio. Larger canvases
retain AmigaOS
[virtual-screen behavior](https://wiki.amigaos.net/wiki/Intuition_Screens#Overscan_and_the_Display_Clip).

The chipset path uses a one-row temporary bitmap for `WritePixelLine8`.
Its width is word-aligned independently of BASIC's framebuffer pitch, so
odd framebuffer widths remain valid.
An optional cache of the last uploaded source rows avoids repeated planar
transfers when only the hardware palette changes or a frame is unchanged.
Comparisons use the complete pixel bytes, and window refresh events invalidate
the cache before repainting. If cache allocation fails, presentation continues
through the normal row conversion and transfer path.
Raw keys use keymap.library; mouse and
window events come from the window's Intuition message port.
`SETMOUSE` hides the native pointer with a transparent chip-memory sprite and
restores the system pointer when requested. Intuition retains responsibility
for presenting it above graphics updates and across focus changes.
The sprite allocation and lifetime follow the native
[Intuition pointer contract](https://wiki.amigaos.net/wiki/Intuition_Pointer).

sfxlib first tries AHI and falls back to Paula's ROM-resident audio.device.
Paula playback uses a left/right channel pair and chip-memory DMA buffers.
The mixer retains its requested rate while a rational phase accumulator
resamples to a supported DMA period. Odd output bytes carry into the next
write because Paula transfers words. Device requests use a reply port owned
by the task performing the write, and both channels finish before their
buffers are reused.

The sound worker and mixer use recursive Exec semaphores. Shutdown requests
stop under the mixer lock, then joins the worker outside that lock before
releasing device state. Output capture writes little-endian PCM WAV even
though this target is big-endian.

## Native qualification

Supply a local Kickstart ROM; ROMs are not distributed with the repository.
The qualification runner creates disposable host-directory boot volumes,
so these tests do not require a Workbench disk image.

```sh
export AMIGA_KICKSTART=/absolute/path/to/kickstart.rom

build_scripts/amiga-run-fbctests.sh --output out/amiga/fbctests --jobs 8
build_scripts/amiga-run-exampleageddon.sh --output out/amiga/exampleageddon --jobs 8
build_scripts/amiga-run-probes.sh --output out/amiga/probes \
    --native-compiler out/amiga/native-fbc
```

Every emulator session runs on its own authenticated Xvfb display with
software rendering. The runner never uses the desktop display. Host audio
is disabled while guest audio interrupts remain active. Xvfb and FS-UAE are
terminated when a batch finishes, times out, or the runner receives SIGTERM.
Install `xvfb` or set `AMIGA_XVFB_BIN` to its executable. An extracted local
installation under `out/amiga/virtual-display` is also recognized.

Each output directory holds a compiler/include/library snapshot, frozen
test sources and resources, compile logs, guest logs, JSON/CSV results, and
a Markdown report. Resuming verifies the recorded input hashes:

```sh
build_scripts/amiga-run-exampleageddon.sh \
    --output out/amiga/exampleageddon --resume
```

Use a new output directory to test changed inputs. `--compile-only` builds
an inventory without launching an emulator. `--dirs file,threads` selects
fbctests directories. `--backend-debug` enables native graphics and sound
diagnostics. The default FS-UAE accuracy setting permits accelerated CPU
execution; `--accuracy 1` can be substantially slower for software floating
point and needs a larger timeout.

Jobs may provide `stdin_text` for a reproducible UTF-8 input transcript. The
runner stages that input separately from program resources and selects its
native DOS handle while the program runs. Five-field manifests remain valid;
the optional sixth field names an input file. The controller restores its
previous input and output handles before starting the next job.

The guest controller uses `LoadSeg` and `RunCommand`, records the actual
return code, and restores process state between jobs. fbctests additionally
requires complete, consistent XML suite and case counts. Printed markers
alone do not qualify a test.

The native probes exercise:

- 200 KB binary command pipes in both directions, backpressure, EOF, early
  close, exit cleanup, metadata, and relative paths;
- inherited native standard streams in child and nested threads, redirected
  worker output, and parent output after joining those threads;
- a worker whose stop request and join belong to a BASIC global destructor;
- sound initialization from a BASIC global destructor after the initial
  sound shutdown, followed by a clean native return;
- direct `LINE INPUT` from LF-terminated redirected standard input, followed
  by `SLEEP`, `GETKEY` and EOF checks on the same buffered stream;
- native bitmap and palette readback at BASIC depths 8, 16, and 32 with an
  odd width, all 256 component values, live palette changes, plus raw-key and
  mouse message translation, including queued key presses/releases consumed
  directly by `MULTIKEY`;
- hidden-page drawing, default `SCREENCOPY`, and visible-page switches at all
  three framebuffer depths;
- native mode selection for a 1280x600 virtual canvas, checking the ROM's
  PAL 1280x512 or NTSC 1280x400 display and pixels at both horizontal edges;
- 1,024 BMP bitfield samples with one- through eight-bit components, including
  narrow alpha expansion and alpha drawing;
- bounded Hunk metadata parsing and native execution of transitive archive
  and direct-object dependency links;
- rejected driver-option exit codes and compile-only SDL2/CRT declarations;
- native sound-driver selection and a quarter-second stereo capture, with
  separate 440 Hz and 880 Hz tones checked for duration, amplitude, and channel
  ordering;
- 20,000 software floating conversion comparisons against a host hardware
  oracle, including boundary values and reproducible random bit patterns;
- 20,000 software multiplication comparisons against a host hardware oracle,
  including signed zeros, subnormals, halfway products, infinities and NaNs,
  plus eight signed-zero division checks;
- 60,000 software addition, subtraction and division comparisons, including
  both operand signs, exact cancellation, overflow and subnormal rounding;
- optional native compiler emission and execution of its generated program.

## Recorded results and limits

The October 2, 2026 runs used FS-UAE 3.2.35, an A1200/020 configuration,
Kickstart 3.1 revision 40.68, 2 MB chip RAM, 8 MB fast RAM, and 256 MB Zorro III
RAM. They booted disposable directory volumes and used private Xvfb displays.

| Qualification | Result |
| --- | --- |
| fbctests | 37 directory batches passed; 1,162,179 assertions; complete XML reports |
| Exampleageddon | All 649 self-contained examples compiled and returned successfully |
| Native probes | Six checks passed, including native compiler output execution |
| Graphics | Native readback and input translation passed at depths 8, 16, and 32 |
| Sound | Paula writes completed; 44,100 Hz capture contained 11,025 stereo frames with the expected tones |

The full Exampleageddon inventory contains 1,683 entries. Interactive demos,
other-platform examples, intentional failures, helper modules, benchmarks,
network examples, and third-party library examples retain their inventory
classifications. They are not counted as unattended runtime passes. All 66
sound example entries compiled; native audio behavior is covered by the
bounded probes above.

Local evidence is in `out/amiga/fbctests-final`,
`out/amiga/exampleageddon-final`, and `out/amiga/probes-workflow`. These
directories contain the exact snapshots used for the runs, rather than a
promise that later workspace edits have been qualified.

The later metadata changes also passed the file batch in
`out/amiga/file-final`. Linux regression checks passed 866 datetime assertions
and the existing raw-output/capture test. A SIGTERM during guest execution
returned runner status 130 and removed its emulator, virtual display, and
authority file; other applications' displays were left alone.

The ROM-only run qualifies the chipset and Paula paths. AHI and CyberGraphX
build successfully but still need runs with those components installed.
The captured WAV records samples accepted by the native driver; it does not
record the emulator's final host audio output. Real hardware timing and
physical serial/printer devices remain separate qualification work.

The October 16 corpus audit also exposed defects in the imported CPU arithmetic
helpers. The provider build repairs the verified assembly copy while retaining
the original pinned source and license notices. These repairs cover guard and
sticky bits, finite/nonfinite classification, overflow and signed-zero returns.
All 14 maintained native probes passed after rebuilding through the normal make
rules, including 100,000 hardware-oracle conversion/arithmetic comparisons and
eight additional signed-zero checks. This focused run does not replace the
earlier complete fbctests and Exampleageddon qualifications.

The corpus vector-field example exposed a further shared gfxlib2 failure:
nonfinite line endpoints could reach integer clipping and raise an Amiga
divide exception. LINE now checks transformed endpoints before conversion.
The graphics suite passed all 99 tests on both targets, with 928,199 host
assertions and 469,447 native assertions. The added regression covers NaNs,
infinities, out-of-range finite values and WINDOW transform overflow for the
line, outline-box and filled-box forms. The original vector-field source
then completed its diagnostic runs without modification.

The Araxia corpus run exposed excessive planar transfers during palette loading.
After adding the source-row cache, all 14 maintained native probes and all 99
graphics tests passed, including 469,447 graphics assertions. The native graphics
probe checks unchanged rows, repainting after a refresh message, and presentation
with the optional cache withheld at depths 8, 16 and 32. The exact snapshots and
results are in `out/amiga/corpus-20261015-8E77C4/presentation-cache-probes` and
`out/amiga/corpus-20261015-8E77C4/presentation-cache-gfx`.

The legacy image-header audit found a separate C/BASIC ABI mismatch. BASIC
places the pixel-size bitfield in the low three bits of its native word;
big-endian C placed it at the high end. The C declarations now preserve the
documented BASIC layout. New tests check metadata at the field boundaries
and drawing from a header constructed in BASIC. Both graphics suites passed
all 101 tests, with 928,395 host assertions and 469,643 native assertions, and
all 14 native probes passed again. Evidence is in the `host-legacy-header-gfx`,
`legacy-header-gfx-after` and `legacy-header-probes-after` subdirectories of
the October 15 corpus audit directory. This fixes native memory layout;
little-endian QB image files still need explicit decoding before native PUT.

A later custom-font check found a second legacy-header reader which treated
the two native 16-bit words as one 32-bit value. Drawing and measuring legacy
fonts now read those words separately. Both graphics suites passed all 102
tests, with 928,431 host and 469,679 native assertions. The font regression
covers an explicit eight-bit font and destination-depth inheritance when drawing.

The Prospector startup audit also found that generic CHDIR bypassed Amiga's
existing path wrapper. Parent traversal now follows the same conversion as
file operations. All 14 native probes passed again, including nested parent
paths, backslash separators, relative file lookup and empty-string safety.
Evidence is under `legacy-font-gfx-after`, `host-legacy-font-gfx` and
`chdir-parent-probes-after` in the October 15 corpus audit directory.

Prospector also exposed a missing foreground-driver poll in SCREENEVENT.
Applications using only that event API could leave native Intuition messages
unprocessed. SCREENEVENT now polls before acquiring the event-queue lock;
peeking still leaves the event available for retrieval. The new native queued
event test fails with exit 48 before the change and passes afterward. All 14
native probes and both 102-test graphics suites passed again. Evidence is under
`screenevent-probes-before`, `screenevent-probes-after`, `screenevent-gfx-after`
and `host-screenevent-gfx` in the same corpus audit directory.

KLOC exposed a native C formatting ABI collision: libamiga's legacy
`sprintf` forwards to Exec RawDoFmt, where `%d` consumes a 16-bit word. Newlib
and generated C pass 32-bit integers, so DATE$ became `00-10-0000` and TIME$
lost its minute and second fields. The Amiga runtime now forwards sprintf to
newlib's vsprintf. A deterministic probe fails with exit 40 before the change
and passes afterward. The new datetime unit checks integer, unsigned, floating
and string arguments plus return counts. The native datetime and CRT batches
passed (88 and 84 tests, including repeated common tests); the combined host
selection passed 93 distinct tests. All 14 maintained native probes passed.
The original KLOC fixture now emits a valid current date/time and identical
source metrics. Evidence is under `date-format-before`, `date-format-after`,
`date-format-probes-after`, `date-format-units-after`, `host-date-format` and
`fb-ext-subset-consumers-v3` in the same corpus audit directory.

The xInput example exposed a separate presentation problem: Intuition could
paint a title strip over a borderless private screen during WINDOWTITLE. The
Amiga driver now invalidates the cached rows and restores the application
canvas after the title update. Public RTG window decorations remain outside
the canvas. A native pixel test fails with exit 51 before the change and passes
afterward at depths 8, 16 and 32. All 14 native probes and the 102-test graphics
batch passed again (`title-colours-before`, `title-colours-after`, and
`title-colours-gfx-after`). Three complete original fb-ext sprite frames also
match GCC exactly, including all 921,600 RGB pixels (`fbext-sprite-colours`).

Corpus asset formats also require explicit byte-order handling. BASIC's binary
`GET` transfers native representations; it does not decode little-endian texture,
font or model files automatically. The audit's necessary BASICVERSE source port
preserves the checked original GCC outputs and decodes all 262,144 texture pixels,
97,280 glyph coverage bytes, 95 advances, 15,204,000 model weight words and
32,000 tokenizer records identically on Amiga. Its 11 entry points now compile
for both targets. A complete model inference step returns successfully and
selects the same token, although floating scores are not bit-identical.

The fb-ext corpus image loaders needed source fixes of their own. Their
packed BMP/TGA headers were little-endian data read as native structures. The
original BMP row converters and grayscale loops also read past row bounds;
TGA used pointer-sized pixel writes and could free borrowed raw-file memory.
ASan reproduces those original failures. The applied library port decodes
checked byte fields, bounds rows/RLE packets, owns its file buffers, and uses
32-bit native pixel words. RGB565 fields now follow the file's red/green/blue
masks. The original 24-bit BMP and real RGBA TGA host pixels remain identical.

All 159 loader cases pass on GCC and Amiga: 54 valid images with 626,530 exact
RGBA pixels and 105 malformed inputs rejected. Grayscale adds 330 full images
and 3,759,270 exact RGBA pixels across both color-key modes and 8/16/32-bit
screen contexts. The decoder-only and grayscale ASan runs report no memory
errors or leaks. A separate automatic-loader registration path retains its
preexisting 16-byte leak; it is outside the modified decoder code.

The library's test makefile now discovers a graphics suite. Its normal builds
pass six tests and 72 assertions on each target. An earlier 20-assertion run
was rejected as coverage because Testly's fatal assertion helper returned
false after successful assertions, skipping subsequent pixel checks. The new
tests use explicit guards and ordinary assertions. Actual BMP, TGA and grayscale
applications then display their original assets and quit normally on both
targets. Six necessary source/test/build files changed; other project files
and existing user edits are preserved by `fbext-applied-raster-port.json`.
Evidence is under `fbext-raster-candidate-v2`, `fbext-grayscale-candidate`,
`fbext-raster-asan`, `fbext-grayscale-asan`, `fbext-raster-normal-make-v2` and
`fbext-raster-actual` in the October 15 audit directory.

Correct software pixels do not imply identical true-color presentation on AGA.
The native true-color fallback uses a fixed RGB332 palette, whose blue channel
has fewer levels than red and green. A neutral source gray may therefore acquire
a visible tint. The pixel exports above check the full source RGBA values;
the maintained native display tests independently check the chosen palette
colors. Indexed modes continue to use the application's palette directly.

The full original BASICVERSE benchmark completed its 20 frames with native
DONE 0/COMPLETE and 14,748 triangles per frame (`basicverse-full-bench`). Reported
rates describe software emulation. Asteroids also returns zero after a diagnostic
native Intuition close message, with three complete 800x600 color frames
validated (`asteroids-close-diagnostic`). Its private chipset screen has no
clickable close gadget, so that result is recorded as diagnostic event processing
rather than an ordinary user-close observation.

Persistent application worker pools must stop cooperatively and join before
executable segments are released. The runtime allows BASIC global destructors
to perform that cleanup; it cannot safely terminate arbitrary application code
that is still using shared state. BASICVERSE needed its existing stop flags
connected to that lifecycle. Initialization failure and repeated shutdown are
covered by the corpus audit as well as the runtime destructor probe.

The corpus audit distinguishes unavailable native providers from compiler
failures. The requested scope is to document platform gaps and finish compatible
programs and FBC fixes; large dependency or application ports are separate work.
A missing provider, source-excluded main, or successful static-library build is
not an executable runtime pass.

| Corpus requirement | Current Amiga qualification gap |
| --- | --- |
| SDL2, SDL2_image, SDL2_ttf, gfxlib3 | No matching native providers in this SDK setup |
| OpenGL/GLU, Tilengine, FreeImage | No matching native graphics libraries |
| SQLite, FreeType, JPEG, GIF, PCRE | Required native providers are not all available |
| Win32 console/UI and X11 | Application APIs require native application ports |
| Unix socket and timezone bindings | Applications select unsupported OS-specific bindings |
| ZIP archive opening | Built provider needs secure entropy absent from this ROM setup |
| Fixed 64-bit pointer casts | Application assumptions do not hold for the 32-bit native ABI |

Some fb-ext examples also have incomplete library build recipes; unresolved
symbols remain build failures until the corresponding original modules are
built and linked. They are not automatically classified as operating-system
limitations. Each corpus target retains its exact compiler log and GCC control.

The later utility checks ran the original Library Master console and graphics
programs, both flight converters, the regression calculator, and Tic-Tac-Toe.
All returned zero through the native guest controller. The console programs
used real CON: handles; redirected input does not exercise INKEY. For the
regression fixture (1,3), (2,5), (3,7), both targets saved identical data and
results: correlation 1 and Y = 2 * X + 1. Tic-Tac-Toe's complete board transitions
and final win/draw messages were checked independently. These observations do
not establish all library-management or calculator paths.

Other current platform limits are explicit:

- Files use checked signed 32-bit native positions. Unsupported larger seeks
  fail rather than truncating the requested offset.
- Exec signals are wake-up bits, not POSIX fatal signals. CPU faults remain
  native Amiga alerts; automatic fatal-signal recovery is unavailable.
- Unix-style dynamic libraries and TCP are unavailable with the pinned SDK.
  Native Amiga library vectors require their own bindings.
- OpenGL, shaped/resizable graphics windows, and joystick polling are not
  implemented by this backend.

The October 16 corpus checkpoint records 98 of 184 inventory targets compiling
for Amiga, with 93 targets having verified execution: 77 ordinary runs and
20 diagnostic runs, with overlap. This is cumulative evidence from frozen
build snapshots, not a complete run with one latest compiler binary. The
86 documented gaps retain matching build-log hashes; 76 have successful GCC
controls, two have failed controls, and eight have no recorded control.

Two compatible original BASICVERSE programs were pending at that checkpoint:
the older island demo and the full 300-frame recording. The island subsequently
completed its original rendering loop and ordinary Escape exit with native
return zero. Cumulative coverage is now 94 targets with verified execution,
including 78 ordinary runs and 20 diagnostic runs, with overlap.

The recording's first six frames each had a maximum channel difference of one
from GCC. An actual recording checkpoint loaded into a filesystem isolation
that makes a writable copy available at the original guest path.
That copy continued from saved frame index 5 and produced the next complete
frame byte-identical to the live native recording. The preceding five files
were preserved. This qualifies the recovery method for this workload; the
diagnostic copy stopped after one new frame and does not establish completion
of all 300 frames. The original recording later reached 23 complete frames
before its host controller limit; its selected recovery state was at frame 22.

A managed continuation waits for the original host controller's configured
limit and verifies that its emulator has terminated before preparing recovery.
It restores into a private volume copy, preserves the inspected output prefix,
and removes later frames only from that copy so they must be recomputed. The
checkpoint selector verifies the executable, loop counter and completed-frame
hashes. Full recording coverage still requires all 300 original frames and
native DONE 0/COMPLETE; a successful recovery experiment alone is insufficient.

The continuation reached 226 complete frames before the October 21 machine
shutdown. It has no native completion marker. Its last available RAM checkpoint
was still at frame 22, so those later calculations must be repeated; the 226
images remain preserved for comparison. The next recovery uses a memory-backed
working volume, with completed images and two rotating, inspected save slots
on durable storage. Image storage may reuse a byte-identical earlier file only
after the native application has recomputed and validated that frame. The
application's original resolution, frame count and inference work are retained.

All 226 captured frames were compared without masks: 231,424,000 RGB pixels.
Of these, 2,623 differ by more than one channel value, and 142 differ by more
than 16; no frame has more than five of the latter. Traced original raster
equations explain three selected large differences: one-ULP camera-yaw changes
flip a leaf edge test or a nearly equal water/sand depth test. These are
different material selections, not channel-order corruption. Other flagged
pixels remain recorded for review, and these diagnostic cases do not establish
full recording completion (`record-colour-investigation` in the audit directory).

A further controlled check explains two bark-texture differences at frame 10,
pixel (700, 300), and frame 27, pixel (857, 480). Changing only GCC's camera yaw
to the observed native value, one ULP away, reproduces the native edge weights,
depth, UV coordinates and final RGB exactly. Their projected triangles cover
about 0.063 and 0.130 square pixels, respectively, making interpolation sensitive
to small projection changes. This is evidence for these two pixels, not a
blanket tolerance increase. The inputs and both controls are preserved under
`record-camera-yaw-control`. All of the initial ten completed pixel diagnostics
have a traced explanation. The buffered batch continues the remaining checks;
its current counts and unresolved cases are in `record-boundary-batch/trace-review.json`.

The initial pixel batch stalled while writing diagnostic console output. Its
saved state contains no ready task, with the application waiting in DOS output;
the cause is not established. That diagnostic was stopped after preserving its
state and partial trace. A separate rerun uses buffered file tracing, retaining
all 142 checks and adding shader operands. All 142 GCC control pixels remain
unchanged. The full recording continues separately with its original workload.

An isolated native diagnostic completed the original camera equations and
projection trigonometry for all 300 frame inputs. Its camera words match the
original pixel traces. Comparing 1,200 sine/cosine results against a 90-digit
reference, using each exact native binary32 argument, found 1,160 native values
equal to the nearest rounded result and 40 one representable float step away.
For the same inputs, the host library returned 1,189 nearest results and 11
adjacent results. These measurements cover the recording's projection angles,
not arbitrary libm inputs. GCC controls supplied with the native camera and
projection trig words reproduce further native edge weights, depth, texture
coordinates and final pixels exactly. The native arithmetic and controls are
preserved under `record-camera-trig-probe` and `record-observed-projection-controls`.

The buffered batch completed all 142 selected pixels and passed all three
independent scalar controls. Its review explains all 142 cases. The final four
bark-texture cases are reproduced exactly by supplying the exported native
static geometry and projection inputs to the unchanged GCC raster equations.
All integer material and flag fields match across the 12,992 static triangles;
their maximum position difference is approximately 0.00000191 world units.
The before/after allocator builds export byte-identical native geometry.
The same complete geometry export is also byte-identical with JIT disabled.
These are the large differences selected
from the first 226 captured frames, not a comparison of all 300 final frames.

## Allocator and emulator follow-up

The Amiga allocator previously moved and copied every reallocation, including
unchanged sizes. It now reuses bounded spare capacity and reserves additional
room for incremental growth. An unsuccessful reservation retries the exact
requested size, preserving the original block on failure. The allocation
header and public interfaces are unchanged. Size checks also leave room for
Exec's eight-byte block rounding, rejecting the seven unsafe sizes at the top
of the native address range before calling the OS.

The maintained allocator probe covers preserved contents, bounded moves,
shrinking, overflow, injected allocation failures and BASIC array clearing.
Native controls returned 40 before capacity reuse and 0 afterward. A second
control returned 68 for an unsafe forwarded request before the rounding guard,
and 0 afterward. Private libraries built with normal Make object rules retain
every other archive member from the qualified baseline.

An emulator discrepancy affected the wider checks: the exact same string-suite
executable passes with JIT disabled and fails one signed-boundary assertion with
JIT enabled. Replaying the previously passing reference executable also
reproduces the JIT failure. This is independent of the allocator change. The
arithmetic probe passes under the interpreter, and the 300-camera diagnostic is
byte-identical in both modes. The allocator candidate therefore uses interpreter
mode for its regression gates. All 16 maintained native probes passed with the
October 20 compiler baseline and candidate libraries. All 37 frozen fbctests
groups also passed: 5,264 tests and 1,167,113 assertions, with no failures or
errors. These results do not qualify unrelated current
working-tree changes. Evidence is under `allocator-full-probes`,
`allocator-full-fbctests`, `allocator-string-interpreter` and
`allocator-string-reference-replay` in the corpus audit directory.

The file-query audit also found a native ABI defect: DOS requires four-byte
alignment for FileInfoBlock, while the NDK type and 68k C stack require only
two bytes. An aligned internal typedef now makes automatic query buffers meet
the DOS boundary without changing their layout. A guarded native regression
passes the ordinary stack path before the fix but rejects a word-aligned stat
buffer with exit 83. Afterward both paths pass, with all 47 DOS queries aligned.
All 15 maintained probes passed, as did the file suite's 133 tests and 7,260
assertions and the graphics suite's 102 tests and 469,679 assertions. This
establishes the alignment fix, not the cause of the earlier historical seek
stall. Evidence is under `fileinfo-alignment-review`, `fileinfo-probes-final`,
`fileinfo-file-tests` and `fileinfo-gfx-tests` in the audit directory.

## October 20 source restructure qualification

The compiler and Amiga libraries were rebuilt after the source restructure and
the Amiga compatibility changes in `d7cd78367`. Both qualification runs used
the same frozen compiler and library inputs. The compiler SHA-256 was
`6fe69dcc24788a9f07429242e4f5280b8dcf09d9a9af36a15ac136a99a23908c`.

All 37 fbctests groups compiled and passed under FS-UAE: 5,264 tests and
1,167,113 assertions, with no failures or errors. Exampleageddon inventoried
1,773 entries; 1,318 compiled and all 654 self-contained examples passed in
the guest. The remaining 455 compile failures belonged to other categories.
Those categories were not required guest runs, so this does not establish
execution coverage for interactive examples or unavailable native providers.

The first Exampleageddon run launched the serialization file example without
its documented filename argument. The Amiga adapter now carries the shared
`EXAMPLE_RUN_ARGUMENTS` metadata into its guest manifest. Resuming with
`saved-state.bin` passed that example and left zero required failures. The
initial report is retained alongside the final report in
`out/amiga/refactor-exampleageddon-20261020`; fbctests evidence is in
`out/amiga/refactor-fbctests-20261020`.

<!-- end of amiga.md -->
