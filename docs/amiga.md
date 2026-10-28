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

## Networking

`OPEN TCP`, `OPEN TCP SERVER`, `TCP ACCEPT`, `EOF`, `EOC`, and `LOC` use
version 4 of `bsdsocket.library`. A TCP/IP stack is required at run time;
ordinary programs do not open this library. The adapter uses IPv4 and the
original resolver vectors, so it also works with UAE's version 4 interface.
It does not call newer Roadshow `getaddrinfo` vectors.

Amiga socket descriptors and signals belong to the task that opens the
library. Each BASIC task keeps its own library context in TLS. Idle sockets
live in the native public pool and are borrowed into the calling task for
each operation. This permits a listener to be opened in one thread, accepted
in another, and closed after the creator exits. The private table supports
64 simultaneous sockets and reports exhaustion through the normal file error.

Receive buffering keeps byte-wise BASIC protocols from paying for a native
socket call on every byte. EOF and EOC preserve unread buffered data. Actual
reads wait for packet fragments with the BASIC file lock released, allowing
another BASIC thread to supply the remaining bytes. Protocol timeouts bound
those waits when specified.

To enable UAE's socket interface and include the native TCP qualification:

```sh
build_scripts/amiga-run-probes.sh --bsdsocket --jit --timeout 240 \
    --output out/amiga/probes-with-tcp
```

The TCP probe runs the shared text/binary, burst and wildcard-listener tests,
then checks a listener after its creating thread exits and formatted INPUT
split across delayed writes. It is compiled with `-g` so assertions execute.

## OMA programs

The maintained source entry points available under `OMA` can be frozen and
cross-built together:

```sh
python3 build_scripts/amiga-build-oma-games.py --output out/amiga/oma --jobs 4
```

The builder defaults to optimized ordinary runtime builds. Duel uses `-mt`
because it creates a BASIC server thread. `--checked` enables additional
runtime error and array-bounds checks. Each result records its compiler
command, source paths and asset directory.

The October 27 run reached gameplay in all nine available maintained titles,
including all four kinematics variants, for twelve native programs. Its
per-program executable hashes, native display captures and input records are
listed in `out/amiga/oma-20261027/manifest.json`. This qualifies reaching
gameplay, rather than complete playthroughs or every game feature. The six
unavailable historical titles were excluded by the user.

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

For interactive pointer tests through Xvfb or VNC, add `--mouse-integration`.
This selects UAE's absolute mouse input and disables pointer grabbing inside
that private display. Relative grabbing can otherwise deliver button events
without motion. See the [FS-UAE mouse documentation](https://fs-uae.net/docs/mouse/).
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
- Unix-style dynamic libraries are unavailable with the pinned SDK.
  Native Amiga library vectors require their own bindings. TCP uses the
  optional `bsdsocket.library` version 4 interface described above.
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

## October 21 corpus audit recovery

The active `out/amiga/corpus-20261015-8E77C4` directory was removed during
execution. Both its original recording controller and its expanded colour
diagnostic subsequently exited. The user requested recovery and continuation.
Historical results above describe completed checks before that removal; many
of their detailed artifacts are no longer available at the listed paths.

Surviving inputs are preserved outside the build output tree in
`/home/jkfirth/fbc-audit-recovery-20261021`. Its `salvage.json` records the sizes
and SHA-256 hashes of the original native recording executable, model and
texture assets, 25 complete original-resolution frames, an earlier RAM state,
and the expanded diagnostic executable with all 2,623 coordinates. Only those
25 recording images were recovered; the earlier set of 226 images has not
been recovered. The saved original executable has SHA-256
`400939f4db974d587471883e6e3f0c6e13a5400f653a634dff356a04253c675e`.

Offline inspection identifies the surviving RAM state at frame 22, with the
main task waiting for rendering workers and runnable workers present. Recovery
preserves frames 0 through 21 in a private volume. Later salvaged images remain
separate evidence while the unchanged application recomputes frames 22 onward.
The recording retains its original 300 frames, 1280 by 800 resolution and
language-model inference. A restored snapshot alone does not establish native
completion. That still requires all 300 complete output images and the native
controller's `DONE 0` and `COMPLETE` records, followed by colour review.

Recovery also located the same 184-target corpus registry and all 184 historical
GCC artifacts outside the removed audit directory. Their build-result metadata
survives, but nearly all original build logs are missing. These artifacts alone
do not establish fresh compilation or native execution.

The maintained fb-ext raster suite was rebuilt from unchanged frozen corpus
sources through its module and test Make rules. Both GCC and native Amiga
passed all six tests and 72 assertions, with no failures or errors; the native
controller recorded return zero and COMPLETE. The checks cover BMP row order,
palettes, RGB565 masks and top-down alpha; raw and RLE TGA; truncated and invalid
inputs; and grayscale in 8-, 16- and 32-bit screen contexts. This fresh result
uses the frozen October 20 compiler and native libraries. It does not qualify
unrelated current working-tree changes or recreate the larger lost loader
matrix. Evidence is in `fbext-raster-requalification-v3/qualification.json`
under the recovery directory.

An unmasked comparison of the 22 preserved native prefix frames against
completed individual images from the rebuilt GCC control checks 22,528,000
pixels. There are 168 pixels differing by more than one channel step, including
eight differing by more than 16. All 168 coordinates exactly match the preserved
original diagnostic case list for those frames. This restores the prefix
comparison evidence; it does not establish full recording completion or replace
numeric review of the differences. Raw RGB values and hashes are retained in
`recovered-prefix-review/review.json` and its per-frame difference files.

The rebuilt GCC recording subsequently completed all 300 original frames and
its original inference step, with exit zero. Its source, compiler, libraries,
assets and frame hashes are preserved in `host-reference`. A separate rebuilt
GCC diagnostic evaluated all 2,623 preserved coordinates. Twelve independent
scalar controls match every edge, depth, UV, winner and raw-colour event from
that diagnostic, including all eight large differences in the recovered prefix.

Fresh native traces show edge or depth selection changes at seven of those
eight pixels. For the remaining bark pixel, frame 10 at (700, 300), replacing
only GCC's yaw with the observed native word, one representable Single step
away, reproduces the native triangle weights, depth, UV, winner and raw RGB
exactly. The source comparison verifies that this yaw assignment is the only
change. These are raster explanations for the eight selected prefix pixels;
they do not prove bloom/overlay equality or completion of all native frames.
Evidence is in `numeric-scalar-validation/validation.json`,
`observed-yaw-replay/validation.json` and `prefix-large-colour-review.json` under
the recovery directory. The larger native diagnostic and original recording
continue independently.

Eight original corpus CLI programs were subsequently rebuilt and re-executed
with paired GCC controls: the four fbJson examples (including its regression
program), ChessbrainFB's UCI exchange, fb-linter help, Kiwi's URL parser, and
fbeauty's source formatting. All eight returned zero natively and matched the
complete normalized GCC stdout. Both fbJson archive builds remain separate
library-build evidence. These repeat existing inventory targets, so they do
not increase the historical distinct-target total. Frozen sources were checked
unchanged after execution. Evidence is in `cli-requalification`.

A direct operand audit also reconstructs observed binary32 raster arithmetic.
The captured GCC trace has 2,588 exact geometry checks and 2,542 eligible exact
shade checks; the captured native prefix has 464 and 431 respectively, with no
mismatches. Water paths, fog requiring unlogged direction inputs, sky-only
pixels and ambiguous clipping fans are explicitly outside individual checks.
This validates those observed calculations, not full colour parity or the
unobserved upstream inputs (`raster-operand-review/review.json`).

Historical tool receipts recovered from this conversation independently retain
the pre-removal cumulative counts: 184 targets, 98 native compilations, 78
ordinary executions and 20 diagnostic executions, covering 94 distinct targets.
`historical-native-coverage-v2/latest-summary.json` identifies its original
call, output and timestamp. These are archived observations, not fresh runs or
recreated per-target files; missing underlying artifacts remain explicit.

The restored linter executable also completed eight additional paired native
workflows: version, catalog, rule explanation, file diagnostics, warning exit,
stdin diagnostics, repair dry-run and repair application. Diagnostic output,
intended exit codes and resulting fixture bytes match GCC. The version and
ruleset match; separate compiler build timestamps are validated and retained
rather than reported as identical. Semantic integration is explicitly disabled
on both sides for these lexical and repair checks.

A guard rejected the historical fixture as repair coverage because it produced
only a non-repairable warning. A separate disposable whitespace/final-newline
fixture then established a nonempty repair plan, an unchanged dry-run file,
and actual applied fixes. The real corpus sources remain untouched. Evidence
is in `linter-requalification/results.json` and `completion.json` under the
recovery directory. These workflows repeat one existing corpus target.

The native static mesh was also recovered read-only from the recording's owned
RAM snapshot. One validated array descriptor identifies the original 148-byte
triangle records. Exporting its first 12,992 static triangles with the original
mesh header produces exactly 1,922,828 bytes and SHA-256
`a1a8e0c6aaae8abc5d80dbf81216e4872d2c949c399d167d879e692d0cf63daa`,
matching the previously qualified native export byte for byte. This restores
lost replay data without rerunning scene generation, changing guest memory or
feeding cached data to the original recording. Source-state and descriptor
provenance are in `recovered-native-mesh/recovery.json` under the recovery
folder.

### Completed recovered numeric colour diagnostic

The expanded native diagnostic completed all 2,623 unique preserved coordinates
with native DONE 0/COMPLETE. Its closed trace is preserved and hashed. All 2,623
raw RGB values and winning states (triangle, material, colour and depth, or sky)
match the separate GCC replay exactly when supplied with the observed native
static geometry, camera and projection words. No mask or tolerance is applied.
Before substitution, the replay reproduces all 186,894 ordered baseline raster
and camera events for the same 2,623 cases. All 125 camera frames in the actual
pixel trace match the independent native camera diagnostic.

The independent camera diagnostic covers all 300 original camera inputs and
1,200 projection trig values. A 90-digit comparison using each exact native
binary32 argument finds 1,160 native results nearest-rounded and 40 one float
step away. Host C `cosf`/`sinf` at those same arguments produce 1,189 nearest
results and 11 adjacent results. These measurements cover the observed camera
argument domain, not arbitrary libm inputs.

The completed native operand audit has 2,584 exact eligible geometry checks and
2,538 exact eligible shade checks, with no mismatches. Unlogged water/fog inputs
and sky-only cases remain outside those individual checks. The input replay
has one dynamic billboard case whose weights/UV are not bit-identical although
its raw colour, winner and depth are identical; it is retained explicitly.
Thus these results establish the checked raw raster behaviour, not universal
intermediate-bit equality or full-frame bloom/overlay parity. Evidence is in
`completed-colour-diagnostic-audit.json`, `native-input-replay`,
`raster-operand-review-complete` and
`camera-trig-requalification/precision-validation.json` under the recovery
folder.

### Original recording remains in progress

Read-only inspection of the owned RAM snapshots confirms progress within
original frame 22. Generated SKY_ROWS code reuses its incoming y0 stack slot as
the current row counter; the instruction at text offset 0x2848 increments it.
Earlier and later snapshots contain changing normalized ray inputs and later
row counters. The next inspected checkpoint has moved from DRAW_SKY to RENDER,
verified against the original executable's call sites. This is real in-frame
work, not a completed frame or completion of the original 300-frame workload.
The unchanged recording continues with rotating durable checkpoints. See
`sky-work-progress.json` for snapshot hashes and bounded stack observations.

The original continuation has since recomputed frame 22, byte-identical to its
recovered predecessor, bringing its durable map to 23 images including the
preserved 22-frame prefix. A separate full-length GCC composite replay retains
all 300 frames and original inference. Its first complete baseline matches all
300 stock GCC image hashes. The first controlled run was superseded after a
diagnostic injection-order error: yaw inputs were replaced after `to_camera`
had consumed them. The corrected v2 replay reruns both complete modes; no
original corpus source or active native executable is changed.

A save-slot navigation error also wrote an extra snapshot into private slot 9.
The log records no additional restore and the canonical input outside the UI
slot directory is unchanged. That extra save was preserved. An independent
observer now retains four verified checkpoint copies outside the UI filenames;
transient service-status query failures are treated as unknown state. A separate
terminal audit checks actual DONE 0 and all 300 original-resolution PPMs before
reporting completion, including any last file missed by the old collector's
two-second stability interval. Original native computation continues separately.

The first late-yaw controlled composite run is explicitly marked superseded;
its unchanged-input baseline remains valid. The corrected v2 applies each yaw
pair before yaw expressions and each pitch pair before pitch expressions. It
retains both full 300-frame runs and original inference. Composite observers
continue independently until actual native completion, rather than stopping
when the host replay finishes. Service-status read errors now preserve unknown
state and continue observation instead of being mistaken for job termination.

### Full-length composite replay, corrected and completed

Both corrected v2 GCC modes completed all 300 frames and the original inference
at frame 205 with exit zero. Independent rehashing verified all 600 output
images. Every baseline hash equals the original stock GCC recording. The
inference source is unchanged, and inspection verifies that yaw/pitch words are
substituted before their respective expressions consume them.

All 25 preserved native composite images were then compared without masks,
covering 25,600,000 RGB pixels including bloom and overlays. Against stock GCC,
169 pixels differ by more than one channel value, including eight by more than
16. Against the corrected native-input replay, none differ by more than one;
the maximum remaining channel difference is one. These are raw descriptive
measurements, not a changed tolerance or automatic whole-corpus pass. Native
images were neither modified nor replaced. The audit is
`full-frame-input-replay-v2/audit.json`, with per-frame measurements in
`full-frame-replay-review-v2/review.json` under the recovery directory.

The original native continuation remains separate and active. Its durable map
has 23 images: 22 preserved checkpoint-prefix images plus the recomputed frame
22. The latest inspected checkpoint is at frame 23. Completing and comparing
all 300 original native frames is still required.

### Restored complete image-loader regression matrix

The larger BMP/TGA matrix was also rebuilt against the frozen original
providers and executed on both GCC and Amiga: 54 valid images with 626,530
exact RGBA pixels per target, and 105 malformed inputs rejected on each. All
159 case identities and native DONE 0/COMPLETE were verified. The driver calls
the real public loaders and exports actual image words using explicit channel
shifts; it contains no decoder or expected-pixel substitute. All alpha bytes
are included, with no tolerance or masks. This is null-display decoder
qualification, not physical palette presentation.

Restoring the initial fixture generator exposed an oracle-version discrepancy:
eight synthetic 16-bit TGA cases rounded five-bit expansion, while the library's
legacy conversion and independent Pillow decoder truncate. Only those expected
bytes were regenerated. Every encoded input and actual decoded output remained
unchanged; all corrected TGA16 expectations were independently checked against
Pillow. No compiler, decoder or corpus source change was needed. The result is
`fbext-raster-matrix/completion.json`; the original expectations and regenerated
`fbext-raster-matrix-oracles-v2` remain available for comparison.

The original continuation has now also recomputed frame 23, byte-identical to
its recovered predecessor. Its durable map contains 24 images including the
22 preserved checkpoint-prefix images. Full native 300-frame completion remains
pending.

The restored full grayscale matrix also passes on both targets: all 54 decoded
valid images plus a 15-pixel alpha/colour-key boundary fixture, transformed in
both keying modes under 8-, 16- and 32-bit null-display contexts. That is 330
complete output images and 3,759,270 exact RGBA pixels per target. Alpha is
preserved, colour-key behaviour is checked explicitly, and all native case
identities and DONE 0/COMPLETE are verified. Expected channels use independent
binary64 luminance arithmetic over the validated source RGBA oracles. No
compiler, decoder, grayscale implementation or corpus source change was needed.
Evidence is in `fbext-grayscale-matrix/completion.json` under the recovery
folder; this remains image-processing qualification, not physical display
palette qualification.

Additional recovery indexed 66 exact historical stdout receipts containing 18
structured project records and 188 project-specific text observations, covering
118 distinct inventory targets. Records retain original call/output line
numbers, timestamps and tool exit status. Several failures precede later fixes,
so these observations must not be flattened into a current pass/fail table.
The original full 184-target matrix has not been reconstructed from fragments.
The index is `historical-project-receipts/index.json` under the recovery
folder, with the exact stdout files alongside it. These receipts add provenance
without counting new compilations or executions.

The previously unresolved PCEM recipe was also investigated using a fresh
snapshot of 241 unchanged source inputs. Its normal `build.py` GCC recipe
passes with the frozen compiler and an explicit include/runtime prefix. The
recipe's existing `vendor/omaGUI` include path removes the earlier native
missing-header error. Native BASIC compilation then rejects pointer conversions
through `ULongInt` on the 32-bit m68k target, while a separate native compile of
the real network bridge fails at its POSIX `dlfcn.h` dependency. These identify
a source platform interface gap; they do not demonstrate an FBC regression.
No corpus or compiler source was changed. The successful GCC build and two
native preflights do not imply a native execution pass. Commands, source hashes
and exact diagnostics are in `pcem-recipe-requalification/audit.json` under the
recovery folder. The earlier private GCC include-path failure is retained
separately from the successful control.

### Restored native display colour readback

Fresh native display observations also confirm the AGA palette distinction.
All 256 grayscale inputs match the independently calculated RGB332 component
levels when read through native `ReadPixel` and `GetRGB32`. A 32-bit source
gray of `(128,128,128)` becomes colour-map RGB `(146,146,170)`, explaining a
visible tint despite correct source pixels. No image pixels or conversion
rules were changed. This observes the emulated native display, not a physical
monitor's calibration. Evidence is `aga-palette-requalification/completion.json`
under the recovery folder.

The complete frozen maintained graphics probe passes again at depths 8, 16
and 32, including 256 mixed-channel samples per depth, live indexed-palette
updates, page copying, title preservation, cache refresh, queued input and the
oversized canvas. Both jobs return zero with native DONE 0/COMPLETE and JIT
disabled. `native-graphics-requalification/completion.json` records the exact
compiler, library archives, observer and executable hashes. These checks do
not qualify concurrent root-tree changes or complete the original 300-frame
native recording.

The continuation has since completed frame 25, its first output beyond the
25 salvaged images. The actual native file and durable copy both match the
corrected native-input GCC frame byte for byte across the full 1280x800
composite. Stock GCC differs at one pixel by up to 72 channel values; complete
image equality with the controlled replay explains that difference without
masking pixels. `continuation-r6/first-new-frame-proof.json` retains hashes,
coordinates and raw channel values. The reviewed native total is now 26 images
and 26,624,000 RGB pixels, including 22 preserved prefix images and four frames
recomputed since the checkpoint. The original 300-frame native run remains
incomplete.

### October 27 recovery after reboot

The old transient services and FS-UAE processes were gone after reboot, and
their temporary working volume was lost. Rehashing the newest protected state
and every archived frame verified a recoverable checkpoint at frame 32 and
32 complete prefix images. The original unchanged recording resumed in
`continuation-r7`, using an authenticated isolated Xvfb display and a durable
working volume inside the recovery directory. Its executable, 300-frame loop,
1280x800 output and later inference workload are unchanged.

The new run has produced an independently inspected and protected live
checkpoint. One controller now sends paced save-menu keys; the earlier
independent menu helper is not running. Full-frame review and actual native
completion auditing have resumed. Rechecking all 32 images covers 32,768,000
RGB pixels, with no controlled difference above one channel value. This does
not complete the original native recording. `recording-current.json` under
the recovery directory identifies the current units, paths and checkpoint
index; verify live process identities before any subsequent restart.

The unfinished portable fb-ext batch also passes: 15 unchanged examples and
17 execution cases with paired GCC controls, actual native exit records and
staged executable hashes. Its private Make recipe now includes the real
`ext-error` and `ext-memory` dependencies. Deterministic stdout is compared in
full; shuffled-bit population and toggles, iterator key/value multisets, sorted
rationals and thread request/response invariants are checked explicitly.

The initial bit-array checker confused bit indices with character positions.
The original `load` and `dump` methods reverse display order, so toggling bits
30, 31 and 32 changes displayed positions 97, 96 and 95. Only that checker was
corrected. The saved native jobs were revalidated without another native run
or rebuild. Evidence is `fbext-cli-requalification/completion.json` under the
recovery folder. These repeat historical targets and do not increase the
historical distinct-execution count or qualify the entire fb-ext library.

The resumed recording has now completed frame 32. Both its actual output and
durable copy match the corrected native-input GCC image byte for byte across
the full 1280x800 composite. This is the first completed frame after restoring
the protected checkpoint following reboot, with the original executable still
unchanged. Stock GCC differs at 27 pixels by more than one channel value, with
a maximum difference of four. `continuation-r7/resume-first-frame-proof.json`
records the evidence. The reviewed native total is 33 frames and 33,792,000 RGB
pixels; full native recording completion remains pending.

### File-hash regressions exposed by fresh corpus runs

Empty-file tests of the original MD5 example hung on the GCC control. The
MD5, SHA-2 and SHA-3 File wrappers all selected a zero-sized block, then looped
forever comparing zero remaining bytes against that block size. Explicit zero
block sizes also failed to make progress. These are library source defects;
changing FBC's valid comparison semantics would be incorrect.

The necessary corpus corrections reject zero block sizes before opening the
file and use a one-byte buffer for empty input so existing finalization runs
without updates. Expanded checks found a second source defect: SHA-3's partial
update loop consumed an extra byte in reverse order, reading past its input.
It now consumes exactly the supplied bytes and permits zero-length updates
without reading memory. Compiler implementation code was not changed.

All 96 independent file-digest cases pass on GCC and Amiga, covering MD5, all
SHA-2/SHA-3 digest sizes, empty/binary inputs and block boundaries. The six
original file, console and checksum examples also pass twelve paired cases.
Their example sources are unchanged; they link the corrected real hash module
through its existing Make rules.

Portable tests were added to the library's existing hash directory and are
discovered automatically. Full maintained hash/HMAC suites pass on both
targets: three suites, 13 tests and 194 assertions, with zero failures/errors.
An existing big-endian SHA-384 fixture typo was corrected from `6EC3` to `6ECE`
to match the little-endian bytes and independent expected digest. The first
failure, original source, exact focused patch and qualification results are
retained under `corpus-hash-fixes.json`, `file-hash-source-fix-v2`,
`file-hash-requalification-fixed` and `maintained-hash-regressions-v2` in the
recovery folder. Unrelated corpus edits were preserved.

The original native recording has also completed frame 33. That full image
matches the corrected native-input GCC replay byte for byte. The reviewed
native total is 34 frames and 34,816,000 RGB pixels, with two frames recomputed
since the frame-32 reboot checkpoint. A protected frame-33 checkpoint now
exists. Full 300-frame native completion remains pending.

### Original BMP viewer colour cycle

The original BMP viewer has also completed its eight-image cycle on GCC and
Amiga with its source and requested 640x480 resolution unchanged. All eight
complete software canvases match byte for byte across both targets, including
stored alpha: 2,457,600 RGBA pixels. Independent source decoding verifies every
displayed image pixel. The packed 16-bit cases use the loader's documented
nearest-integer channel expansion; Pillow's truncation differs by one, so the
validator independently calculates the exact declared result from the packed
words and masks. No pixels, tolerance or masks were changed to obtain equality.

A read-only observer forwards the original sleep calls. Real Escape events
on owned authenticated displays end the cycle; neither drawing nor keyboard
queries are replaced. The required real loader-registry source was added to a
separate Make-built partial graphics archive. Existing qualified archives and
all original viewer/image files are preserved. This observes software canvases;
AGA display-palette quantization remains a separate tested limitation.
`bmp-example-observation/completion.json` contains the full evidence, and its
lossless preview has separate pixel-identity provenance.

The original grayscale example is now checked across both full 640x240
screens. Real Space/Escape input advances its original waits. A private
identity observer records the unchanged elapsed numeric value; only a separate
GCC counterfactual imports that value for the timing caption. Both complete
native RGBA canvases match that replay exactly, including the intro text,
covering 307,200 pixels without masks or tolerance. The final canvas also
matches ordinary GCC and an independent luminance/keying calculation across
all 153,600 RGB pixels. Its original edge clipping is preserved. The actual
corpus source, native timing calculation and image operations are unchanged.
Source differences and raw observations are retained under
`grayscale-example-observation/completion.json` in the recovery folder, with a
lossless software-framebuffer preview and separate pixel-identity record.

The original TGA viewer also passes at its unchanged 800x600 resolution.
All 480,000 RGBA canvas pixels, including the elapsed-time caption, match a
separate GCC replay of the native numeric timing value. The native timing
expression is observed through an identity helper and stays unchanged. The
source image's 11,340 RGB pixels match independent decoding. Real Escape input
ends the original wait; no image output is fed back into the native program.
Evidence is in `tga-example-observation/completion.json` in the recovery folder.

Fresh `fb-imageproc` checks are running through
`imageproc-example-observation`. Grayscale, flipping, all four channel views,
colourizing, gradient and blur match GCC byte for byte across every original
complete RGBA canvas. Separate calculations verify the full grayscale, flip
and RGB/HSV/HSL/HCY channel canvases, including blank areas, alpha, original
placement and clipping. A read-only sleep observer forwards the unchanged
default wait, and real keys on owned authenticated Xvfb displays advance the
examples. Pending programs remain unqualified until their complete observations
and actual native completion records have been checked.

The original BASICVERSE `fonttest` also passes with its normal `-O 2 -mt`
build. Its entire 1280x800 PPM matches GCC and an independent rasterization of
the seven original text rows and bitmap-font data. The source and output
colours are unchanged. `basicverse-font-requalification/completion.json`
records all 1,024,000 exact RGB pixels; this separate finite example does not
qualify or replace the original running 300-frame recording.

All four original console mini-games have fresh GCC/native session checks.
Guessing exercises all six unsuccessful attempts. Rock-paper-scissors checks
every printed random choice, round count, score and result. Tic-tac-toe checks
its complete board history, supplied/rejected moves and terminal result.
The adventure checks quit/retry, its winning route and all three death endings
with complete matching transcripts. Randomization and original waits remain
unchanged; these sessions do not claim every possible gameplay sequence.

All 27 unchanged `fb-learn-opengl` lessons build with the frozen GCC compiler.
Every native link reports missing `-lGL`, and the current Amiga display driver
explicitly rejects OpenGL context requests. These are documented provider gaps
under the approved scope, with individual source hashes and actual diagnostics
in `opengl-provider-requalification/index.json`. No native OpenGL execution or
renderer qualification is claimed.

The original shared-image example also passes with the documented
`FBEXT_NO_EXTERNAL_LIBS` setting on both targets and the real BMP/TGA provider
archive. All 76,800 RGBA pixels match an independent calculation. Its default
`Image.Display` operation is XOR, including stored alpha: opaque red XOR
opaque black leaves red with alpha zero. The initial expectation of opaque
red was corrected against this existing API; saved program outputs were
unchanged and were validated without rerunning either program. The example's
image allocation, display and shared-pointer ownership remain intact.
Evidence is in `shared-image-example-observation/completion.json`.

The original `fb-imageproc` convolution, Gaussian blur, gradient-map and both
scaling screens also match GCC exactly. Ten complete programs now account for
fourteen complete canvases. The blend example's first emulator exited with its
native protocol unfinished; its three complete screens remain partial evidence.
A separate full six-screen execution now completes with all canvases matching
GCC exactly. It reuses the verified original binary/resources, records the
emulator exit status and supplies no saved frames to the guest. All eleven
programs and twenty canvases now pass, totaling 15,652,800 exact RGBA pixels.

The original list editor now has a functional GCC/native session: real Tab,
Space and Ctrl-Q input selects and toggles an item, then the actual saved
Markdown files match byte for byte. All three complete 640x480 RGBA screens
also match, including text and alpha, totaling 921,600 pixels. The English
translation asset must be beside the executable because its loader uses
`EXEPATH`. The first GCC control placed it only beside the working directory
and fell back to Dutch. That control is retained; the correctly staged GCC
control matches the preserved native screens without rerunning the native
program or masking text. Corpus source is unchanged.

`fbfrog` required one necessary corpus assertion correction in `src/tk.bi`.
Its compact internal token has a 10-byte natural C/BASIC layout on Amiga and
a 16-byte layout on the host. The original equality asserted desktop tail
padding. The guard now enforces its intended memory ceiling, leaving every
field and packing rule unchanged. Original normal Make builds reproduce the
GCC pass/native assertion failure; the corrected source passes normal builds,
all 5,519 maintained unit checks per target, actual C/BASIC layout probes and
complete matching header translation. The applied header matches the qualified
source snapshot byte for byte. Evidence and the focused patch are under
`fbfrog-requalification`; no compiler change was needed for this correct ABI.

The two logging examples have a verified source platform gap: their real
datetime provider has no Amiga timezone branch and fails on undeclared
`trailer` and `tzoffset`. Normal GCC builds run and their messages/XML validate,
but their saved UTC-labelled timestamps are about one hour ahead of actual
write times. Both findings are retained under `logging-provider-requalification`;
no timezone value or successful native execution is fabricated.

The long recording and blend emulators exited before completion during parallel
qualification. Their unfinished native protocols and quit logs are retained;
the cause is unproven. The recording has resumed as
`fbc-amiga-record-continuation-r9.service` from the independently protected R7
frame-38 state. All 38 prior images and the original executable are hash-checked,
and actual restoration plus a new live checkpoint are verified. R9 preserves
the original 300 frames, resolution, inference and R7 emulator settings. Its
checkpoint, native-completion and full-colour observers are active. An R8
launch variant did not establish restoration and is not qualified evidence.
Use `recording-current.json` in the recovery folder for current identities;
scope emulator cleanup to the owning job's PIDs when other audits are running.

The original spreadsheet also passes a real edit/recalculate/save session.
Its saved workbook changes B2 from 7 to 9 while retaining B1's `=B2+B3`
formula and B3's value 5. All six full 640x480 UI canvases and complete saved
files match GCC, totaling 1,843,200 RGBA pixels. Native previews show the
formula changing from 12 to 14; sampled formula pixels are RGB(85,255,85),
matching the source palette. No editor source changes were needed.

Schrijf's current GCC build succeeds, while its native link requires missing
X11. Its icon and screen-snapshot module selects X11 for every non-Windows
platform, so a native window interface or explicit feature-support policy is
required. Actual diagnostics and source hashes are retained; no X11 stub or
successful native run is substituted.

The fresh evidence index now records 58 programs across 73 paired cases,
37 documented current platform gaps and 87 entries with historical evidence
only. Library-only builds and the unfinished original recording remain
separate. R9 has produced its first new frame after restoration, bringing the
reviewed recording to 39 complete images; original 300-frame completion is
still pending.

An independent `fb-ext-lib` colour audit found a shared GCC/Amiga bug in
`changeColor`: it treated image width as the row stride. Padded images could
retain old pixels and have their padding changed. Twenty of 24 row cases
failed on both targets. The routine now indexes each active row using the
image's actual pitch and preserves the first metadata row for font buffers.
Null/empty buffers and invalid row dimensions are guarded. Matching and alpha
semantics are unchanged. This necessary library correction is in
`src/modules/graphics/libext-gfx-manip.bas` in the corpus, with maintained
regressions in `tests/graphics/raster.bas`; the compiler's aligned image layout
was correct.

The complete maintained graphics suite now has eight tests and 382 assertions.
The new tests reproduce 54 failures with the old provider and zero with the
fix on both GCC and Amiga. All 24 independent row cases pass after the fix.
A separate 100-case alpha audit, covering 3,650 RGBA pixels per target, also
passes, as does the unchanged vector example. The initial suspicion about
alpha-expression grouping was disproved: both compiler targets evaluate the
shift/add expression as intended, so no alpha routine change was made.

The original two-screen colour-manipulation example passes with the corrected
provider, real Space/Escape input and unchanged source. Both full 320x240
RGBA canvases match GCC, and all 20,000 image pixels match independent colour
and font-row expectations. Its aligned rows retain their existing behavior.
Evidence, original/candidate hashes and the focused source patch are under
`color-row-fix` in the recovery folder.

The recording emulator exited again after preserving 40 complete images.
R10 resumes the latest independently protected frame-39 state and must
recompute the last completed frame; that retained image is not supplied as
guest input. Actual restoration and a fresh checkpoint are verified. Host
signal observation and structured emulator exit records now investigate the
unexplained quits, and checkpoint spacing is ten minutes. The original
recording executable, all 300 frames, resolution, inference and emulation
settings remain unchanged. After the October 27 kernel panic and reboot, R11
restored the independently protected frame-40 checkpoint and all 40 legitimate
prefix images. Actual restoration and fresh checkpoints are verified.
`recording-current.json` identifies the active R11 services. Old process and
session identifiers are invalid after reboot.

The current evidence index records 95 original programs across 117 paired cases
and 66 documented platform gaps. Sixteen current paired builds await complete
runtime/colour qualification; 4 targets retain historical evidence only. The original animated alpha example's first
complete 320x240 RGBA canvas matches a separate GCC control exactly. Native
clock inputs are observed and returned unchanged; only the GCC control imports
those numeric inputs. The original loop exits through real Escape input.

Six additional projects build with GCC. Four native links require unavailable
SDL/OpenGL libraries; the image viewer's non-Windows source assumes POSIX
`realpath` and `getcwd`. Asteroids builds for both targets. Its unchanged original
program now exits normally through a real guest close-gadget click. All 480,000
RGBA pixels of its first full canvas match a separate GCC control of the 150
observed native random values. The native random values, loop and waits remain
unchanged. This checks the first-frame/normal-close session, not all gameplay.

A native probe exposed a driver defect: the private-palette AGA path created a
borderless window without a close gadget even for ordinary window requests.
Private palette ownership now stays separate from window decorations. Ordinary
windows reserve space for their actual Intuition borders; fullscreen and
explicitly borderless windows omit the title that otherwise reserves a hidden
top border. Chipset transfers now use client-area coordinates, as RTG transfers
already did. Adjusted windows cannot silently clip the BASIC client area.

The maintained native regression reproduces the old missing-gadget failure and
passes with the fix. It covers 8/16/32-bit native pixels, palettes, input, page
copying, cache refresh, oversized canvases and windowed/fullscreen/borderless
layouts. Evidence is under `native-window-layout-fix-v3` in the recovery folder.
The ongoing original recording retains its frozen executable and archives.

The triangle, rectangle-collision and indexed perpendicular-line examples now
complete with real Escape input and full canvas comparisons. The two original
FBgfx composition examples load all three actual BMP assets successfully and
match their complete 32-bit and RGB565 canvases. ToyBASIC passes its bundled
arithmetic input, two additional supported cases and its explicit unsupported
division path. Oregon Trail completes two original game endings with matching
transcripts and independently checked starting cash and inventory. These runs
retain their original source, waits and loops.

A private native zlib provider now supplies Prospector's memory compression
dependency. The upstream release checksum, static Hunk build adapter and
native compression/stream/checksum results are retained under
`zlib-native-provider`. Five native compressed outputs independently decode
to their exact input. Gzip linkage still needs SDK `fcntl`; gzip support is
not claimed. Prospector now links, while StrawberryNES needs FreeImage and
Pyromax needs OpenGL/GLU. No dependency is installed into the frozen recording.

R11 has genuinely completed its first new frame after recovery. There are now
46 reviewed native images and a protected frame-46 checkpoint. The first new frame's
full 1,024,000 pixels match its native-input GCC control exactly. The ordinary
GCC frame differs by at most four channel levels, with one pixel above one;
the complete differences remain recorded. The original 300-frame recording
and its final completion audit are still pending.

NebEdit now completes its normal Escape exit with unchanged source. All 32
bitmap loads succeed, and its complete 1024x768 RGBA scene matches a separate
GCC control of the actual native mouse values. Those native values are returned
unchanged; no control inputs are supplied to the guest.

The Griffon title audit found a shared adapter key-state mismatch: an integer
`-1` is compared with a previous byte value of `255`, repeating held key events.
A private `0/1` candidate passes press/hold/release checks on both targets and
the full GCC menu quit path. Its native run remains active in resource/audio
loading. The corpus header is unchanged and the program is not yet qualified.
The recovery log records the live job and a FBLITE bitmap-hook verifier issue
that must be resolved before its result is accepted.

All nine remaining BASICVERSE sources and seven freebasic-projects sources
now build from current snapshots on both targets, with two build workers.
These remain build-only results until execution is checked. The original
beautyofmath demo completes all nine formatted arithmetic rows with independently
checked values and matching GCC/native transcripts; its final SLEEP is retained.

Griffon's separate final-review service will check the actual whole title
canvas, native tick identity, real menu quit and original asset/configuration
inputs. It also checks opaque title-logo colours independently of GCC. The
FBLITE bitmap-hook limitation remains explicit; individual load-return receipts
are unavailable for that run. Its native audio preload and original recording
remain live, and the public corpus header is still unchanged.

The original regex example now completes three paired cases using a native
PCRE8 build with the host library's UTF/Unicode settings. URL validity and both
replacement results are independently checked. The GUI input example accepts
actual typed input, returns the exact name and completes its final wait; both
complete prompt/greeting canvases match GCC.

The intensity example is checked across four naturally reached colour phases,
including dark and bright pixels. All four complete RGBA canvases match a
separate GCC control of the observed native clocks. Independent calculations
also verify all 307,200 RGB pixels and 40,000 image-alpha values. The initial
black-only observation is retained as limited evidence, not colour-range
coverage. The original clock, source frame loop and waits remain unchanged.

Seven additional paired build gaps identify the package-specific SQLite and
libzip dependencies and the net library's Windows/Linux-only socket bindings.
The source datetime branch remains unported. Selected real misc units support
the verified examples; complete native fb-ext library coverage is not claimed.

The original Gematria database workflow completes with 32 whole records and
416 independently checked values. Its database file exactly matches GCC.
The graphical flight converter accepts actual menu/numeric input, retains its
result wait and exits normally. Its three full canvases, conversion value and
cyan/red/white/black palette are verified. The original 16-bit rotation and
indexed collision scenes also match whole GCC canvases and expected colours.

The real FreeType provider is pinned to the installed host version and built
with the native static ABI and actual zlib. The original GetTextWidth and
fonttest examples successfully load Vera.ttf. All 1,217 glyph loads, metrics,
204,257 alpha bytes, independent text-width sums and both full canvases match
GCC. Their original waits and real Escape exits remain intact. The GetTextWidth
sample paints white while using a white default font; its
white-on-white text does not indicate a font-loading failure.

The console name widget completes on both genuine interactive terminals with
the expected returned name and colour-state cycles. Its native stock Workbench
window has four pens and lacks the requested red/green; ROM font encoding also
differs from the original PC box characters. This functional session remains
outside the fully qualified count, with its physical-colour gap recorded.

Griffon's actual emulator remains live in audio loading. Recent observations
show increased CPU time but an unchanged gardens2.ogg file offset. A separate
decoder-progress check is needed; elapsed time and CPU use alone establish
neither preload completion nor a hang. The existing job remains preserved.

The original BASICVERSE palm diagnostic now completes both full 1280x800 views,
and the original shot programme completes its texture load, geometry and bloom.
Every pixel is compared against GCC, 3,072,000 pixels in total. There are 417
changed pixels, each differing by one final 8-bit channel level; none exceed
the allowance specified before execution. No masks or workload reductions are
used. Both programmes exit naturally.

A separate complete-file SFX probe decodes the actual pending Griffon music
asset and validates all 13,107,200 returned samples. Controlled graphics/SFX
loads pass both emulator audio-output settings and the FBLITE directory lookup.
These results do not explain the full game's I/O waits. An actual saved state
shows waiting tasks and ample free RAM. A further run uses the same exact game
binary under an owned debugger parent with central SFX diagnostics, preserving
all original assets/audio/configuration. It passed the first three music loads
before another waiting-task sample. The cause remains under investigation;
no guessed compiler/runtime fix is applied. Earlier runs remain preserved.

The proposed adapter key regression now has optional Make integration, verified
privately on GCC and Amiga. It retains the original failure and passing 0/1
candidate. Public corpus sources remain unchanged pending the full title audit.

The original JPEG/Scale2X example exposed two necessary library corrections.
The JPEG metadata-skip callback discarded its cursor and fed the file's start
marker twice. It now consumes bounded bytes and matches the declared C-long
parameter. The observed native BASIC/C structure sizes both equal 462 bytes;
no compiler packing change was needed. Scale2X also used zero outside the image,
creating four black corners. It now repeats the nearest border pixel as the
published algorithm requires, retaining the interior formula and public API.

The example and asset remain unchanged. Its full 307,200-pixel RGBA canvas
matches GCC exactly. All 37,636 source-image RGB pixels match an independent
JPEG decode, and all 150,544 scaled pixels match the independent Scale2X
reference without masks or tolerance. The combined maintained suite passes
689 assertions on GCC and Amiga, retaining the prior raster and row cases.

The new regression also exposed the fb-ext test runner mapping Boolean false
to successful process exit zero. Failure now exits one. The old JPEG callback
fails with nine GCC and ten native assertions; the old border code fails eight
on each target. Their fixed variants pass. New optional classic-JPEG tests use
the existing Make discovery and require a matching v6 provider. These necessary
library/test corrections are recorded by exact pre/post source hashes.

The original blur demo completes all 617 iterations and both JPEG loads, with
unchanged initial/final waits and actual final Escape. Both complete canvases
match GCC. The original rotate demo also passes actual Down/Left/Up/Escape
controls at observed 0/180/185/reset-0 angles. Its four full canvases match GCC,
and reset returns exactly to the first scene. Together these cases compare
460,800 RGBA pixels without masks or tolerance. Their initial JPEG colours
are independently checked; no example or implementation source changes were
needed for these two cases.

The sprite example's complete ten-step pulsation, three starfield scenes with
all 5,001 original vertices, and three complete 50-segment line-intersection
scenes also pass. All 4,915,200 RGBA pixels agree exactly with GCC. Sprite and
stars use ordinary GCC inputs; line-intersect has a separate GCC replay of
unchanged native RNG/clock inputs. An independent segment model verifies its
clipped sweep endpoints. The GCC/native polygon coordinates differ by at most
5.7e-14, bounded by four result ULPs; every colour byte must still agree.
No example changes or source loop caps were needed. Actual Escape exits each.

The original rotozoom ASM entry point is a documented corpus architecture
gap: its x86-64/ARM bodies are empty, and m68k enters the x86 assembly branch.
Normal native Make fails on those instructions. It is not a qualified scene
or an FBC emitter failure. The math module's custom Makefile separately builds
all nine original units successfully on both targets; the active shift64
branch is portable BASIC.

Both original GIF-loading branches pass. The original fixed RGB332 display
reduced 247 visible source colours to 52 colours despite exact software RGB.
The native driver now preserves all 247 colours exactly in its private palette,
with zero error across both complete Amiga bitmap readbacks. Software canvases
still match GCC and independent Pillow RGB. No corpus changes, pixel masks or
colour tolerances were needed. The real legacy giflib provider matches the
original binding's ABI and builds privately through normal configure/Make rules.

True-colour chipset displays now use exact pens when the frame and stable UI
colours fit. Borderless displays reserve only black for the surrounding screen.
A bounded lookup table and optional byte-per-pixel cache avoid converting rows
twice. Richer frames or cache-allocation failure retain nearest RGB fallback;
indexed PALETTE ownership and RTG conversion remain unchanged. Palette remaps
redraw unchanged rows too, and incomplete native transfers remain dirty for retry.

Maintained ordinary and MT regressions reproduce the baseline loss and pass
exact RGB565/32-bit colours, 240-colour table compaction, transitions to/from a
319-colour fallback frame, and all 256 pens in borderless displays. Existing
palette, cache, input, page, oversized-canvas and window-form checks also pass.
The source/test fix is applied. Future corpus runs use the qualified prefix
recorded in native-compiler-current.json; live recording and Griffon SDKs remain
immutable.

Vector-field and yinyang also complete three original full scenes each, with
2,880,000 RGBA pixels exactly matching GCC replay of unchanged native inputs.
The field accepts real positive/negative wheel events. Particle physics retains
all 40 particles, original RNG and real clock results, including the native
DateStamp clock's coarse initial zero delta.

Both 3drotate and PointAt exercise all three original drawing methods through
actual Space/Escape. PointAt retains its original 2/0/1 order. Filled modes are
byte-exact against GCC. The initial 3drotate wireframe differs at 588 shared-edge
pixels because C qsort may reorder equal-depth faces. Complete pre-sort geometry
and shading agree, both sorted arrays are valid permutations, and independent
line/font reconstruction verifies every wireframe pixel for each actual order.
No mask, tolerance, native reorder or corpus modification is used.

The original profiler completed its unchanged 12-worker configuration and ten
passes each of sky, raster and bloom. All 3,072,000 RGB pixels were checked:
sky is exact; only eight raster/bloom pixels differ by one final channel level.
Native exit0 and complete phase counters/controller records are retained.

The sprite benchmark exposed a corpus bug: it assigned frames1/2/3 to a
three-frame zero-based Sprite, and SetImage lacked a bounds check. ASan proves
the original heap write. The necessary caller/index guards and maintained tests
are applied; 717 assertions pass on GCC and Amiga, and fixed ASan exits0.
The complete benchmark then stops naturally on both targets, with every early
red/green/blue canvas matching GCC input replay. Original waits, growth and the
40ms rule are preserved. The separate original sprite demo also still passes.

The full textured benchmark is now running under its original 14-worker request
and all 20 sky/render/bloom iterations. GCC completed all passes; native
completion remains pending. Actual buffer exports follow original timing.
A real private checkpoint and stable volume copy are preserved, restoration
untested. All graphical runs use isolated displays and preserve the ongoing
original 300-frame recording.

Current source/build fingerprints also confirm that fbvnc, tiko, catfish and
phoenix lack the native gfxlib3 presentation adapter required by their original
recipes. These are documented source-platform gaps, without a new GCC-target
build or native-runtime claim. The original recording now has 53 genuine frames
reviewed; its complete 300-frame workload remains unfinished.


### Antialiased UI colours on chipset screens

Antialiased text can exceed the exact pen budget even when most pixels use a
few flat UI colours. The driver now counts up to 512 colours and, when four
colours cover at least three quarters of the canvas, preserves those colours
exactly. Remaining pens reduce the largest frequency-weighted squared RGB
errors. Rare shades map to the nearest retained colour. Richer frames retain
the fixed RGB fallback. Indexed modes and RTG presentation are unchanged.

The original AZdecrypt UI exposed this case: the same complete 1280x900 software
canvas contained 376 colours. Before this change, 256,361 displayed pixels
differed, with a maximum component error of 42. With the new palette, 330 pixels
differ by at most four levels; the background, borders and selection colours
are exact. These are actual planar bitmap and colour-map reads, with no masks.
The remaining differences are explicit eight-bit palette quantization.

Maintained ordinary and multithreaded graphics tests cover dominant UI colours,
rare colour bands, unchanged-row remapping, and all previous palette/input/page/
window checks. Both baseline variants reproduce the UI failure and both updated
variants pass. The unchanged GIF cases still retain all 247 displayed colours.
